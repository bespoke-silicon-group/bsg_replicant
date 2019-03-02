#!/bin/bash
INCLUDES = -I$(SDK_DIR)/userspace/include
INCLUDES += -I$(HDK_DIR)/common/software/include -I.
CFLAGS = -std=c11 -fPIC $(INCLUDES)
# This script builds and installs the BSG Manycore Runtime API
build: bsg_manycore_driver.o bsg_manycore_loader.o bsg_manycore_mem.o bsg_manycore_print.o
	$(CC) -shared -Wl,-soname,libbsg_manycore_runtime.so.1 -o libbsg_manycore_runtime.so.1.0  bsg_manycore_driver.o bsg_manycore_loader.o bsg_manycore_mem.o bsg_manycore_print.o	
	
install: build
	mv libbsg_manycore_runtime.so.1.0 /usr/lib64/
	ln -sf /usr/lib64/libbsg_manycore_runtime.so.1.0 /usr/lib64/libbsg_manycore_runtime.so.1
	ln -sf /usr/lib64/libbsg_manycore_runtime.so.1.0 /usr/lib64/libbsg_manycore_runtime.so
	cp -t /usr/include ../../cl_manycore/libraries/bsg_manycore_driver.h ../../cl_manycore/libraries/bsg_manycore_loader.h ../../cl_manycore/libraries/bsg_manycore_mem.h ../../cl_manycore/libraries/bsg_manycore_print.h 

clean:
	rm -f *.o

uninstall: clean
	sudo rm /usr/lib64/bsg_manycore_* /usr/include/bsg_manycore_*

.PHONY: build install clean uninstall 
