CC = gcc
obj-m += bsg_manycore_kernel_driver.o
MAJOR_NUM = 0

IS_BSG_MANYCORE_LOADED = $(shell lsmod | grep "bsg_manycore_kernel_driver")
IS_XDMA_LOADED = $(shell lsmod | grep "xdma")
build:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules modules_install

install: build 
ifeq ($(IS_XDMA_LOADED),)
else
	#xdma driver loaded. will unload this driver first.
	rmmod xdma
endif
	modprobe bsg_manycore_kernel_driver slot=0x1d
	dmesg | grep "major number is:" | awk '{system("mknod /dev/bsg_manycore_kernel_driver c " $$NF " 0")}'
	chmod 777 /dev/bsg_manycore_kernel_driver



clean:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean

uninstall: clean
	sudo rm -f /dev/bsg_manycore_kernel_driver
	sudo rmmod bsg_manycore_kernel_driver
endif

.PHONY: build install clean uninstall


