#!/bin/bash

# This script builds the Manycore RISC-V tools 

sudo yum -y install autoconf automake libmpc-devel mpfr-devel gmp-devel gawk  bison flex texinfo patchutils gcc gcc-c++ zlib-devel expat-devel
sudo yum -y clean all
sudo yum -y autoremove

make -C ~/bsg_manycore/software/riscv-tools checkout-all
make -C ~/bsg_manycore/software/riscv-tools build-riscv-tools
