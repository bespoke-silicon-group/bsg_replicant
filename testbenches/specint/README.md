# Specint

This directory runs cosimulation regression tests of Manycore
functionality on F1. Each test is a .c/.h, or a .cpp/.hpp file pair,
located in the `regression/specint` directory.

To add a test, see the instructions in `regression/specint/`. Tests
added to tests.mk in the `regression/specint/` will automatically
be run during regression testing.

To run all tests in an appropriately configured environment, run:

```make regression``` 

Or, alternatively, run `make help` to see a list of available targets.
