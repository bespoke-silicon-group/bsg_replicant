#!/bin/sh

sudo rmmod bsg_dma_driver.ko
cd driver
make clean
make
sudo setpci -s 0x1d COMMAND=6
sudo insmod bsg_dma_driver.ko slot=0x1d
cd ..
make clean
make
