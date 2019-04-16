# SPMD (Single Program Multiple Data)

This directory contains regression tests for Manycore functionality on F1. Each
test is a .c/.h, or a .cpp/.hpp file pair. 

All tests must:
   - Have a `.c` and `.h` file OR `.cpp` and `.hpp` file. 
   - The names of the `.c`/`.h` OR `.cpp`/`.hpp` must match
   - The name must start with `test_`. The remainder must match the name of a
     manycore binary in the `software/spmd` directory of `bsg_manycore`
   - Return 0 on success or non-zero on failure

To add a test, create a `.c` and `.h` file, or a `.cpp` and `.hpp` file then add
the name to the list in Makefile.tests in this directory.