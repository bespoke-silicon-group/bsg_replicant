"""Compile with machine constants defined after the DPI makefile is included.

Run with python3 tests/test_dpi_defines.py; requires GNU Make and C/C++ compilers.
The fake uname selects each makefile branch, not a cross-platform runtime test.
"""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class DpiDefines(unittest.TestCase):
    def test_late_constants_and_feature_macros(self):
        repo = Path(__file__).resolve().parents[1]
        make = shutil.which("gmake") or shutil.which("make")
        for host in ("Darwin", "Linux"):
            with self.subTest(host=host), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                (root / "uname").write_text(f"#!/bin/sh\nprintf '%s\\n' {host}\n")
                (root / "uname").chmod(0o755)
                (root / "Makefile").write_text(f"""
LIBRARIES_PATH := {repo}/libraries
BSG_PLATFORM_PATH := $(LIBRARIES_PATH)/platforms/bigblade-verilator
ARGP_CPPFLAGS := -I.
DEFINES = -DNUM_POD_X=$(LATE_PODS) -D_XOPEN_SOURCE=500
DEFINES += -D_BSD_SOURCE -D_DEFAULT_SOURCE
include $(BSG_PLATFORM_PATH)/compilation.mk
LATE_PODS := 3
DEFINES += -DAFTER_INCLUDE=7
all: probe_c.o probe_cpp.o
""")
                condition = "defined" if host == "Darwin" else "!defined"
                source = f"""
#if NUM_POD_X != 3 || AFTER_INCLUDE != 7
#error late constants were lost
#endif
#if {condition}(_XOPEN_SOURCE) || {condition}(_BSD_SOURCE) || {condition}(_DEFAULT_SOURCE)
#error wrong feature macros for selected host branch
#endif
int probe(void) {{ return NUM_POD_X; }}
"""
                (root / "probe_c.c").write_text(source)
                (root / "probe_cpp.cpp").write_text(source)
                env = dict(os.environ, PATH=str(root) + os.pathsep + os.environ["PATH"])
                result = subprocess.run([make, "--no-print-directory", "all"],
                                        cwd=root, env=env, text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertTrue((root / "probe_c.o").is_file())
                self.assertTrue((root / "probe_cpp.o").is_file())


if __name__ == "__main__":
    unittest.main()
