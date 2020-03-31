# BSG Replicant: Cosimulation and Emulation Infrastructure for running HammerBlade

## Contents

This repository contains the following folders: 

- `build`: Vivado scripts for building FPGA Design Checkpoint Files to upload to AWS-F1
- `hardware`: HDL sources, ROM scripts, and package files
- `libraries`: C/C++ driver and CUDA-lite Runtime library sources
- `regression`: C/C++ Regression tests for co-simuation and AWS F1 Execution
- `testbenches`: Testbench makefiles for C/C++ Co-Simulation
- `machines`: Customized `Makefile.machine.include` file for different designs.

This repository contains the following files:

- `Makefile`: Contains targets for Co-Simulation, Bitstream Generation, F1 Regression
- `machine.mk`: Defines the path to the current Machine Configuration for co-simulation
- `README.md`: This file
- `cadenv.mk`: A makefile fragment for deducing the CAD tool environment
- `environment.mk`: A makefile fragment for deducing the build environment. 
- `hdk.mk`: A makefile fragment for deducing the AWS-FPGA HDK build environment.

## Machine Configurations

Each Manycore configuration is called a "Machine" and defines a size,
memory type, memory hierarchy, cache type, and many other
parameters. Each machine is defined by  `Makefile.machine.include`
file. 

To switch machines set `BSG_MACHINE_PATH`, defined in
[machine.mk](machine.mk), to define the current target for
simulation. To switch machines, modify the value of `BSG_MACHINE_PATH`
to point to any subdirectory of the `machines` directory.

See [machines/README.md](machines/README.md) for more documentation.

# Quick-Start

The simplest way to use this project is to clone it's meta-project: [BSG Bladerunner](https://github.com/bespoke-silicon-group/bsg_bladerunner/). 

BSG Bladerunner tracks this repository, BSG Manycore, and BaseJump STL
repositories as submodules and maintains a monotonic versionining
scheme. BSG Bladerunner also contains cosimulation instructions.

## Dependencies

To simulate/co-simulate/build these projects you must have the following tools.

   1. Vivado 2019.1
   2. A clone of aws-fpga
   3. Synopsys VCS (We use O-2018.09-SP2, but others would work)

This repository depends on the following repositories: 

   1. [BSG Manycore](https://github.com/bespoke-silicon-group/bsg_manycore)
   2. [BaseJump STL](https://github.com/bespoke-silicon-group/basejump_stl)
   3. [AWS FPGA](https://github.com/aws/aws-fpga)




