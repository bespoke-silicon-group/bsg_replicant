# CUDA (CUDA Lite Runtime Tests)

This directory runs regression tests of Manycore
functionality on F1. Each test is a .c/.h, or a .cpp/.hpp file pair,
located in the `regression/cuda` directory.

To add a test, see the instructions in `regression/cuda/`. Tests
added to tests.mk in the `regression/cuda/` will automatically
be run during regression testing.

To run all tests in an appropriately configured environment, run:

```make regression``` 

Or, alternatively, run `make help` to see a list of available targets.
