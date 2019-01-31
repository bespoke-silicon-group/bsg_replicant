# Software

The software directory contains the C application for an AWS hardware design.

## Contents

- `cl_app.c`: The C application for this AWS hardware design. Drives
  co-simulation and an FPGA instance on a server.
- `include`: Include files for this AWS hardware design
- `makefile`: Makefile for building the C application for AWS. (The cosimulation
  binary is built by the Vivado tools, separately)
- `README.md`: This file

## Writing your C application

If you are porting this project to a new application you must follow three steps:

1. Rename and reuse `cl_app.c`
    - If necessary, replace `cl_app.c` in SRC's definition in `makefile`
    - If necessary, replace `cl_app` in BIN's definintion in `makefile`
2. Cosimulate your application by running `make cosim` from the parent directory
3. Upload your application to an AWS instance and run `make` in this directory.
4. Run the output binary

## Notes

The C Macro `SV_TEST` is used to block includes and definitions used by the
cosimulation framework. **In particular** it replcaes `int main` with the
function `int test_main`, which is called from
`testbenches/cosim/cosim_wrapper.sv` (using `main` will cause linker errors,
because the co-simulator already has a `main` method). Therefore, **do not remove
`test_main`** if you intend to use this C application for co-simulation.

