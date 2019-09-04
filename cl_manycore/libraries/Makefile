include ../environment.mk
include $(CL_DIR)/hdk.mk
INCLUDES += -I$(SDK_DIR)/userspace/include
INCLUDES += -I$(HDK_DIR)/common/software/include 
INCLUDES += -I$(AWS_FPGA_REPO_DIR)/SDAccel/userspace/include -I.

CFLAGS   += -std=c11 -D_GNU_SOURCE -fPIC $(INCLUDES)
CXXFLAGS += -std=c++11 -fPIC $(INCLUDES)
LDFLAGS  += -lfpga_mgmt

HEADERS += bsg_manycore_loader.h
HEADERS += bsg_manycore_tile.h
HEADERS += bsg_manycore_vcache.h
HEADERS += bsg_manycore_eva.h
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
HEADERS += bsg_manycore_npa.h
HEADERS += bsg_manycore_epa.h
HEADERS += bsg_manycore_origin_eva_map.h
HEADERS += bsg_manycore_packet.h
HEADERS += bsg_manycore_response_packet.h
HEADERS += bsg_manycore_request_packet.h
HEADERS += bsg_manycore_fifo.h
HEADERS += bsg_manycore_request_packet_id.h
HEADERS += bsg_manycore_responder.h

OBJECTS += bsg_manycore_loader.o
OBJECTS += bsg_manycore_eva.o
OBJECTS += bsg_manycore_tile.o
OBJECTS += bsg_manycore_bits.o
OBJECTS += bsg_manycore_elf.o
OBJECTS += bsg_manycore_memory_manager.o
OBJECTS += bsg_manycore_cuda.o
OBJECTS += bsg_manycore_printing.o
OBJECTS += bsg_manycore_config.o
OBJECTS += bsg_manycore.o
OBJECTS += bsg_manycore_origin_eva_map.o
OBJECTS += bsg_manycore_request_packet_id.o
OBJECTS += bsg_manycore_responder.o
OBJECTS += bsg_manycore_uart_responder.o
OBJECTS += bsg_manycore_print_int_responder.o

# object that should be compiled for debugging
DEBUG_OBJECTS  :=
#DEBUG_OBJECTS  += bsg_manycore_responder.o
#DEBUG_OBJECTS  += bsg_manycore_uart_responder.o

# objects that should be compiled with strict compilation flags
STRICT_OBJECTS :=
STRICT_OBJECTS += bsg_manycore_responder.o
STRICT_OBJECTS += bsg_manycore_loader.o
STRICT_OBJECTS += bsg_manycore_packet_id.o
STRICT_OBJECTS += bsg_manycore_eva.o
STRICT_OBJECTS += bsg_manycore_origin_eva_map.o
STRICT_OBJECTS += bsg_manycore_print_int_responder.o

$(STRICT_OBJECTS): CXXFLAGS += -Wall -Werror
$(STRICT_OBJECTS): CXXFLAGS += -Wno-unused-variable
$(STRICT_OBJECTS): CXXFLAGS += -Wno-unused-function
$(STRICT_OBJECTS): CXXFLAGS += -Wno-unused-but-set-variable

$(DEBUG_OBJECTS):  CXXFLAGS += -DDEBUG

# This script builds and installs the BSG Manycore Runtime API
build: $(OBJECTS)
	$(CXX) $(CFLAGS) -shared -Wl,-soname,libbsg_manycore_runtime.so.1 -o libbsg_manycore_runtime.so.1.0 $(OBJECTS) $(LDFLAGS)

install: build
	mv libbsg_manycore_runtime.so.1.0 /usr/lib64/
	ln -sf /usr/lib64/libbsg_manycore_runtime.so.1.0 /usr/lib64/libbsg_manycore_runtime.so.1
	ln -sf /usr/lib64/libbsg_manycore_runtime.so.1.0 /usr/lib64/libbsg_manycore_runtime.so
	cp -t /usr/include $(HEADERS)

clean:
	rm -f *.o libbsg_manycore_runtime.*

uninstall: clean
	sudo rm -f /usr/lib64/libbsg_manycore_* /usr/include/bsg_manycore*.h

.PHONY: build install clean uninstall
