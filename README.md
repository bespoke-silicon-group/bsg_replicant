# BSG F1 Starter Repository

This repository contains Bespoke Silicon projects for Amazon AWS F1. 

Each subfolder that starts with `cl_` represents a working project. These
projects are based on the example projects in the
[aws-fpga](https://github.com/aws/aws-fpga) repository.  However, they have been
modified to be less janky/fragile in the presence of modifications/alternative
build flows.

| Project       | Description |
| ------------- | ----------- |
| `cl_hdk`      | HDL-based "Hello World" project derived from cl_hello_world in aws-fpga |
| `cl_ipi`      | IP Integrator-based "Hello World" project derived from cl_hello_world_hdk in aws-fpga |
| `cl_mcl`      | HDL Project demonstrating the Host-To-ManycoreLink API |
| `cl_manycore` | HDL Project for the BSG Manycore architecture (Depends on changes in `cl_mcl`) |

There are also several non-project folders. These are described below: 

| Folder    | Description |
| --------- | ----------- |
| `hdl`     | Verilog used in more than one project. For example, `cl_mcl` and `cl_manycore` use the same Verilog for interfacing with PCI Express |
| `scripts` | Scripts used to create, configure and boot AWS Images and scripts to compile and upload FPGA Images |

## Dependencies

To simulate/cosimulate/build these projects you must have: 

   1. Vivado 2018.2 on $PATH. (i.e. you must source the `settings64.sh` file.)
   2. A clone of aws-fpga (v1.4.3) with the exported environment variable 
      - Run `export AWS_FPGA_REPO_DIR=<path to aws-fpga>` or add it to your `.bashrc` file

## Quick-Start Simulate/Cosimulate/Build Instructions

To simulate/cosimulate/build these projects:

   1. Clone this repository
   2. Run `source $AWS_FPGA_REPO_DIR/hdk_setup.sh`
   3. From inside one of the projects, run:
      * `make rtlsim` to run the RTL simluation
      * `make cosim` to run C cosimulation
      * `make build` to run vivado and build the project. 

The build result will be a file named `<timestamp>.Developer_CL.tar` file in
`$AWS_FPGA_REPO_DIR/<project>/build/checkpoints/to_aws/` folder.

Running `make clean` will remove all simulation, cosimulation, and build results.

## F1 Deployment Instructions:

The following instructions describe the process of creating an AWS Account,
loading an FPGA image, and launching an AWS F1 Instance. 

### Get, and set up your AWS account (You only need to do this once):
    
   1. Request an account from Dustin (dustinar@uw.edu). Save your: 

      * **Access key ID** (e.g. AKIAIOSFODNN7EXAMPLE)

      * **Secret access key** (e.g. wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY)

   2. Set the AWS command Line Interface for your account using this guide: [link](https://docs.aws.amazon.com/cli/latest/userguide/cli-chap-configure.html)

### Build your FPGA Design
    
   1. Load the Xilinx tools in your environment (e.g.: `source <path to Vivado>/<version>/settings64.sh>`)

   2. Set up the AWS Build environment (e.g.: `source <path to aws-fpga>/hdk_setup.sh`)

   3. Run `make build` from any one of the project directories in this repository

   This will generate the file `<timestamp>.Developer_CL.tar` in `build/checkpoints/to_aws/`

### Deploy your FPGA Design

   1. Create an S3 bucket for your design. (You can skip this step if you already have an S3 bucket): 

       `$ aws s3 mb s3://<bucket-name> --region <region>`

   2. Upload your design to S3: (*Note, the trailing `/` is critical*)

       `$ aws s3 cp build/checkpoints/to_aws/<timestamp>.Developer_CL.tar s3://<bucket-name>/<folder-name>/`

   3. Create a log file 

       `$ touch log.txt`

       `$ aws s3 cp log.txt s3://<bucket-name>/<folder-name>/`

   4. Process your design into an Amazon FPGA Image (AFI)

      `$ aws ec2 create-fpga-image --region <region> --name <afi-name> --description <afi-description> 
          --input-storage-location Bucket=<bucket-name>,Key=<folder-name>/<timestamp>.Developer_CL.tar
          --logs-storage-location Bucket=<bucket-name>,Key=<folder-name>/log.txt`

   	  For example: `--name=cl_manycore_4x4_20181231_v1 --description=\"Manycore 4x4 design 2019/12/31 version 1\"`

   5. Record the AGI and AGFI for your design from the previous command

      * Amazon FPGA Image Identifier (AFI ID), e.g. `afi-06d0ffc989feeea2a`

      * Amazon Global FPGA Image Identifier (AGFI ID), e.g. `agfi-0f0e045f919413242`

   6. Check the status of your FPGA design:

       `$ aws ec2 describe-fpga-images --fpga-image-ids afi-06d0ffc989feeea2a`

       If AFI state is 'pending' your design is still processing. When it is done, the AFI state is set to 'avaliable'

### Load your design on F1

When your design has finished processing it can be deployed on F1. If you are unfamiliar with launching and using instances you should read [this tutorial](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/LaunchingAndUsingInstances.html) first.

   1. Launch a f1.2xlarge instance (If you don't have one running already)

   2. Clone the AWS FPGA Management tools (If your instance does not have them)

       `$ git clone https://github.com/aws/aws-fpga.git $AWS_FPGA_REPO_DIR`

   3. Load the AWS SDK and HDK Tools:

       `$ source $AWS_FPGA_REPO_DIR/sdk_setup.sh; source $AWS_FPGA_REPO_DIR/hdk_setup.sh`

   4. Clear any AFI you have previously loaded in your slot

       `$ sudo fpga-clear-local-image  -S 0`

   5. Load your AFI to FPGA slot 0:

       `$ sudo fpga-load-local-image -S 0 -I agfi-0f0e045f919413242`

   6. Validate that the AFI was loaded properly

       `$ sudo fpga-describe-local-image -S 0 -R -H`

   7. Build and run your design

       This exercise is left to the user
