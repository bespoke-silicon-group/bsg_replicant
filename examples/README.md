# Examples

This directory contains HammerBlade examples

To add a test, see the instructions in the relevant subdirectories.

## Contents

This directory contains the following folders: 

- `library`: Tests for the BSG Manycore Runtime Library
- `spmd`: Tests for basic Manycore functionality. Most tests launch a self-checking program from the [BSG Manycore SPMD Directory](https://github.com/bespoke-silicon-group/bsg_manycore/tree/master/software/spmd)
- `cuda`: Tests for CUDA-Lite functionality. These launch CUDA-Lite kernels from [BSG Manycore CUDA-Lite Directory](https://github.com/bespoke-silicon-group/bsg_manycore/tree/master/software/spmd/bsg_cuda_lite_runtime)
- `python`: Tests with a Python top level (e.g. for TVM)
- `apps`: HammerBlade examples that require additional work to run (e.g. submodules, datasets, packages, etc.). Each app should include a README describing the instructions to set it up.

This directory contains the following files:

- `Makefile`: Contains targets for regression (Run `make help` for more information)
- `compile.mk`: A makefile fragment for compiling tests.
- `link.mk`: A makefile fragment for linking tests.
- `execution.mk`: A makefile fragment for compiling tests.
- `regression.mk`: A makefile fragment for running regression and reporting results.

## Quick-Start

To run all the regression tests in each of the sub folders run: 

`make regression`

To run a specific sub-suite, run: 

`make <sub-directory name>`

Alternatively, navigate into one of the subdirectories and run `make
help` for more information.
