# Co-Simulation (Testbenches)

This directory tree contains makefiles and source files for running
C/C++ Co-Simulation. In Co-Simulation a C/C++ top level drives inputs
to the simulated RTL running in VCS. 

Our C/C++ top-level files are the same as used in regression testing
on F1, and are located in the directory tree of `bsg_f1/regression/`.

## Contents

This directory contains the following folders: 

- `library`: Tests for the BSG Manycore Runtime Library
- `spmd`: Tests for basic Manycore functionality. Most tests launch a self-checking program from the [BSG Manycore SPMD Directory](https://github.com/bespoke-silicon-group/bsg_manycore/tree/master/software/spmd)
- `cuda`: Tests for CUDA-Lite functionality. These launch CUDA-Lite kernels from [BSG Manycore CUDA-Lite Directory](https://github.com/bespoke-silicon-group/bsg_manycore/tree/master/software/spmd/bsg_cuda_lite_runtime)
- `python`: Tests with a Python top level (e.g. for TVM)

This directory contains the following files:

- `Makefile`: Contains targets for Co-Simulation (Run `make help` for more information)
- `cosimulation.mk`: A makefile fragment for running and building Co-Simulation binaries. Used in all of the sub-folders.
- `compilation.mk`: A makefile fragment for building Co-Simulation binaries. Used in cosimulation.mk.
- `simlibs.mk`: A makefile fragment for building hardware and software simulation libraries
- `aws.vcs.f`: A VCS filelist containing all of the AWS VCS Sources
- `gen_simlibs.tcl`: A TCL Script for exporting the pre-compiled VCS Simulation libraires from Vivado
- `cosim_wrapper.sv`: The Top-Level Verilog file for RTL-C/C++ Co-Simulation. Calls the C/C++ function `test_cosim`.
- `sh_dpi_tasks.svh`: A System Verilog Header containing a list of exported System Verilog DPI tasks. This version differs slightly than the one provided in [AWS-FPGA](https://github.com/aws/aws-fpga/blob/master/hdk/common/verif/include/sh_dpi_tasks.svh)

## Quick-Start

To run all the regression tests in each of the sub folders run: 

`make regression`

To run a specific sub-suite, run: 

`make <sub-directory name>`

Alternatively, navigate into one of the subdirectories and run `make
help` for more information.
