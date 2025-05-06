# BSG Replicant: Execution/Emulation Infrastructure for HammerBlade

## Quickstart

The simplest way to use this project is to clone its meta-project: [BSG Bladerunner](https://github.com/bespoke-silicon-group/bsg_bladerunner/). 

BSG Bladerunner tracks this repository, BSG Manycore, and BaseJump STL
repositories as submodules and maintains a monotonic versionining
scheme. 

Once the setup instructions in BSG Bladrunner have been completed, run:

`make regression`

This will run all of the example programs in [examples](examples). See
the README.md file in that directory for more information. We make
every attempt to ensure the examples there are working.

## Contents

This repository contains the following folders: 

- `hardware`: HDL sources, package files, and ROM scripts.
- `libraries`: C/C++ driver and runtime library sources.
- `examples`: Example C/C++ applications and regression tests.
- `machines`: Customized `Makefile.machine.include` file for different HammerBlade designs and configurations.
- `build`: Vivado scripts for building FPGA Design Checkpoint Files to upload to AWS-F1. **Currently stale/not actively supported**.

This repository contains the following files:

- `README.md`: This file.
- `Makefile`: Targets for Regression, and (stale) bitstream generation commands.
- `platform.mk`: Defines the path to the current exeuction Platform (BSG_PLATFORM_PATH).
- `machine.mk`: Defines the path to the current Machine Configuration (BSG_MACHINE_PATH).
- `environment.mk`: A makefile fragment for deducing the build environment.
- `cadenv.mk`: A makefile fragment for deducing the CAD tool environment (e.g. VCS_HOME).
- `hdk.mk`: A makefile fragment for deducing the AWS-FPGA HDK build environment (stale).

## Platforms

HammerBlade applications can be run on multiple platforms. These
platforms could simulate the architecture (VCS, Verilator), emulate
it, or run natively.

We currently support two platforms:

- `bigblade-vcs`: Native (x86) host execution, simulated HammerBlade (with VCS, using Verilog DPI for IO)
- `bigblade-verilator`: Native (x86) host execution, simulated HammerBlade (with Verilator, using Verilog DPI for IO)

Each platform has different advantages and drawbacks. Simulated
platforms support an in-depth profiling infrastructure and emulated
memory systems via non-synthesizable constructs. VCS is a 4-state
simulator, but requires Synopsys and VCS licenses. Verilator does not
require licenses, but it cannot simulated an FPGA system with
encrypted HDL.

To select the execution platform, set the `BSG_PLATFORM` variable in
[platform.mk](platform.mk). Most users will use `bigblade-vcs`.

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

## Dependencies

To use VCS Platforms, users will need: 

   1. Synopsys VCS (We use O-2018.09-SP2, but others would work)

To use Verilator platforms, users will need: 

   1. A recent version of Verilator

Users should use the Verilator installation provided by
bsg_bladerunner.

