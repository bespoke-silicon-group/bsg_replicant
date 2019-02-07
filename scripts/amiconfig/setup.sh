#!/bin/bash

# Top-level setup script for building a BSG AMI.

# $1 = aws_ver
# $2 = AGFI
# $N N+1 = (bsg_repo_name, commit_id)

if [ -z ${AWS_FPGA_REPO_DIR+x} ]; 
then 
    export AWS_FPGA_REPO_DIR=/home/centos/src/project_data/aws-fpga/; 
fi

#./update_packages.sh
./clone_repositories.sh $1 $3 $4 $5 $6 $7 $8
./build_xdma_driver.sh
./set_xdma_permissions.sh
./build_mcl_driver.sh
./build_riscv_tools.sh
./aws_fpga_firstrun.sh

./copy_environment_scripts.sh $2

sudo shutdown -h now
