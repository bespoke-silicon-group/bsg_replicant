# BSG Custom Logic (CL) HDK Demo

This folder is a slightly modified version of the cl_hello_world example in the
aws-fpga example repository.

## Contents

- `build`: Contains the build flow scripts for AWS-F1
- `hardware`: Contains the top-level design files for your design
- `makefile`: Contains targets for RTL simulation, Co-Simulation, and Bild
- `software`: Contains the C-software for C Co-Simulation, and running on AWS-F1
- `testbenches`: Contains the testbench files for RTL and C Co-Simulation
- `README.md`: This file

## Getting Started

You can start your own AWS project in a few simple steps:

1. Copy this directory to a new location and rename it
2. Change the definition of `PROJECT` in the makefile
    - **`PROJECT` should match the name of the top-level module for your design**
3. Add your files to the `hardware` folder
    - Rename, reuse, and modify the top-level `hardware/cl-demo.sv` file
        - Make sure that the top-level module in your top-level design file matches the PROJECT variable.
4. Create and run your RTL Simulation
    - Rename, reuse, and modify the `testbenches/rtlsim/cl_tb.sv` RTL Testbench file
    - List your HDL source files in `testbenches/rtlsim/top.vivado.f`
    - Change the definition of `RTL_TB_MODULE` in `testbenches/rtlsim/makefile`
    - Run `make rtlsim`
        - Run `make rtlsim DEBUG=1` to bring up the waveform debugger
        - Run `make rtlsim CHECK=1` to instantiate the AXI Protocol checker
        - Run `make rtlsim CHECK=1 DEBUG=1` to do both the above
5. Create and run your C Co-Simulation
    - Rename, reuse, and modify the C source `software/cl_demo.c`
    - Change the definition of `C_TB_TOP` in `testbenches/cosim/makefile` to point to your new C file.
    - List your HDL source files in `testbenches/cosim/top.vivado.f` (Do not include the new top-level RTL testbench)
    - Run `make cosim`
        - Run `make cosim DEBUG=1` to bring up the waveform debugger
        - Run `make cosim CHECK=1` to instantiate the AXI Protocol checker
        - Run `make cosim CHECK=1 DEBUG=1` to do both the above
6. Create your AWS design tarball
    - List your hardware design files in `build/encrypt.tcl`
    - If necessary, edit the constraints in `build/constraints`
    - Run `make build`
    - Wait...
    - Your build results will be a `.tar.gz` file in `build/checkpoints/to_aws`

## Differences from cl_hello_world
The high-level modifications are as follows: 

- Renamed `verif` folder to `testbenches`
- Renamed `design` to `hardware`
- Added a top-level makefile with the following targets:
    - `rtlsim`: runs RTL simulation from the `testbenches/rtlsim` directory
    - `cosim`: runs Co-Simulation from the `testbenchmes/cosim` directory using
      the C application in `software`
    - `build`: runs the HDL Build flow from the `build` directory. This target can
      take several hours.

In general, the major changes in the makefile structure make it easier to re-use
this directory as a basis for new AWS-F1 projects with a simplified directory
and makefile structure.

## Notes:

The top-level makefile exports `CL_DIR` as an environment variable, as the path
to this directory.. This is necessary for the HDK build flow. If you run the
makefiles from the sub-directories you must have CL_DIR defined in your own
environment

The top-level makefile defines the PROJECT environment variable for the
sub-makefiles. Likewise, if you run makefiles in the sub-directories you must
define `PROJECT` from the command line (`PROJECT=cl_demo make <target>`) or in
your environment. `PROJECT` must contain the name of the top-level module in
your top-level design file.
