# Copyright (c) 2020, University of Washington All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this list
# of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice, this
# list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.
#
# Neither the name of the copyright holder nor the names of its contributors may
# be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Include the machine configuration to make things clear(er)
include $(BSG_MACHINE_PATH)/Makefile.machine.include

# RISC-V tools
RV_CC = $(BLACKPARROT_SDK_DIR)/install/bin/riscv64-unknown-elf-dramfs-gcc
RV_CXX = $(BLACKPARROT_SDK_DIR)/install/bin/riscv64-unknown-elf-dramfs-g++
RV_AR = $(BLACKPARROT_SDK_DIR)/install/bin/riscv64-unknown-elf-dramfs-ar
RV_OBJDUMP = $(BLACKPARROT_SDK_DIR)/install/bin/riscv64-unknown-elf-dramfs-objdump

DROMAJO_DIR = $(BLACKPARROT_SDK_DIR)/dromajo

LIB_CXXSOURCES += $(LIBRARIES_PATH)/features/tracer/noimpl/bsg_manycore_tracer.cpp
LIB_CXXSOURCES += $(LIBRARIES_PATH)/features/profiler/noimpl/bsg_manycore_profiler.cpp
LIB_CXXSOURCES += $(BSG_PLATFORM_PATH)/bsg_manycore_platform.cpp

LIB_CSOURCES += $(BSG_PLATFORM_PATH)/software/src/flockfile.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/software/src/funlockfile.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/software/src/argp/argp-ba.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/software/src/argp/argp-eexst.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/software/src/argp/argp-fmtstream.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/software/src/argp/argp-fs-xinl.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/software/src/argp/argp-help.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/software/src/argp/argp-parse.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/software/src/argp/argp-pv.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/software/src/argp/argp-pvh.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/software/src/argp/argp-xinl.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/software/src/bp_utils.c
LIB_CSOURCES += $(BSG_PLATFORM_PATH)/software/src/args.c
LIB_CSOURCES += $(BLACKPARROT_SDK_DIR)/perch/bsg_newlib_intf.c
LIB_CSOURCES += $(BLACKPARROT_SDK_DIR)/perch/emulation.c

LIB_SSOURCES += $(BLACKPARROT_SDK_DIR)/perch/atomics.S
LIB_SSOURCES += $(BLACKPARROT_SDK_DIR)/perch/exception.S
LIB_SSOURCES += $(BLACKPARROT_SDK_DIR)/perch/muldiv.S
LIB_SSOURCES += $(BSG_PLATFORM_PATH)/software/src/crt0.S

# For now, use the noimpl version of the features
include $(LIBRARIES_PATH)/features/dma/noimpl/feature.mk

$(DMA_FEATURE_OBJECTS): INCLUDES += -I$(BSG_PLATFORM_PATH)/software/include
$(DMA_FEATURE_OBJECTS): CFLAGS += -march=rv64imafd -mcmodel=medany -mabi=lp64 -D_BSD_SOURCE -D_XOPEN_SOURCE=500
$(DMA_FEATURE_OBJECTS): CXXFLAGS += -march=rv64imafd -mcmodel=medany -mabi=lp64 -D_BSD_SOURCE -D_XOPEN_SOURCE=500

# Add the riscv-newlib specific includes for the library
LIB_PLATFORM_INCLUDES  = -I$(BSG_PLATFORM_PATH)
LIB_PLATFORM_INCLUDES += -I$(BSG_PLATFORM_PATH)/software/include
LIB_PLATFORM_INCLUDES += -I$(BLACKPARROT_SDK_DIR)/perch

$(LIB_OBJECTS) $(LIB_OBJECTS_CUDA_POD_REPL) $(LIB_OBJECTS_REGRESSION): CC = $(RV_CC)
$(LIB_OBJECTS) $(LIB_OBJECTS_CUDA_POD_REPL) $(LIB_OBJECTS_REGRESSION): CXX = $(RV_CXX)

$(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.a: AR = $(RV_AR)
$(BSG_PLATFORM_PATH)/libbsgmc_cuda_legacy_pod_repl.a: AR = $(RV_AR)
$(BSG_PLATFORM_PATH)/libbsg_manycore_regression.a: AR = $(RV_AR)

# Base addresses for various segments
# SM: For now let us assume the manycore code will fit into 16MB (I believe this is the requirement for the 
# .text section of the manycore binary; but let's assume even data fits into this limit). The Dromajo codebase
# has a hardcoded parameter that start addresses should be one of 00x80000000, 0x8000000000 or 0xC000000000.
# Another entry for 0x81000000 has also been added. Remapping to a different start address requires the creation
# of a custom bootrom for Dromajo. BlackParrot should not face any such issue because of the presence of a 
# base address register in the manycore which will get set to the correct base address during the NBF load stage.
# FIXME: Need a more elegant solution for relocating BlackParrot code to different pods or DRAM regions when
# running on Dromajo
BP_DRAM_BASE_ADDR := 0x81000000
HB_DRAM_BASE_ADDR := 0x80000000
TOP_OF_STACK_ADDR := 0x8F000000

# Make the litteFS file system
$(BSG_PLATFORM_PATH)/lfs.cpp: MKLFS = $(BLACKPARROT_SDK_DIR)/install/bin/dramfs_mklfs 128 64
$(BSG_PLATFORM_PATH)/lfs.cpp:
	$(MKLFS) > $@

$(BSG_PLATFORM_PATH)/lfs.o: $(BSG_PLATFORM_PATH)/lfs.cpp
	$(CXX) $^ -c -o $@ $(CXXFLAGS) $(INCLUDES)

# Include the compiled manycore binary if one exists
# FIXME: Some caveats with this target --> You have to use it with the *.log target
# In the future we either move away from linking the manycore binary at compile time
# or have a better compilation strategy (requires changes in the test infrastructure)
$(BSG_PLATFORM_PATH)/mcbin.o: $(BSG_MANYCORE_KERNELS)
$(BSG_PLATFORM_PATH)/mcbin.o: CFLAGS := -march=rv64imafd -mabi=lp64 -mcmodel=medany
$(BSG_PLATFORM_PATH)/mcbin.o: INCLUDES := -I$(shell pwd)
$(BSG_PLATFORM_PATH)/mcbin.o: CC := $(RV_CC)
$(BSG_PLATFORM_PATH)/mcbin.o:
	cp $(BSG_MANYCORE_KERNELS) manycore.riscv
	sed "s|BSG_MANYCORE_KERNELS|\"manycore.riscv\"|g" $(BSG_PLATFORM_PATH)/mcbin.S > genmcbin.S
	$(CC) -c -o $@ $(CFLAGS) $(DEFINES) $(INCLUDES) genmcbin.S

include $(LIBRARIES_PATH)/features/dma/simulation/dramsim3.mk
include $(LIBRARIES_PATH)/features/dma/simulation/libdmamem.mk

$(LIBRARIES_PATH)/features/dma/simulation/libdmamem.so: INCLUDES :=
$(LIBRARIES_PATH)/features/dma/simulation/libdramsim3.so: INCLUDES :=

DROMAJO_CXXSOURCES += $(DROMAJO_DIR)/src/cutils.cpp
DROMAJO_CXXSOURCES += $(DROMAJO_DIR)/src/dromajo_cosim.cpp
DROMAJO_CXXSOURCES += $(DROMAJO_DIR)/src/dromajo_main.cpp
DROMAJO_CXXSOURCES += $(DROMAJO_DIR)/src/dromajo_manycore.cpp
DROMAJO_CXXSOURCES += $(DROMAJO_DIR)/src/dw_apb_uart.cpp
DROMAJO_CXXSOURCES += $(DROMAJO_DIR)/src/elf64.cpp
DROMAJO_CXXSOURCES += $(DROMAJO_DIR)/src/fs.cpp
DROMAJO_CXXSOURCES += $(DROMAJO_DIR)/src/fs_disk.cpp
DROMAJO_CXXSOURCES += $(DROMAJO_DIR)/src/iomem.cpp
DROMAJO_CXXSOURCES += $(DROMAJO_DIR)/src/json.cpp
DROMAJO_CXXSOURCES += $(DROMAJO_DIR)/src/machine.cpp
DROMAJO_CXXSOURCES += $(DROMAJO_DIR)/src/pci.cpp
DROMAJO_CXXSOURCES += $(DROMAJO_DIR)/src/riscv_cpu.cpp
DROMAJO_CXXSOURCES += $(DROMAJO_DIR)/src/riscv_machine.cpp
DROMAJO_CXXSOURCES += $(DROMAJO_DIR)/src/softfp.cpp
DROMAJO_CXXSOURCES += $(DROMAJO_DIR)/src/virtio.cpp

DROMAJO_OBJECTS += $(patsubst %cpp,%o,$(DROMAJO_CXXSOURCES))

$(DROMAJO_OBJECTS): $(BSG_MACHINE_PATH)/Makefile.machine.include
$(DROMAJO_OBJECTS): INCLUDES := -I$(DROMAJO_DIR)/include
$(DROMAJO_OBJECTS): INCLUDES += -I$(BSG_PLATFORM_PATH)/software/include/dromajo
$(DROMAJO_OBJECTS): CXXFLAGS := -std=c++11 -fPIC -Wall -Wno-parentheses -MMD -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_GNU_SOURCE  -D__STDC_FORMAT_MACROS -DFIFO_MAX_ELEMENTS=$(BSG_MACHINE_IO_EP_CREDITS) -std=gnu++11
$(DROMAJO_OBJECTS): LDFLAGS := -fPIC
$(DROMAJO_OBJECTS): CXX = g++

PLATFORM_CXXSOURCES += $(BSG_PLATFORM_PATH)/bsg_manycore_simulator.cpp

PLATFORM_DEFINES  = -DHOST_X_COORD=$(BSG_MACHINE_HOST_COORD_X)
PLATFORM_DEFINES += -DHOST_Y_COORD=$(BSG_MACHINE_HOST_COORD_Y)
PLATFORM_DEFINES += -DBP_POD_X=0 -DBP_POD_Y=1
PLATFORM_DEFINES += -DNUM_DROMAJO_INSTR_PER_TICK=1000
PLATFORM_DEFINES += -DNUM_TX_FIFO_CHK_PER_TICK=100
PLATFORM_DEFINES += -DFIFO_MAX_ELEMENTS=$(BSG_MACHINE_IO_EP_CREDITS)
PLATFORM_DEFINES += -DBP_DRAM_BASE_ADDR=$(subst 0x,,$(BP_DRAM_BASE_ADDR))

PLATFORM_OBJECTS += $(patsubst %cpp,%o,$(PLATFORM_CXXSOURCES))
PLATFORM_OBJECTS += $(patsubst %c,%o,$(PLATFORM_CSOURCES))

$(PLATFORM_OBJECTS): $(BSG_MACHINE_PATH)/Makefile.machine.include
$(PLATFORM_OBJECTS): INCLUDES := -I$(LIBRARIES_PATH)
$(PLATFORM_OBJECTS): INCLUDES += -I$(EXAMPLES_PATH)
$(PLATFORM_OBJECTS): INCLUDES += -I$(BSG_MACHINE_PATH)/notrace/
$(PLATFORM_OBJECTS): INCLUDES += -I$(BSG_PLATFORM_PATH)
$(PLATFORM_OBJECTS): INCLUDES += -I$(VCS_HOME)/linux64/lib/
$(PLATFORM_OBJECTS): INCLUDES += -I$(BSG_MANYCORE_DIR)/testbenches/dpi/
$(PLATFORM_OBJECTS): INCLUDES += -I$(BASEJUMP_STL_DIR)/bsg_test/
$(PLATFORM_OBJECTS): INCLUDES += -I$(BLACKPARROT_SDK_DIR)/dromajo/include
$(PLATFORM_OBJECTS): CFLAGS   := -std=c11 -fPIC -DVCS -D_GNU_SOURCE -DVERILATOR -D_BSD_SOURCE -D_XOPEN_SOURCE=500 $(PLATFORM_DEFINES)
$(PLATFORM_OBJECTS): CXXFLAGS := -std=c++11 -fPIC -DVCS -D_GNU_SOURCE -DVERILATOR -D_BSD_SOURCE -D_XOPEN_SOURCE=500 $(PLATFORM_DEFINES)
$(PLATFORM_OBJECTS): LDFLAGS  := -fPIC
$(PLATFORM_OBJECTS): CXX = g++

$(BSG_PLATFORM_PATH)/libbsg_manycore_platform.so.1.0: $(DROMAJO_OBJECTS)
$(BSG_PLATFORM_PATH)/libbsg_manycore_platform.so.1.0: $(PLATFORM_OBJECTS)
$(BSG_PLATFORM_PATH)/libbsg_manycore_platform.so.1.0: $(LIBRARIES_PATH)/features/dma/simulation/libdmamem.so
$(BSG_PLATFORM_PATH)/libbsg_manycore_platform.so.1.0: $(LIBRARIES_PATH)/features/dma/simulation/libdramsim3.so
$(BSG_PLATFORM_PATH)/libbsg_manycore_platform.so.1.0: LDFLAGS := -fPIC
$(BSG_PLATFORM_PATH)/libbsg_manycore_platform.so.1.0: LDFLAGS += -L$(LIBRARIES_PATH)/features/dma/simulation -Wl,-rpath=$(LIBRARIES_PATH)/features/dma/simulation -ldmamem
$(BSG_PLATFORM_PATH)/libbsg_manycore_platform.so.1.0: LDFLAGS += -L$(LIBRARIES_PATH)/features/dma/simulation -Wl,-rpath=$(LIBRARIES_PATH)/features/dma/simulation -ldramsim3
$(BSG_PLATFORM_PATH)/libbsg_manycore_platform.so.1.0: LD = $(CXX)
$(BSG_PLATFORM_PATH)/libbsg_manycore_platform.so.1.0:
	$(LD) -shared -Wl,-soname,$(basename $(notdir $@)) -o $@ $(DROMAJO_OBJECTS) $(PLATFORM_OBJECTS) $(LDFLAGS)

$(BSG_PLATFORM_PATH)/libbsg_manycore_platform.so.1: %: %.0
	ln -sf $@.0 $@

$(BSG_PLATFORM_PATH)/libbsg_manycore_platform.so: %: %.1
	ln -sf $@.1 $@

platform.clean:
	rm -f $(PLATFORM_OBJECTS)
	rm -f $(DROMAJO_OBJECTS)
	rm -f $(BSG_PLATFORM_PATH)/lfs.cpp
	rm -f $(BSG_PLATFORM_PATH)/lfs.o
	rm -f $(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.a
	rm -f $(BSG_PLATFORM_PATH)/libbsgmc_cuda_legacy_pod_repl.a
	rm -f $(BSG_PLATFORM_PATH)/libbsg_manycore_regression.a
	rm -f $(BSG_PLATFORM_PATH)/libbsg_manycore_platform.so.1.0
	rm -f $(BSG_PLATFORM_PATH)/libbsg_manycore_platform.so.1
	rm -f $(BSG_PLATFORM_PATH)/libbsg_manycore_platform.so

libraries.clean: platform.clean
