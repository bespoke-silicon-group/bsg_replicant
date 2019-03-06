CC = gcc
obj-m += bsg_manycore_kernel_driver.o
MAJOR_NUM = 0

IS_BSG_MANYCORE_LOADED = $(shell lsmod | grep "bsg_manycore_kernel_driver")
IS_XDMA_LOADED = $(shell lsmod | grep "xdma")
IS_0x1d_LOADED = $(shell lspci -v -s 0x1d | grep "Kernel driver in use")
build:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules modules_install

install: build 
ifneq ($(IS_XDMA_LOADED), )
     	#xdma is in use, remove it first
	rmmod xdma
endif
ifneq ($(IS_0x1d_LOADED), )
     	#slot 0x1d is loaded, remove it first
	rmmod xocl
endif
	modprobe bsg_manycore_kernel_driver slot=0x1d
	dmesg | grep "major number is:" | awk '{system("mknod /dev/bsg_manycore_kernel_driver c " $$NF " 0")}'
	chmod 777 /dev/bsg_manycore_kernel_driver

clean:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean

uninstall: clean
	sudo rm -f /dev/bsg_manycore_kernel_driver
	sudo rmmod bsg_manycore_kernel_driver

.PHONY: build install clean uninstall


