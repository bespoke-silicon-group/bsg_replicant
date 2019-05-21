INCLUDES := -I$(SDK_DIR)/userspace/include
INCLUDES += -I$(HDK_DIR)/common/software/include -I$(AWS_FPGA_REPO_DIR)/SDAccel/userspace/include -I.

CXXFLAGS  := -std=c++11 -fPIC $(INCLUDES)
LDLIBS  := -lfpga_mgmt 

HEADERS := bsg_manycore_driver.h
HEADERS += bsg_manycore_loader.h
HEADERS += bsg_manycore_tile.h
HEADERS += bsg_manycore_mem.h
HEADERS += bsg_manycore_mmio.h
HEADERS += bsg_manycore_errno.h
HEADERS += bsg_manycore_bits.h
HEADERS += bsg_manycore_elf.h
HEADERS += bsg_manycore_memory_manager.h
HEADERS += bsg_manycore_cuda.h
HEADERS += bsg_manycore_features.h
HEADERS += bsg_manycore_printing.h
HEADERS += bsg_manycore_config.h
HEADERS += bsg_manycore_coordinate.h
HEADERS += bsg_manycore.h

OBJECTS := bsg_manycore_driver.o
OBJECTS += bsg_manycore_loader.o
OBJECTS += bsg_manycore_mem.o
OBJECTS += bsg_manycore_tile.o
OBJECTS += bsg_manycore_bits.o
OBJECTS += bsg_manycore_elf.o
OBJECTS += bsg_manycore_memory_manager.o
OBJECTS += bsg_manycore_cuda.o
OBJECTS += bsg_manycore_printing.o
OBJECTS += bsg_manycore_config.o
OBJECTS += bsg_manycore.o

# This script builds and installs the BSG Manycore Runtime API
build: $(OBJECTS)
	$(CXX) $(CFLAGS) -shared -Wl,-soname,libbsg_manycore_runtime.so.1 -o libbsg_manycore_runtime.so.1.0 $(OBJECTS) $(LDLIBS)	

install: build
	mv libbsg_manycore_runtime.so.1.0 /usr/lib64/
	ln -sf /usr/lib64/libbsg_manycore_runtime.so.1.0 /usr/lib64/libbsg_manycore_runtime.so.1
	ln -sf /usr/lib64/libbsg_manycore_runtime.so.1.0 /usr/lib64/libbsg_manycore_runtime.so
	cp -t /usr/include $(HEADERS)

clean:
	rm -f *.o

uninstall: clean
	sudo rm /usr/lib64/libbsg_manycore_* /usr/include/bsg_manycore*.h

.PHONY: build install clean uninstall 
