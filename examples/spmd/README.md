# SPMD (Single Program Multiple Data)

This directory runs regression tests of Manycore functionality using
the CUDA Lite runtime libraries. Each test is a .c/.h, or a .cpp/.hpp
file pair, located in the `regression/spmd` directory.

To run all tests in an appropriately configured environment, run:

```make regression``` 

Device C/C++ sources use GCC by default. Select the HammerBlade LLVM build
with `make regression SPMD_COMPILER=llvm LLVM_DIR=/path/to/llvm-build`.
The legacy `CLANG=1` spelling remains supported.

Or, alternatively, run `make help` to see a list of available targets.
