# Co-Simulation (Testbenches)

This directory tree contains makefiles and source files for running
C/C++ Co-Simulation. In Co-Simulation a C/C++ top level drives inputs
to the simulated RTL running in VCS.

Our C/C++ top-level files are the same as used in regression testing
on F1, and are located in the directory tree of `bsg_replicant/regression/`.

## Contents

This directory contains the following folders: 

- `library`: Tests for the BSG Manycore Runtime Library
- `spmd`: Tests for basic Manycore functionality. Most tests launch a self-checking program from the [BSG Manycore SPMD Directory](https://github.com/bespoke-silicon-group/bsg_manycore/tree/master/software/spmd)
- `cuda`: Tests for CUDA-Lite functionality. These launch CUDA-Lite kernels from [BSG Manycore CUDA-Lite Directory](https://github.com/bespoke-silicon-group/bsg_manycore/tree/master/software/spmd/bsg_cuda_lite_runtime)
- `python`: Tests with a Python top level (e.g. for TVM)

This directory contains the following files:

- `Makefile`: Contains targets for Co-Simulation (Run `make help` for more information)
- `cosimulation.mk`: A makefile fragment for running and building Co-Simulation binaries. Used in all of the sub-folders.
- `link.mk`: A makefile framgment for linking cosimulation binaries

## Quick-Start

To run all the regression tests in each of the sub folders run: 

`make regression`

To run a specific sub-suite, run: 

`make <sub-directory name>`

Alternatively, navigate into one of the subdirectories and run `make
help` for more information.
