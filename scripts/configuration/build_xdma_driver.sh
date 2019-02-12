#!/bin/bash

# This script builds and installs the Xilinx XDMA PCIe Driver 

make -C $AWS_FPGA_REPO_DIR/sdk/linux_kernel_drivers/xdma
sudo make -C $AWS_FPGA_REPO_DIR/sdk/linux_kernel_drivers/xdma install
