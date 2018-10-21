# BSG F1 Starter Repository

This repository contains example/starter projects for Amazon F1. 

They are based on the example projects in the
[aws-fpga](https://github.com/aws/aws-fpga) repository. These sub-directories
have been modified to be less janky/fragile in the presence of
modifications/alternative build flows.

For more information about the changes I've made, see the subdirectories in this
repository.

To simulate/cosimulate/build these projects you must have: 

   1. Vivado 2018.2 on $PATH. (i.e. you must source the `settings64.sh` file.)
   2. A clone of aws-fpga (v1.4.3) in $AWS_FPGA_REPO_DIR

To simulate/cosimulate/build these projects:

   1. Clone this repository
   2. Run `source $AWS_FPGA_REPO_DIR/hdk_setup.sh`
   3. From inside one of the subfolders, run
      1. `make rtlsim` to run the RTL simluation
      2. `make cosim` to run C cosimulation
      3. `make build` to run vivado and build the project. 

The build result will be a `.tar.gz` file in the `<project>/build/checkpoints/to_aws`
folder.

Running `make clean` will remove all simulation, cosimulation and build results.

More documentation to follow...