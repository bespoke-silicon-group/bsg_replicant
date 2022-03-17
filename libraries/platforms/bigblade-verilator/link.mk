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
# bigblade-verilator binaries

ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

# This file REQUIRES several variables to be set. They are typically set by the
# Makefile that includes this fragment...

# BSG_PLATFORM_PATH: The path to the testbenches folder in BSG F1
ifndef BSG_PLATFORM_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_PLATFORM_PATH is not defined$(NC)"))
endif

# hardware.mk is the file list for the simulation RTL. It includes the
# platform specific hardware.mk file.
include $(HARDWARE_PATH)/hardware.mk

# libraries.mk defines how to build libbsg_manycore_runtime.so, which is
# pre-linked against all other simulation binaries.
include $(LIBRARIES_PATH)/libraries.mk

# Generic Verilator source files that are compiiled into libmachine.so
LIBMACHINE_CXXSRCS := verilated.cpp verilated_vcd_c.cpp verilated_dpi.cpp
# Uncomment for Verilator threads
# LIBMACHINE_CXXSRCS += verilated_threads.cpp
LIBMACHINE_OBJS += $(LIBMACHINE_CXXSRCS:%.cpp=%.o)
LIBMACHINE_OBJECTS = $(LIBMACHINE_OBJS:%.o=$(MACHINES_PATH)/%.o)

$(LIBMACHINE_OBJECTS): DEFINES := -DVL_PRINTF=printf
$(LIBMACHINE_OBJECTS): DEFINES += -DVM_SC=0
$(LIBMACHINE_OBJECTS): DEFINES += -DVM_TRACE=0
# Uncomment for Verilator threads
# $(LIBMACHINE_OBJECTS): DEFINES += -DVL_THREADED=1
$(LIBMACHINE_OBJECTS): INCLUDES := -I$(VERILATOR_ROOT)/include
$(LIBMACHINE_OBJECTS): INCLUDES += -I$(VERILATOR_ROOT)/include/vltstd
$(LIBMACHINE_OBJECTS): INCLUDES += -I$(BSG_MACHINE_PATH)/obj_dir

# Uncomment to enable Verilator profiling with operf
# $(LIBMACHINE_OBJECTS): CFLAGS    += -g -pg
# $(LIBMACHINE_OBJECTS): CXXFLAGS  += -g -pg
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
$(BSG_PLATFORM_PATH)/bsg_manycore_simulator.o: INCLUDES += -I$(BSG_MACHINE_PATH)/obj_dir
$(BSG_PLATFORM_PATH)/bsg_manycore_simulator.o: INCLUDES += -I$(BASEJUMP_STL_DIR)/bsg_test
$(BSG_PLATFORM_PATH)/bsg_manycore_simulator.o: INCLUDES += -I$(VERILATOR_ROOT)/include
$(BSG_PLATFORM_PATH)/bsg_manycore_simulator.o: INCLUDES += -I$(VERILATOR_ROOT)/include/vltstd
$(BSG_PLATFORM_PATH)/bsg_manycore_simulator.o: CXXFLAGS := -std=c++11 -fPIC $(INCLUDES)
$(BSG_PLATFORM_PATH)/bsg_manycore_simulator.o: $(BSG_MACHINE_PATH)/obj_dir/V$(BSG_DESIGN_TOP)__ALL.a
# Uncomment for Verilator threads
# $(BSG_PLATFORM_PATH)/bsg_manycore_simulator.o: DEFINES  += -DVL_THREADED

# VHEADERS must be compiled before VSOURCES.
VDEFINES += BSG_MACHINE_ORIGIN_X_CORD=$(BSG_MACHINE_ORIGIN_COORD_X)
VDEFINES += BSG_MACHINE_ORIGIN_Y_CORD=$(BSG_MACHINE_ORIGIN_COORD_Y)
VDEFINES += HOST_MODULE_PATH=replicant_tb_top
VDEFINES += BSG_MACHINE_DRAMSIM3_PKG=$(BSG_MACHINE_MEM_DRAMSIM3_PKG)
VDEFINES += BSG_NO_TIMESCALE
VCFLAGS = -fPIC -Wno-format-extra-args

# Uncomment to enable Verilator profiling with operf
# VERILATOR_CFLAGS += -g -pg
# VERILATOR_LDFLAGS += -g -pg

VERILATOR_CFLAGS    += $(foreach vcf,$(VCFLAGS),-CFLAGS "$(vcf)")
VERILATOR_VINCLUDES += $(foreach inc,$(VINCLUDES),+incdir+"$(inc)")
VERILATOR_VDEFINES  += $(foreach def,$(VDEFINES),+define+"$(def)")
VERILATOR_LDFLAGS += $(foreach vlf,$(LDFLAGS),-LDFLAGS "$(vlf)")
VERILATOR_VFLAGS = $(VERILATOR_VINCLUDES) $(VERILATOR_VDEFINES)
VERILATOR_VFLAGS += -Wno-widthconcat -Wno-unoptflat -Wno-lint
VERILATOR_VFLAGS += -Wno-MULTIDRIVEN # verilator no-lint doesn't work with this warning
VERILATOR_VFLAGS += --assert
# Uncomment to enable Verilator profiling with operf
# VERILATOR_VFLAGS += --prof-cfuncs

# Uncomment for Verilator threads
# VERILATOR_VFLAGS += --threads 4

# These enable verilator waveforms
# VERILATOR_VFLAGS += --trace --trace-structs

# Debugging (in case of segfault, break glass)
# VERILATOR_VFLAGS += --debug --gdbbt

# TODO: instead of obj_dir, what we could have multiple targets for
# each set of compilation flags, e.g. threaded, oprof, debug, etc.
# This would simplify the number of commented lines above.
$(BSG_MACHINE_PATH)/obj_dir/V$(BSG_DESIGN_TOP).mk: $(VHEADERS) $(VSOURCES)
	$(info BSG_INFO: Running verilator)
	@$(VERILATOR) -Mdir $(dir $@) --cc $(VERILATOR_CFLAGS) $(VERILATOR_VFLAGS) $^ --top-module $(BSG_DESIGN_TOP)

$(BSG_MACHINE_PATH)/obj_dir/V$(BSG_DESIGN_TOP)__ALL.a: $(BSG_MACHINE_PATH)/obj_dir/V$(BSG_DESIGN_TOP).mk
	$(MAKE) OPT_FAST="-O2 -march=native" -C $(dir $@) -f $(notdir $<) default

# libmachine.so is the machine-specific shared object file that implements the
# Verilator simulation. It is generated by compiling the archive file (above),
# verilator C++ files, and the Machine/Manycore interface
# bsg_manycore_verilator.o. It is linked with libbsg_manycore_runtime.so at
# executable compile time.
$(BSG_MACHINE_PATH)/libmachine.so: LD = $(CXX)
$(BSG_MACHINE_PATH)/libmachine.so: $(LIBMACHINE_OBJECTS)
$(BSG_MACHINE_PATH)/libmachine.so: $(BSG_PLATFORM_PATH)/bsg_manycore_simulator.o
$(BSG_MACHINE_PATH)/libmachine.so: $(BSG_MACHINE_PATH)/obj_dir/V$(BSG_DESIGN_TOP)__ALL.a
	$(LD) -shared -Wl,--whole-archive,-soname,$@ -o $@ $^ -Wl,--no-whole-archive

$(BSG_MACHINExPLATFORM_PATH)/exec $(BSG_MACHINExPLATFORM_PATH)/debug $(BSG_MACHINExPLATFORM_PATH)/saifgen $(BSG_MACHINExPLATFORM_PATH)/profile:
	mkdir -p $@

%/simsc: LD = $(CXX)
# libbsg_manycore_runtime will be compiled in $(BSG_PLATFORM_PATH)
%/simsc: LDFLAGS = -L$(BSG_PLATFORM_PATH) -Wl,-rpath=$(BSG_PLATFORM_PATH) -lbsg_manycore_regression -lbsg_manycore_runtime
%/simsc: LDFLAGS += -L$(LIBRARIES_PATH)/features/dma/simulation -Wl,-rpath=$(LIBRARIES_PATH)/features/dma/simulation -ldramsim3
%/simsc: LDFLAGS += -L$(LIBRARIES_PATH)/features/dma/simulation -Wl,-rpath=$(LIBRARIES_PATH)/features/dma/simulation -ldmamem
%/simsc: LDFLAGS += -lm
%/simsc: LDFLAGS += -ldl
%/simsc: LDFLAGS += -lpthread
# TODO: I don't like that we have to enumerate every single shared
# library dependency, when these are already dependencies for the
# runtime. Is there a better way?
%/simsc: $(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so $(BSG_PLATFORM_PATH)/libbsgmc_cuda_legacy_pod_repl.so $(BSG_PLATFORM_PATH)/libbsg_manycore_regression.so $(LIBRARIES_PATH)/features/dma/simulation/libdramsim3.so $(LIBRARIES_PATH)/features/dma/simulation/libdmamem.so  $(BSG_MACHINE_PATH)/obj_dir/V$(BSG_DESIGN_TOP)__ALL.a $(LIBMACHINE_OBJECTS) $(BSG_PLATFORM_PATH)/bsg_manycore_simulator.o | %
	$(LD) -o $@ $(LDFLAGS) $(filter %.a,$^) $(filter %.o,$^)

.PRECIOUS:$(BSG_MACHINExPLATFORM_PATH)/exec/simsc
.PRECIOUS:$(BSG_MACHINExPLATFORM_PATH)/debug/simsc
.PRECIOUS:$(BSG_MACHINExPLATFORM_PATH)/proile/simsc
.PRECIOUS:$(BSG_MACHINExPLATFORM_PATH)/saifgen/simsc

# When running recursive regression, make is launched in independent,
# non-communicating parallel processes that try to build these objects
# in parallel. That is no-bueno. We define REGRESSION_PREBUILD so that
# regression tests can build them before launching parallel
# compilation and execution
REGRESSION_PREBUILD += $(BSG_MACHINExPLATFORM_PATH)/exec/simsc
REGRESSION_PREBUILD += $(BSG_MACHINExPLATFORM_PATH)/debug/simsc
REGRESSION_PREBUILD += $(BSG_MACHINExPLATFORM_PATH)/profile/simsc
REGRESSION_PREBUILD += $(BSG_MACHINExPLATFORM_PATH)/saifgen/simsc
REGRESSION_PREBUILD += $(BSG_PLATFORM_PATH)/libbsgmc_cuda_legacy_pod_repl.so
REGRESSION_PREBUILD += $(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so
REGRESSION_PREBUILD += $(BSG_PLATFORM_PATH)/libbsg_manycore_regression.so

.PHONY: platform.link.clean
platform.link.clean:
	rm -rf $(BSG_MACHINE_PATH)/libmachine.so
	rm -rf $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/
	rm -rf $(BSG_MACHINE_PATH)/obj_dir
	rm -rf *.debug *.profile *.saifgen *.exec

link.clean: platform.link.clean ;

