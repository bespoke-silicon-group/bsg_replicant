# SPMD (Single Program Multiple Data)

This directory runs cosimulation regression tests of Manycore functionality on
F1. Each test is a .c/.h, or a .cpp/.hpp file pair, located in the `regression`
directory of the design.

To add a test, see the instructions in `regression/spmd/`. Tests
added to Makefile.tests in the `regression/spmd/` will automatically
be run in this directory. 

To run all tests in an appropriately configured environment, run:

```make cosim``` 

The Makefile in this directory expects that the user has set `CL_DIR`,
`BSG_IP_CORES_DIR`, `BSG_MANYCORE_DIR`, and sourced the script hdk_setup.sh
inside of aws-fgpa.



