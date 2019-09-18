# Build Flow

This directory contains the Vivado Build flow for BSG F1. 

This file is not intended as a end-all for build flow issues. You
should see the documentation in the aws-fpga repository for more
information. This is a simplified (but equally powerful) build flow
compared to those provided with the AWS examples.

## Contents

This directory contains the following folders: 

- `constraints`: FPGA Constraints for an AWS FPGA design

This directory contains the following files:

- `Makefile`: Provides targets for building and cleaning a design
- `aws_build_dcp_from_cl.sh`: BASH script file that builds an AWS design in three steps
    1. Collects and encrypts all the source files listed in `encrypt.tcl`
    2. Synthesizes the RTL Design
    3. Implements (places & routes) the RTL Design
    4. Creates a Design Checkpoint (DCP) file
    5. Collects all design files and the final DCP in a tarball (.tar)
- `create_dcp.tcl`: TCL script responsible for creating the DCP file that is required by Amazon.
- `synth.tcl`: Synthesis commands for Vivado
    - Reads all verilog files copied and encrypted by `encrypt.tcl`
- `README.md`: This file

# Quick-Start

For a list of makefile targets, run: 

`make help`
