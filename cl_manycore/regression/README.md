# Regression

The regression directory contains the C/C++ Regression Tests for the Manycore
application. 

To add a test, see the instructions in the relevant subdirectories.

## Contents

   - `library`: These programs test the F1 library. These programs cannot
     require a binary running on the manycore.

   - `spmd`: These programs test the Manycore functionality. These programs
     expect a matching binary in the `software/spmd` directory of
     `bsg_manycore`

   - `Makefile`: This is a simple makefile for running all of the regression tests
     in the sub-directories. If a new directory is added, simply add it to the
     `TARGETS` variable and it should be added to `make regression`

   - `Makefile.include`: This Makefile snippet is included by the Makefiles in the
     subdirectories.

## Notes

All tests in the subdirectories must:

   - Have a `.c` and `.h` file OR `.cpp` and `.hpp` file. 
   - The names of the `.c`/`.h` OR `.cpp`/`.hpp` must match
   - The names must art with `test_`
   - Return 0 on success or non-zero on failure
   - Use `bsg_pr_test_info` and `bsg_pr_test_pass_fail` defined in `cl_manycore_regression.h` instead of stdio calls.
