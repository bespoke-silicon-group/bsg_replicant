# Copyright (c) 2019, University of Washington All rights reserved.
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

# This Makefile fragment defines all of the rules for linking
# aws-vcs binaries

ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

# This file REQUIRES several variables to be set. They are typically set by the
# Makefile that includes this fragment...

# BSG_PLATFORM_PATH: The path to the platform folder
ifndef BSG_PLATFORM_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_PLATFORM_PATH is not defined$(NC)"))
endif

# BSG_MACHINE_PATH: The path to the machines folder
ifndef BSG_MACHINE_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_MACHINE_PATH is not defined$(NC)"))
endif

# hardware.mk is the file list for the simulation RTL. It also defines
# BSG_DESIGN_TOP
include $(HARDWARE_PATH)/hardware.mk

# libraries.mk defines how to build libbsg_manycore_runtime.so, which is
# pre-linked against all other simulation binaries (libdramsim3.so, etc.)
include $(LIBRARIES_PATH)/libraries.mk



# Generic Verilator source files that are compiiled into libmachine.so
LIBMACHINE_CXXSRCS := verilated.cpp verilated_vcd_c.cpp verilated_dpi.cpp
LIBMACHINE_OBJS += $(LIBMACHINE_CXXSRCS:%.cpp=%.o)
LIBMACHINE_OBJECTS = $(LIBMACHINE_OBJS:%.o=$(MACHINES_PATH)/%.o)

$(LIBMACHINE_OBJECTS): DEFINES := -DVL_PRINTF=printf
$(LIBMACHINE_OBJECTS): DEFINES += -DVM_SC=0
$(LIBMACHINE_OBJECTS): DEFINES += -DVM_TRACE=0
$(LIBMACHINE_OBJECTS): INCLUDES := -I$(VERILATOR_ROOT)/include
$(LIBMACHINE_OBJECTS): INCLUDES += -I$(VERILATOR_ROOT)/include/vltstd
$(LIBMACHINE_OBJECTS): INCLUDES += -I$(BSG_MACHINE_PATH)

$(LIBMACHINE_OBJECTS): CFLAGS    += -std=c11 -fPIC $(INCLUDES) $(DEFINES)
$(LIBMACHINE_OBJECTS): CXXFLAGS  += -std=c++11 -fPIC $(INCLUDES) $(DEFINES)
$(LIBMACHINE_OBJECTS): $(MACHINES_PATH)/%.o: $(VERILATOR_ROOT)/include/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

# bsg_manycore_simulator.cpp is the interface between
# libbsg_manycore_runtime.so and libmachine.so that hides the
# implementation of the Vmanycore_tb_top and allows
# libbsg_manycore_runtime to be compiled independently from the
# machine.
$(BSG_PLATFORM_PATH)/bsg_manycore_simulator.o: INCLUDES := -I$(BSG_PLATFORM_PATH) 
$(BSG_PLATFORM_PATH)/bsg_manycore_simulator.o: INCLUDES += -I$(BSG_MACHINE_PATH)/notrace
$(BSG_PLATFORM_PATH)/bsg_manycore_simulator.o: INCLUDES += -I$(BSG_MACHINE_PATH)
$(BSG_PLATFORM_PATH)/bsg_manycore_simulator.o: INCLUDES += -I$(BASEJUMP_STL_DIR)/bsg_test
$(BSG_PLATFORM_PATH)/bsg_manycore_simulator.o: INCLUDES += -I$(VERILATOR_ROOT)/include
$(BSG_PLATFORM_PATH)/bsg_manycore_simulator.o: INCLUDES += -I$(VERILATOR_ROOT)/include/vltstd
$(BSG_PLATFORM_PATH)/bsg_manycore_simulator.o: CXXFLAGS := -std=c++11 -fPIC $(INCLUDES)
$(BSG_PLATFORM_PATH)/bsg_manycore_simulator.o: $(BSG_MACHINE_PATH)/V$(BSG_DESIGN_TOP)__ALL.a

# Verilator compilation is a little funky. Compiling the HDL generates
# a Makefile fragment, <top-level-module-name>.mk and transpiled C++
# files. This makefile fragment compiles the transpiled C++ into an
# archive file <top-level-module-name>_ALL.a.
VCFLAGS = -fPIC
VERILATOR_CFLAGS    += $(foreach vcf,$(VCFLAGS),-CFLAGS "$(vcf)")
VERILATOR_VINCLUDES += $(foreach inc,$(VINCLUDES),+incdir+"$(inc)")
VERILATOR_VDEFINES  += $(foreach def,$(VDEFINES),+define+"$(def)")
VERILATOR_VFLAGS = $(VERILATOR_VINCLUDES) $(VERILATOR_VDEFINES)
VERILATOR_VFLAGS += -Wno-widthconcat -Wno-unoptflat -Wno-lint
VERILATOR_VFLAGS += --assert
# These enable verilator tracing
# VERILATOR_VFLAGS += --trace --trace-structs
$(BSG_MACHINE_PATH)/V$(BSG_DESIGN_TOP).mk: $(VHEADERS) $(VSOURCES) 
	$(info BSG_INFO: Running verilator)
	@$(VERILATOR) -Mdir $(dir $@) --cc $(VERILATOR_CFLAGS) $(VERILATOR_VFLAGS) $^ --top-module $(BSG_DESIGN_TOP)

$(BSG_MACHINE_PATH)/V$(BSG_DESIGN_TOP)__ALL.a: $(BSG_MACHINE_PATH)/V$(BSG_DESIGN_TOP).mk
	$(MAKE) -j -C $(dir $@) -f $(notdir $<) default

# libmachine.so is the machine-specific shared object file that implements the
# Verilator simulation. It is generated by compiling the archive file (above),
# verilator C++ files, and the Machine/Manycore interface
# bsg_manycore_verilator.o. It is linked with libbsg_manycore_runtime.so at
# executable compile time.
$(BSG_MACHINE_PATH)/libmachine.so: LD = $(CXX)
$(BSG_MACHINE_PATH)/libmachine.so: $(LIBMACHINE_OBJECTS)
$(BSG_MACHINE_PATH)/libmachine.so: $(BSG_PLATFORM_PATH)/bsg_manycore_simulator.o
$(BSG_MACHINE_PATH)/libmachine.so: $(BSG_MACHINE_PATH)/V$(BSG_DESIGN_TOP)__ALL.a
	$(LD) -shared -Wl,--whole-archive,-soname,$@ -o $@ $^ -Wl,--no-whole-archive

# Executable compilation rules
LDFLAGS    += -lmachine -L$(BSG_MACHINE_PATH) -Wl,-rpath=$(BSG_MACHINE_PATH) 
LDFLAGS    += -lbsg_manycore_runtime -L$(BSG_PLATFORM_PATH) -Wl,-rpath=$(BSG_PLATFORM_PATH)
LDFLAGS    += -lm

INCLUDES   += -I$(LIBRARIES_PATH)
INCLUDES   += -I$(BSG_MACHINE_PATH)

%: %.o $(BSG_MACHINE_PATH)/libmachine.so $(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so
	g++ -std=c++11 $< -o $@ $(LDFLAGS)

# Remove the machine library files. 
machine.clean:
	rm -rf $(BSG_MACHINE_PATH)/libmachine.so
	rm -rf $(LIBMACHINE_OBJECTS)
	rm -rf $(BSG_MACHINE_PATH)/V$(BSG_DESIGN_TOP)*
	rm -rf $(BSG_MACHINE_PATH)/notrace
	rm -rf $(BSG_PLATFORM_PATH)/bsg_manycore_verilator.o

# Removing the verilator machine library should only be done when hardware is
# cleaned, because it takes a bit to compile.
hardware.clean: machine.clean

.PRECIOUS: %/V$(BSG_DESIGN_TOP).mk %__ALL.a $(LIBMACHINE_OBJECTS) $(BSG_MACHINE_PATH)/libmachine.so
.PHONY: machine.clean
