# Libraries

This directory contains the BSG Manycore Runtime Library. This runtime library
is a collection of C/C++ files that implement the low-level driver for issuing
reads and writes to the high-level CUDA-Lite driver for launching tile groups.

## Contents: 

This directory contains the following files:

- `Makefile`: Contains targets for building and installing the runtime library on an F1 instance
- `libraries.mk`: A makefile fragment that contains targets for building the runtime library as a shared object. `libraries.mk` is reused by `compilation.mk` in the `bsg_f1/testbenches` directory. 
- `*.cpp/*.h`: C++ files that implement various parts of the runtime library

## Quick-Start

To see the available make targets run: 

`make help`