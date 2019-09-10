# Library (Host Library Tests)

This directory contains regression tests for Host-Library functionality on
F1. Each test is a .c/.h, or a .cpp/.hpp file pair.

All tests must:
   - Have a `.c` and `.h` file OR `.cpp` and `.hpp` file. 
   - The names of the `.c`/`.h` OR `.cpp`/`.hpp` must match
   - Return 0 on success or non-zero on failure

To add a test, create a `.c` and `.h` file, or a `.cpp` and `.hpp` file then add
the name to the list in Makefile.tests in this directory.