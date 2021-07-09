# SPMD (Single Program Multiple Data)

This directory runs regression tests of Manycore functionality using
the CUDA Lite runtime libraries. Each test is a .c/.h, or a .cpp/.hpp
file pair, located in the `regression/spmd` directory.

To run all tests in an appropriately configured environment, run:

```make regression``` 

Or, alternatively, run `make help` to see a list of available targets.
