INCLUDES := -I$(SDK_DIR)/userspace/include
INCLUDES += -I$(HDK_DIR)/common/software/include -I.

CFLAGS  := -std=c11 -fPIC $(INCLUDES)
LDLIBS  := -lfpga_mgmt 

HEADERS := bsg_manycore_driver.h
HEADERS += bsg_manycore_loader.h
HEADERS += bsg_manycore_mem.h
HEADERS += bsg_manycore_print.h
HEADERS += bsg_manycore_packet.h

OBJECTS := bsg_manycore_driver.o
OBJECTS += bsg_manycore_loader.o
OBJECTS += bsg_manycore_mem.o
OBJECTS += bsg_manycore_print.o

# This script builds and installs the BSG Manycore Runtime API
build: $(OBJECTS)
	$(CC) $(CFLAGS) -shared -Wl,-soname,libbsg_manycore_runtime.so.1 -o libbsg_manycore_runtime.so.1.0 $(OBJECTS) $(LDLIBS)	

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
