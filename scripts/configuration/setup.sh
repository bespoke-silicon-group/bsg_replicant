#!/bin/bash

# Top-level setup script for building a BSG AMI.

if [ -z ${AWS_FPGA_REPO_DIR+x} ]; 
then 
    export AWS_FPGA_REPO_DIR=/home/centos/src/project_data/aws-fpga/; 
fi

./update_packages.sh
./clone_repositories.sh
./build_xdma_driver.sh
./set_xdma_permissions.sh
./build_mcl_driver.sh
./build_riscv_tools.sh
./aws_fpga_firstrun.sh

./copy_environment_scripts.sh

sudo shutdown -h now
