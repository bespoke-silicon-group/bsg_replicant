# BSG Replicant: Cosimulation and Emulation Infrastructure for HammerBlade

## Contents

This repository contains the following folders: 

- `build`: Vivado scripts for building FPGA Design Checkpoint Files to upload to AWS-F1
- `hardware`: HDL sources, ROM scripts, and package files
- `libraries`: C/C++ driver and CUDA-lite Runtime library sources
- `examples`: Example C/C++ applications and regression tests.
- `machines`: Customized `Makefile.machine.include` file for different HammerBlade designs.

This repository contains the following files:

- `README.md`: This file
- `Makefile`: Contains targets for Bitstream Generation, and Regression
- `platform.mk`: Defines the path to the current exeuction Platform (BSG_PLATFORM_PATH)
- `machine.mk`: Defines the path to the current Machine Configuration (BSG_MACHINE_PATH)
- `environment.mk`: A makefile fragment for deducing the build environment.
- `cadenv.mk`: A makefile fragment for deducing the CAD tool environment (e.g. VCS_HOME)
- `hdk.mk`: A makefile fragment for deducing the AWS-FPGA HDK build environment.

## Platforms

HammerBlade applications can be run on multiple platforms. These
platforms could simulate the manycore and run the host code natively
(VCS, Verilator), emuate the manycore (AWS F1) and run the host code
natively, or simulate the manycore and host (coming soon!).

We currently support four platforms:

- `aws-fpga`: Native (x86) host execution, Emulated Manycore (with an FPGA), 
- `aws-vcs`: Native (x86) host execution, Simulated Manycore (with VCS, on a simulated FPGA)
- `dpi-verilator`: Native (x86) host execution, Simulated Manycore (with Verilator, using DPI for IO)
- `dpi-vcs`: Native (x86) host execution, Simulated Manycore (with VCS, using DPI for IO)

Each platform has different advantages and drawbacks. Simulated
platforms support an in-depth profiling infrastructure and emulated
memory systems via non-synthesizable constructs. VCS is a 4-state
simulator, but requires Synopsys and VCS licenses. Verilator does not
require licenses, but it cannot simulated an FPGA system with
encrypted HDL. The `aws-fpga` systems is much faster, but has a
limited size, and no introspection or profiling tools.

To select the execution platform, set the `BSG_PLATFORM` variable in
[platform.mk](platform.mk). Most users will use `dpi-vcs` or
`dpi-verilator`. Users with Vivado installed can use `aws-vcs`. Users
with access to AWS F1 images should use `aws-fpga`.

## Machines

Each HammerBlade configuration is called a *Machine* and defines a
size, memory type, memory hierarchy, cache type, and many other
parameters. Each machine is defined by `Makefile.machine.include`
file in [machines](machines).

To switch machines set `BSG_MACHINE_PATH`, defined in
[machine.mk](machine.mk). To switch machines, modify the value of
`BSG_MACHINE_PATH` to point to any subdirectory of the `machines`
directory.

See [machines/README.md](machines/README.md) for more documentation.

# Quick-Start

The simplest way to use this project is to clone its meta-project: [BSG Bladerunner](https://github.com/bespoke-silicon-group/bsg_bladerunner/). 

BSG Bladerunner tracks this repository, BSG Manycore, and BaseJump STL
repositories as submodules and maintains a monotonic versionining
scheme. 

Once the setup instructions in BSG Bladrunner have been completed, run:

`make regression`

This will run all of the example programs in [examples](examples). See
the README in that directory for more information.

## Dependencies

To use the `aws-vcs` platform, users will need: 

   1. Vivado 2019.1
   2. A clone of aws-fpga (Cloned by bsg_bladerunner)
   3. Synopsys VCS (We use O-2018.09-SP2, but others would work)

To use the `dpi-verilator` platform, users will need: 

   1. A recent version of Verilator

Users should use the Verilator installation provided by
bsg_bladerunner.

This repository depends on the following repositories: 

   1. [BSG Manycore](https://github.com/bespoke-silicon-group/bsg_manycore)
   2. [BaseJump STL](https://github.com/bespoke-silicon-group/basejump_stl)
   3. [AWS FPGA](https://github.com/aws/aws-fpga)




