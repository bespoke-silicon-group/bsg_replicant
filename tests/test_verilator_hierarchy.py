"""Small build-policy tests; no HammerBlade model or simulations required."""
import importlib.util
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

REPO = Path(__file__).resolve().parents[1]
PLATFORM = REPO / "libraries/platforms/bigblade-verilator"
SPEC = importlib.util.spec_from_file_location("hierarchy", PLATFORM / "hierarchy.py")
H = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(H)


class Hierarchy(unittest.TestCase):
    def setUp(self):
        # These tests check platform defaults as well as explicit overrides.
        # A user's simulation policy must not change the expected defaults.
        environment = patch.dict(os.environ)
        environment.start()
        self.addCleanup(environment.stop)
        for name in ('VERILATOR_THREADS', 'VERILATOR_HIERARCHY',
                     'VERILATOR_PROFILE_HIERARCHY', 'VERILATOR_OPT_FAST',
                     'VERILATOR_OPT_SLOW', 'VERILATOR_OPT_GLOBAL', 'VDEFINES',
                     'MAKEFLAGS', 'MFLAGS'):
            os.environ.pop(name, None)

    def test_stamp_and_validation(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "stamp"
            H.stamp(path, "processor", "1", "flags")
            before = path.stat().st_mtime_ns
            H.stamp(path, "processor", "1", "flags")
            self.assertEqual(before, path.stat().st_mtime_ns)
            for mode, threads in (("typo", "1"), ("flat", "0"),
                                  ("flat", "-1"), ("processor", "16")):
                with self.assertRaises(ValueError):
                    H.stamp(path, mode, threads, "flags")
            H.stamp(path, "flat", "16", "other flags")
            self.assertIn('"mode": "flat"', path.read_text())

    def test_unused_child_is_not_success(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "Vbsg_manycore_proc_vanilla_c").mkdir()
            with self.assertRaises(ValueError):
                H.check_reuse(root, "top")
            symbol = "bsg_manycore_proc_vanilla_c_protectlib_combo_update"
            (root / "Vtop__Dpi.h").write_text("void " + symbol + "();")
            (root / "Vtop__Dpi.cpp").write_text("extern void " + symbol + "();")
            with self.assertRaises(ValueError):
                H.check_reuse(root, "top")
            (root / "Vtop__0.cpp").write_text(symbol + "(handle);")
            self.assertEqual(H.check_reuse(root, "top"), [symbol])
            (root / "Vtop__0.cpp").write_text("    result = " + symbol + "(handle);")
            self.assertEqual(H.check_reuse(root, "top"), [symbol])

    def test_generation_failure_invalidates_makefile(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            makefile = root / "Vtop.mk"
            makefile.touch()
            with patch.object(H.subprocess, "check_output", return_value="Verilator 5.050"), \
                 patch.object(H.subprocess, "run") as run:
                with self.assertRaises(ValueError):
                    H.generate(root, "top", "processor", ["verilator", "--cc"])
                self.assertFalse(makefile.exists())
                self.assertIn("--hierarchical-params-file", run.call_args.args[0])

    def test_make_branches(self):
        make = shutil.which("gmake") or shutil.which("make")
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for name in ("hardware.mk", "libraries.mk"):
                (root / name).touch()
            (root / "Makefile").write_text(f"""
BSG_PLATFORM_PATH := {root}
LIBRARIES_PATH := {root}
HARDWARE_PATH := {root}
BSG_MACHINExPLATFORM_PATH := {root}/machine
BSG_DESIGN_TOP := probe
VERILATOR := echo
include {PLATFORM}/link.mk
""")
            for host in ("Linux", "Darwin"):
                (root / "uname").write_text(f"#!/bin/sh\necho {host}\n")
                (root / "uname").chmod(0o755)
                env = dict(os.environ, PATH=str(root) + os.pathsep + os.environ["PATH"])
                for variant in ("exec", "profile", "trace", "debug"):
                    target = str(root / "machine" / variant / "Vprobe.mk")
                    result = subprocess.run([make, "-n", target], cwd=root, env=env,
                                            text=True, capture_output=True)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    command = result.stdout
                    self.assertIn("--mode " + ("flat" if variant == "debug" else "processor"), command)
                    self.assertEqual("--profile-ports" in command, variant in ("profile", "trace"))
                    self.assertEqual('+define+"VERILATOR_WORKAROUND_DISABLE_VCORE_TRACE"' in command,
                                     variant in ("exec", "profile"))
                    self.assertEqual('+define+"BSG_ENABLE_VANILLA_CORE_TRACE"' in command,
                                     variant in ("trace", "debug"))
                    self.assertEqual('--trace-fst' in command, variant == "debug")
                    self.assertEqual("+define+\"BSG_MACHINE_DISABLE_REMOTE_OP_PROFILING\"" in command,
                                     variant == "exec")
                    if variant == "exec":
                        self.assertIn("--threads 1", command)
                    if variant == "debug":
                        self.assertIn("--trace-fst", command)
                result = subprocess.run([make, "-n", "VERILATOR_HIERARCHY=flat",
                                         str(root / "machine/exec/Vprobe.mk")],
                                        cwd=root, env=env, text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn("--threads " + ("16" if host == "Linux" else "1"), result.stdout)
                self.assertNotIn("+define+\"BSG_MACHINE_DISABLE_REMOTE_OP_PROFILING\"", result.stdout)
                result = subprocess.run([make, "-n", "VERILATOR_PROFILE_HIERARCHY=flat",
                                         str(root / "machine/profile/Vprobe.mk")],
                                        cwd=root, env=env, text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn("--mode flat", result.stdout)
                self.assertNotIn("+define+\"BSG_MACHINE_DISABLE_VCORE_PROFILING\"", result.stdout)
                for setting in ("VERILATOR_HIERARCHY=flat", "VERILATOR_PROFILE_HIERARCHY=flat"):
                    result = subprocess.run([make, "-n", setting,
                                             str(root / "machine/trace/Vprobe.mk")],
                                            cwd=root, env=env, text=True, capture_output=True)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertIn("--mode flat", result.stdout)
                    self.assertIn("--threads 1", result.stdout)
                    self.assertNotIn('+define+"VERILATOR_WORKAROUND_DISABLE_VCORE_TRACE"', result.stdout)

            # Existing task/application profile defines must not silently turn
            # the explicitly selected trace binary into another no-text model.
            for define in ("VERILATOR_WORKAROUND_DISABLE_VCORE_TRACE",
                           "VERILATOR_WORKAROUND_DISABLE_VCORE_TRACE=1"):
                result = subprocess.run([make, "-n", "VDEFINES=" + define + " KEEP_ME=7",
                                         str(root / "machine/trace/Vprobe.mk")],
                                        cwd=root, text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertNotIn('+define+"VERILATOR_WORKAROUND_DISABLE_VCORE_TRACE', result.stdout)
                self.assertIn('+define+"KEEP_ME=7"', result.stdout)

    def test_debug_support_dependencies_are_isolated(self):
        make = shutil.which("gmake") or shutil.which("make")
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for name in ("hardware.mk", "libraries.mk"):
                (root / name).touch()
            include = root / "include"
            (include / "fstcpp").mkdir(parents=True)
            (include / "fstcpp/fstcpp_writer.cpp").touch()
            (include / "verilated_fst_c.cpp").touch()
            (include / "verilated.cpp").touch()
            (root / "Makefile").write_text(f"""
BSG_PLATFORM_PATH := {root}
LIBRARIES_PATH := {root}
HARDWARE_PATH := {root}
BSG_MACHINExPLATFORM_PATH := {root}/machine
BSG_DESIGN_TOP := probe
VERILATOR_ROOT := {root}
VERILATOR_LZ4_PREFIX := /test/lz4
VERILATOR_FST_CPPFLAGS := -I/test/lz4/include
VERILATOR_FST_LDLIBS := -L/test/lz4/lib -llz4
include {PLATFORM}/link.mk
""")
            for variant, source in (("exec", "verilated"), ("debug", "verilated_fst_c")):
                result = subprocess.run([make, "-n", str(root / "machine" / variant / (source+".o"))],
                                        cwd=root, text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
                self.assertEqual("-I/test/lz4/include" in result.stdout, variant == "debug")
                # The debug support stamp must also track link dependencies.
                self.assertEqual("-llz4" in result.stdout, variant == "debug")

    def test_named_build_and_run_targets(self):
        make = shutil.which("gmake") or shutil.which("make")
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for name in ("hardware.mk", "libraries.mk", "main.so", "main.riscv"):
                (root / name).touch()
            (root / "Makefile").write_text(f"""
BSG_PLATFORM_PATH := {root}
LIBRARIES_PATH := {root}
HARDWARE_PATH := {root}
BSG_MACHINE_PATH := {root}/machine
BSG_PLATFORM := platform
BSG_MACHINExPLATFORM_PATH := $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)
BSG_DESIGN_TOP := probe
BSG_MANYCORE_KERNELS := main.riscv
VERILATOR := echo
include {PLATFORM}/link.mk
include {PLATFORM}/execution.mk
""")
            for variant in ("exec", "profile", "trace", "debug"):
                binary = root / "machine/platform" / variant / "simsc"
                binary.parent.mkdir(parents=True)
                binary.touch()
                for target in ("sim-" + variant, variant + ".log"):
                    result = subprocess.run([make, "-n", "-o", str(binary), target],
                                            cwd=root, text=True, capture_output=True)
                    self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
                    self.assertEqual(" | tee " in result.stdout, target.endswith(".log"))
                    if target.endswith(".log"):
                        self.assertIn(str(binary), result.stdout)
            self.assertFalse(list(root.glob("*.log")), "dry run launched a simulator")

    def test_dpi_unit_excludes_stale_children(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "Vtop__Dpi.cpp").touch()
            child = "Vbsg_manycore_hetero_socket_1"
            (root / child).mkdir()
            (root / child / (child + "__Dpi.cpp")).touch()
            H.dpi_unit(root, "top", [child[1:] + "_protectlib_combo_update"])
            self.assertIn(child, (root / "bsg_unified_dpi.cpp").read_text())
            H.dpi_unit(root, "top", [])
            self.assertNotIn(child, (root / "bsg_unified_dpi.cpp").read_text())

    def test_old_manycore_has_clear_profile_error(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            with patch.object(H.subprocess, "check_output", return_value="Verilator 5.050"):
                with self.assertRaisesRegex(ValueError, "update bsg_manycore"):
                    H.generate(root, "top", "processor", ["verilator", "--cc"], True)

    @unittest.skipUnless(os.environ.get("TEST_VERILATOR"), "set TEST_VERILATOR for real generation/build checks")
    def test_real_processor_flat_processor_rebuild(self):
        make = shutil.which("gmake") or shutil.which("make")
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for name in ("hardware.mk", "libraries.mk"):
                (root / name).touch()
            (root / "probe.sv").write_text("""
module top(input logic clk, input logic [3:0] a, output logic [3:0] y, z);
  bsg_manycore_proc_vanilla #(.N(4)) u0(clk, a, y);
  bsg_manycore_proc_vanilla #(.N(4)) u1(clk, y, z);
endmodule
module bsg_manycore_proc_vanilla #(parameter N=8)
  (input logic clk, input logic [N-1:0] a, output logic [N-1:0] y);
  always_ff @(posedge clk) y <= a + 1'b1;
endmodule
""")
            (root / "Makefile").write_text(f"""
BSG_PLATFORM_PATH := {root}
LIBRARIES_PATH := {root}
HARDWARE_PATH := {root}
BSG_MACHINExPLATFORM_PATH := {root}/machine
BSG_DESIGN_TOP := top
VERILATOR := {os.environ['TEST_VERILATOR']}
VSOURCES := {root}/probe.sv
include {PLATFORM}/link.mk
""")
            directory = root / "machine/exec"
            archive = directory / "Vtop__ALL.a"
            for mode in ("processor", "flat", "processor"):
                result = subprocess.run([make, "-j2", "VERILATOR_HIERARCHY=" + mode,
                                         "VERILATOR_THREADS=1", "CXX=" + os.environ.get("CXX", "c++"),
                                         str(archive)], cwd=root, text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                symbols = subprocess.check_output(["nm", "-g", str(archive)], text=True)
                self.assertEqual("protectlib_combo_update" in symbols, mode == "processor")
                before = archive.stat().st_mtime_ns
                result = subprocess.run([make, "VERILATOR_HIERARCHY=" + mode,
                                         "VERILATOR_THREADS=1", "CXX=" + os.environ.get("CXX", "c++"),
                                         str(archive)], cwd=root, text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(before, archive.stat().st_mtime_ns, "no-op build rebuilt archive")
            # A flags-only change must rebuild child as well as parent code.
            result = subprocess.run([make, "VERILATOR_OPT_FAST=-Os", "VERILATOR_THREADS=1",
                                     "CXX=" + os.environ.get("CXX", "c++"), str(archive)],
                                    cwd=root, text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            commands = [line for line in result.stdout.splitlines() if " -c " in line]
            self.assertTrue(any(" -Os " in line and "bsg_manycore_proc_vanilla" in line
                                for line in commands), "child flags were not refreshed")


if __name__ == "__main__":
    unittest.main()
