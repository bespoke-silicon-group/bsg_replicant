#!/bin/env python3
import os
import sys
import pathlib
import subprocess
import argparse
from ctypes.util import find_library

msg = """ HammerBlade Test Runner

"""

parser = argparse.ArgumentParser(
    description=msg,
    formatter_class=argparse.RawDescriptionHelpFormatter,
    conflict_handler='error')

parser.add_argument("NAME", nargs=1,
                    type=str, help="The name of the test to run")

# Deduce the current suite from the execution path
cwd = pathlib.Path(os.getcwd())
suite = cwd.name

# Todo: Fail if not library, spmd, or cuda?
argfmt = "noargs"
if(suite in {"cuda", "spmd"}):
    argfmt = "path+kernel"

parser.add_argument("--arg-fmt", nargs=1, default=argfmt,
                    type=str, choices=["noargs", "path+kernel"],
                    help="Argument format")

parser.add_argument("--binary", nargs=1, type=str, default="",
                    help="Path to HammerBlade kernel binary")

# TODO: Document more
parser.add_argument("--platform", nargs=1, default=None,
                    type=str, choices=["aws-vcs", "aws-fpga", "native"],
                    help="Execution platform. (Deduction attempted)")

parser.add_argument("--quiet", action="store_true",
                    help="Suppress stdout and write to a log file")

parser.add_argument("--waveform", action="store_true",
                    help="Generate a waveform (Only supported by aws-vcs platform)")

parser.add_argument("--cargs", nargs=argparse.REMAINDER, type=str, default="",
                    help="String-enclosed additional C/C++ arguments")

parser.add_argument("--pargs", nargs=argparse.REMAINDER, type=str, default="",
                    help="String-enclosed additional platform arguments")

# Parse arguments
args = parser.parse_args()

# We'll build execution arguments as an array so we can pass it to
# subprocess.
exeargs = []

# TODO: Default to None?

# Get the processes' current working directory

# Build executable path
#  Check if executable exists (and executable)
#  Check if test_loader exists (and executable)
tname = args.NAME[0]
nom = pathlib.Path(tname)
ldr = pathlib.Path("test_loader")
if(nom.exists()):
    exe = nom.resolve()
elif(ldr.exists()):
    exe = ldr.resolve()
else:
    print(f"BSG ERROR: Executables {nom} and test_loader "
          f" not found in {cwd}  --  ",
          "Has the executable been built?")
    exit(1)

# Verify that the path is executable
if(not os.access(exe, os.X_OK)):
    print(f"BSG ERROR: File {exe} is not executable. Has an executable been specified?")
    exit(1)

# Deduce platform (if None). TODO: Document More
if(not args.platform):
    # On the aws-fpga platform, libfpga_mgmt is installed in the
    # system library path and will be found by ctypes. On simulation
    # platform, it is not, and will not be found by ctypes.
    if(find_library("fpga_mgmt")):
        platform="aws-fpga"
    else:
        # Otherwise, it's a simulation platform

        # Run the simulation command, but pass a VCS-specific
        # argument. This will cause VCS to return 0 (pass) and other
        # simulations should return non-zero (fail).
        ret = subprocess.run([str(exe), "+vcs+finish+0"], 
                             stderr=subprocess.STDOUT,
                             stdout=subprocess.DEVNULL,
                             cwd=str(cwd))

        # If the command passed, assume aws-vcs. Otherwise, assume
        # native.
        if(not ret.returncode):
            platform = "aws-vcs"
        else:
            platform = "native"

# Check if kernel binary exists
if(args.binary):
    kpath = pathlib.Path(args.binary[0])
elif(suite in {"cuda", "spmd"}):
    # Get the path to bsg_manycore by finding bsg_replicant in the cwd
    # path, and then searching its parent for bsg_manycore.
    idx = 0
    try:
        idx = cwd.parts.index("bsg_replicant")
    except ValueError:
        print("BSG ERROR: bsg_replicant is not in directory tree. "
              "Specify --binary or run from inside of BSG Replicant")
        exit(1)

    parent = pathlib.Path(*cwd.parts[:idx])

    # Search bsg_replicant's parent' directory for bsg_manycore
    manycore = parent / "bsg_manycore"
    if(not manycore.exists()):
        print("BSG ERROR: bsg_manycore directory could not be found. "
              f"Searched {str(parent)}. Specify --binary or run from "
              "inside of BSG Replicant")
        exit(1)

    # Specify the kernel source suite directory
    kdir = manycore / "software" / "spmd"
    if(suite == "cuda"):
        kdir /= "bsg_cuda_lite_runtime"
    
    # Assume that the kernel binary is located in bsg_manycore, in a
    # directory without the test_ prefix.
    kpath = kdir / tname[len("test_"):] / "main.riscv"
    if(not kpath.exists()):
        print("BSG ERROR: Kernel binary not found at default path: "
              f"{str(kpath)}. Specify --binary or build kernel")
        exit(1)
else:
    kpath = None

# Produce platform Arguments
pargs = [args.pargs]
if(args.waveform):
    if(platform == "aws-vcs"):
        pargs += ["+vcs+vcdpluson"]
        pargs += ["+vcs+vcdplusmemon"]
        pargs += ["+memcbk"]

        # Add .debug suffix to the executable to generate the waveform
        exe = exe.with_suffix(".debug")
        if(not exe.exists()):
            print(f"BSG ERROR: Waveform executable {exe.relative_to(cwd)} "
                  "does not exist. Has it been built?")
    else:
        print("BSG ERROR: --waveform specified, but platform "
              f"{platform} does not support waveform generation")
        exit(1)

exeargs += pargs

# Format Arguments
if(platform == "aws-vcs"):
    vcs_args = "+c_args="
    if suite in {"cuda", "spmd"}:
        vcs_args += f"{str(kpath)} {tname} "

    vcs_args += args.cargs
    exeargs += [vcs_args]

elif(platform == "aws-fpga"):
    exeargs = ["sudo"] + [exeargs]

else:
    exeargs += [kpath, tname, args.cargs]

cmd = [str(exe), *exeargs]
log = exe.with_suffix(".hb_runner.log")
with open (log, "w") as logfile:
    print(f"BSG INFO: Command is:\t{' '.join(cmd)}")
    
    fp = None
    if(args.quiet):
        fp = logfile
    
    ret = subprocess.run(cmd, 
                         stderr=subprocess.STDOUT,
                         stdout=fp,
                         cwd=str(cwd))

    if(not ret.returncode):
        print(f"BSG INFO: Test {nom} Passed!")
    else:
        print(f"BSG INFO: Test {nom} Failed!")
