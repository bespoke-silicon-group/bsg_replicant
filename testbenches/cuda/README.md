# CUDA (Host CUDA Tests)

This directory runs cosimulation regression tests of Manycore
functionality on F1. Each test is a .c/.h, or a .cpp/.hpp file pair,
located in the `regression/library` directory.

Each test also has a corresponding .riscv binary file in the bsg_manycore/software/spmd/bsg_cuda_lite_runtime/ directory

To add a test, see the instructions in `regression/cuda/`. Tests
added to Makefile.tests in the `regression/cuda/` will automatically
be run in this directory. 

To run all tests in an appropriately configured environment, run:

```make cosim``` 

Or, alternatively, run `make help` to see a list of available targets.



