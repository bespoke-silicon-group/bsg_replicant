#!/bin/bash
INCLUDES = -I$(SDK_DIR)/userspace/include
INCLUDES += -I$(HDK_DIR)/common/software/include -I.
CFLAGS = -std=c11 -fPIC $(INCLUDES)
# This script builds and installs the BSG Manycore Runtime API
build: bsg_manycore_driver.o bsg_manycore_loader.o bsg_manycore_mem.o bsg_manycore_print.o
	$(CC) -shared -Wl,-soname,libbsg_manycore_runtime.so.1 -o libbsg_manycore_runtime.so.1.0  bsg_manycore_driver.o bsg_manycore_loader.o bsg_manycore_mem.o bsg_manycore_print.o	
	
install: build
	mkdir /usr/lib/bsg_manycore
	mv libbsg_manycore_runtime.so.1.0 /usr/lib/bsg_manycore/
	ln -sf /usr/lib/bsg_manycore/libbsg_manycore_runtime.so.1.0 /usr/lib/bsg_manycore/libbsg_manycore_runtime.so.1
	ln -sf /usr/lib/bsg_manycore/libbsg_manycore_runtime.so.1.0 /usr/lib/bsg_manycore/libbsg_manycore_runtime.so
	cp -t /usr/include ../../cl_manycore/libraries/bsg_manycore_driver.h ../../cl_manycore/libraries/bsg_manycore_loader.h ../../cl_manycore/libraries/bsg_manycore_mem.h ../../cl_manycore/libraries/bsg_manycore_print.h 

clean:
	rm *.o

uninstall: clean
	rm -rf /usr/lib/bsg_manycore /usr/include/bsg_manycore_*

.PHONY: build install clean uninstall 
