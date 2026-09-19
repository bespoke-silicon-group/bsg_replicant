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
                for variant in ("exec", "profile", "debug"):
                    target = str(root / "machine" / variant / "Vprobe.mk")
                    result = subprocess.run([make, "-n", target], cwd=root, env=env,
                                            text=True, capture_output=True)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    command = result.stdout
                    self.assertIn("--mode " + ("processor" if variant == "exec" else "flat"), command)
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
