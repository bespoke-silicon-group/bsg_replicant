#!/bin/sh

cd driver/
make clean
make 
sudo setpci -s 0x1d command=6
sudo insmod bsg_dma_driver.ko slot=0x1d
cd ..
