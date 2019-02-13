#!/bin/bash

# This script builds and installs the BSG Manycore Link PCIe Driver

cd ../../cl_manycore/software/deploy/driver
make clean
make
sudo rmmod xdma.ko
sudo insmod bsg_dma_driver.ko slot=0x1d
dmesg | grep "major number is:" | awk '{system("sudo mknod /dev/bsg_dma_driver c " $NF " 0")}'
sudo chmod 777 /dev/bsg_dma_driver
cd ../../../../scripts/configuration
