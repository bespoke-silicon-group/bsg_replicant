# Build Flow

This file is not intended as a end-all for build flow issues. You should see the
documentation in the aws-fpga repository for more information. This is a
simplified (but equally powerful) build flow compared to those provided with the
AWS examples.

## Contents

- `constraints`: FPGA Constraints for your design
- `aws_build_dcp_from_cl.sh`: BASH script file that builds an AWS design in three steps
    1. Collects and encrypts all the source files listed in `encrypt.tcl`
    2. Synthesizes the RTL Design
    3. Implements (places & routes) the RTL Design
    4. Creates a Design Checkpoint (DCP) file
    5. Collects all design files and the final DCP in a tarball (.tar.gz)
- `create_dcp.tcl`: TCL script responsible for creating the DCP file that is required by Amazon.
- `encrypt.tcl`: Contains a list of hardware design files that should be synthesized and encrypted
- `makefile`: Provides targets for building and cleaning a design
- `synth.tcl`: Synthesis commands for Vivado
    - Reads all verilog files copied and encrypted by `encrypt.tcl`
- `README.md`: This file

## Building a Design

You should really build a design from the parent directory...

## Preparing a Design

All you need to do is list your HDL files in `encrypt.tcl`. The build flow
should take care of the rest.

## Notes

When DDR is used, there are additional directives in `synth.tcl` that may need
to be uncommented.

