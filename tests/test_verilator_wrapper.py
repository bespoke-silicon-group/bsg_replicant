"""Exercise the production wrapper/build rule with a cache-line-aligned model.

The model and Verilator callbacks are small substitutes; the wrapper source,
public header and Make rule are copied unchanged from the platform. No RTL
generation is needed. Run with CXX selecting a supported GCC or Clang compiler.
"""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import time
import unittest


REPO = Path(__file__).resolve().parents[1]
PLATFORM = REPO / "libraries/platforms/bigblade-verilator"


class SimulatorWrapper(unittest.TestCase):
    def test_aligned_lifetime_and_cached_wrapper_rebuild(self):
        compiler = os.environ.get("CXX", "c++")
        make = shutil.which("gmake") or shutil.which("make")
        env = dict(os.environ)
        for key in ("MAKEFLAGS", "MFLAGS"):
            env.pop(key, None)
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            policy = root / "policy"
            policy.mkdir()
            shutil.copyfile(PLATFORM / "link.mk", policy / "link.mk")
            source = root / "bsg_manycore_simulator.cpp"
            shutil.copyfile(PLATFORM / source.name, source)
            shutil.copyfile(REPO / "libraries/bsg_manycore_simulator.hpp",
                            root / "bsg_manycore_simulator.hpp")
            for name in ("hardware.mk", "libraries.mk"):
                (root / name).touch()
            include = root / "include"
            include.mkdir()
            (include / "verilated.h").write_text("""
struct Verilated {
    static void assertOn(bool) {}
    static void traceEverOn(bool) {}
};
""")
            (root / "bsg_nonsynth_dpi_clock_gen.hpp").write_text("""
namespace bsg_nonsynth_dpi {
struct bsg_timekeeper { static void next() {} };
}
""")
            (include / "Vreplicant_tb_top.h").write_text("""
class alignas(64) Vreplicant_tb_top {
public:
    void eval() {}
    void final() {}
};
""")
            driver = root / "probe.cpp"
            driver.write_text(r'''
#include "bsg_manycore_simulator.hpp"
#include <cstdlib>
#include <new>
static int allocations = 0, releases = 0;
void* operator new(std::size_t size, std::align_val_t alignment) {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, static_cast<std::size_t>(alignment), size))
        throw std::bad_alloc();
    ++allocations;
    return ptr;
}
void operator delete(void* ptr, std::align_val_t) noexcept {
    ++releases;
    std::free(ptr);
}
void operator delete(void* ptr, std::size_t, std::align_val_t alignment) noexcept {
    ::operator delete(ptr, alignment);
}
int main() {
    {
        SimulationWrapper wrapper;
        wrapper.eval();
        wrapper.assertOn(true);
        if (wrapper.getRoot() != "TOP.replicant_tb_top") return 1;
    }
    // Deterministic even when ordinary malloc happens to return aligned memory.
    return allocations == 1 && releases == 1 ? 0 : 2;
}
''')
            (root / "Makefile").write_text(f"""
BSG_PLATFORM_PATH := {root}
LIBRARIES_PATH := {root}
HARDWARE_PATH := {root}
BASEJUMP_STL_DIR := {root}
BSG_MACHINE_PATH := {root}/machine
BSG_MACHINExPLATFORM_PATH := {root}/machine
BSG_DESIGN_TOP := probe
VERILATOR_ROOT := {root}
include {policy}/link.mk
""")
            def run(command):
                result = subprocess.run(command, cwd=root, env=env, text=True,
                                        capture_output=True)
                self.assertEqual(result.returncode, 0,
                                 f"{command}\n{result.stdout}{result.stderr}")

            # Freeze generated archives: changing wrapper policy must work even
            # when regenerating the model leaves its archive untouched.
            old_time = time.time() - 60
            os.utime(source, (old_time, old_time))
            os.utime(policy / "link.mk", (old_time, old_time))
            objects = []
            for mode in ("exec", "profile", "trace", "debug"):
                with self.subTest(mode=mode):
                    directory = root / "machine" / mode
                    directory.mkdir(parents=True)
                    archive = directory / "Vprobe__ALL.a"
                    archive.touch()
                    os.utime(archive, (old_time, old_time))
                    obj = directory / "bsg_manycore_simulator.o"
                    command = [make, "--no-print-directory", "CXX=" + compiler,
                               "-o", str(archive), str(obj)]
                    run(command)
                    probe = directory / "probe"
                    run(shlex.split(compiler) + ["-std=c++17", "-I" + str(root),
                                                str(driver), str(obj), "-o", str(probe)])
                    run([str(probe)])
                    before = obj.stat().st_mtime_ns
                    run(command)
                    self.assertEqual(obj.stat().st_mtime_ns, before,
                                     "unchanged build recompiled the wrapper")
                    objects.append((obj, command))

            # Arrange distinct, past timestamps without sleeps or touching the
            # repository sources. Only the copied Make policy becomes newer.
            policy_time = time.time_ns() - 1_000_000_000
            object_time = policy_time - 1_000_000_000
            for obj, _ in objects:
                os.utime(obj, ns=(object_time, object_time))
            os.utime(policy / "link.mk", ns=(policy_time, policy_time))
            for obj, command in objects:
                with self.subTest(rebuild=obj.parent.name):
                    before = obj.stat().st_mtime_ns
                    run(command)
                    self.assertNotEqual(obj.stat().st_mtime_ns, before,
                                        "build-policy update retained a cached wrapper")
                    after = obj.stat().st_mtime_ns
                    run(command)
                    self.assertEqual(obj.stat().st_mtime_ns, after)


if __name__ == "__main__":
    unittest.main()
