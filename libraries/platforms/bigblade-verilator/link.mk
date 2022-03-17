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

# Debugging (in case of segfault, break glass)
# VERILATOR_VFLAGS += --debug --gdbbt

DIRS  = $(foreach t,exec profile,$(BSG_MACHINExPLATFORM_PATH)/$t)
FRAGS = $(foreach d,$(DIRS),$d/V$(BSG_DESIGN_TOP).mk)
SIMOS = $(foreach d,$(DIRS),$d/bsg_manycore_simulator.o)
LIBS  = $(foreach d,$(DIRS),$d/V$(BSG_DESIGN_TOP)__ALL.a)
SIMSCS = $(foreach d,$(DIRS),$d/simsc)

# Build directory rule
$(DIRS):
	mkdir -p $@

# Verilation Rules.
# Disable profiling to turbocharge the exec binary
$(BSG_MACHINExPLATFORM_PATH)/exec/V$(BSG_DESIGN_TOP).mk: VDEFINES += BSG_MACHINE_DISABLE_VCORE_PROFILING
$(BSG_MACHINExPLATFORM_PATH)/exec/V$(BSG_DESIGN_TOP).mk: VDEFINES += BSG_MACHINE_DISABLE_CACHE_PROFILING
$(BSG_MACHINExPLATFORM_PATH)/exec/V$(BSG_DESIGN_TOP).mk: VDEFINES += BSG_MACHINE_DISABLE_ROUTER_PROFILING

# TODO: To enable waveform tracing, a new macro needs to be defined to
# uncomment the lines in bsg_manycore_simulator.cpp, and the
# $dumpfile/dumpvars calls in dpi_top.sv
# These enable verilator waveforms
$(BSG_MACHINExPLATFORM_PATH)/debug/V$(BSG_DESIGN_TOP).mk: VERILATOR_VFLAGS += --trace --trace-structs

# TODO: Threaded options will require per-configuration compilation of
# the LIBMACHINE objects, which is a mess -- but doable -- and
# compilation of verilated_threads.cpp
$(BSG_MACHINExPLATFORM_PATH)/threaded/V$(BSG_DESIGN_TOP).mk: VERILATOR_VFLAGS += --threads 4

# TODO: A target for C/C++ profiling (to diagnose where time is being
# spent) will be difficult It is better just to go find all the
# annotations where I said "Uncomment to enable Verilator profiling
# with operf" Uncomment to enable Verilator profiling with operf
$(BSG_MACHINExPLATFORM_PATH)/operf/V$(BSG_DESIGN_TOP).mk: VERILATOR_VFLAGS += --prof-cfuncs

# TODO: Don't like pattern matching. Better way?
$(FRAGS): %/V$(BSG_DESIGN_TOP).mk : | %
$(FRAGS): $(VHEADERS) $(VSOURCES)
	$(info BSG_INFO: Running verilator)
	$(VERILATOR) -Mdir $(dir $@) --cc $(VERILATOR_CFLAGS) $(VERILATOR_VFLAGS) $^ --top-module $(BSG_DESIGN_TOP)

# Static library build rules
$(LIBS): %/V$(BSG_DESIGN_TOP)__ALL.a : %/V$(BSG_DESIGN_TOP).mk
	$(MAKE) OPT_FAST="-O2 -march=native" -C $(dir $@) -f $(notdir $<) default

# bsg_manycore_simulator.cpp is the interface between
# libbsg_manycore_runtime.so and libmachine.so that hides the
# implementation of the Vmanycore_tb_top and allows
# libbsg_manycore_runtime to be compiled independently from the
# machine.
# $(BSG_MACHINExPLATFORM_PATH/threaded/bsg_manycore_simulator.o: DEFINES  += -DVL_THREADED
$(SIMOS): INCLUDES := -I$(BSG_PLATFORM_PATH)
$(SIMOS): INCLUDES += -I$(BSG_MACHINE_PATH)/notrace
$(SIMOS): INCLUDES += -I$(BASEJUMP_STL_DIR)/bsg_test
$(SIMOS): INCLUDES += -I$(VERILATOR_ROOT)/include
$(SIMOS): INCLUDES += -I$(VERILATOR_ROOT)/include/vltstd
$(SIMOS): CXXFLAGS := -std=c++11 -fPIC $(INCLUDES)
# TODO: Don't like pattern matching. Better way?
$(SIMOS): %/bsg_manycore_simulator.o : %/V$(BSG_DESIGN_TOP)__ALL.a
$(SIMOS): $(BSG_PLATFORM_PATH)/bsg_manycore_simulator.cpp
	$(CXX) -c $(CXXFLAGS) -I$(dir $@) $^ -o $@ 


# NOTE: This approach is deprecated in favor of a static library that improves verilator perf.
# libmachine.so is the machine-specific shared object file that implements the
# Verilator simulation. It is generated by compiling the archive file (above),
# verilator C++ files, and the Machine/Manycore interface
# bsg_manycore_verilator.o. It is linked with libbsg_manycore_runtime.so at
# executable compile time.
# $(BSG_MACHINE_PATH)/libmachine.so: LD = $(CXX)
# $(BSG_MACHINE_PATH)/libmachine.so: $(LIBMACHINE_OBJECTS)
# $(BSG_MACHINE_PATH)/libmachine.so: $(BSG_PLATFORM_PATH)/bsg_manycore_simulator.o
# $(BSG_MACHINE_PATH)/libmachine.so: $(BSG_MACHINE_PATH)/obj_dir/V$(BSG_DESIGN_TOP)__ALL.a
# 	$(LD) -shared -Wl,--whole-archive,-soname,$@ -o $@ $^ -Wl,--no-whole-archive

$(SIMSCS): LD = $(CXX)
$(SIMSCS): LDFLAGS  = -L$(BSG_PLATFORM_PATH) -Wl,-rpath=$(BSG_PLATFORM_PATH) -lbsg_manycore_regression -lbsg_manycore_runtime
$(SIMSCS): LDFLAGS += -L$(LIBRARIES_PATH)/features/dma/simulation -Wl,-rpath=$(LIBRARIES_PATH)/features/dma/simulation -ldramsim3 -ldmamem
$(SIMSCS): LDFLAGS += -lm
$(SIMSCS): LDFLAGS += -ldl
$(SIMSCS): LDFLAGS += -lpthread
$(SIMSCS): $(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so
$(SIMSCS): $(BSG_PLATFORM_PATH)/libbsgmc_cuda_legacy_pod_repl.so
$(SIMSCS): $(BSG_PLATFORM_PATH)/libbsg_manycore_regression.so
$(SIMSCS): $(LIBRARIES_PATH)/features/dma/simulation/libdramsim3.so
$(SIMSCS): $(LIBRARIES_PATH)/features/dma/simulation/libdmamem.so
$(SIMSCS): $(LIBMACHINE_OBJECTS) 
# TODO: Don't like pattern matching. Better way?
$(SIMSCS): %/simsc : %/bsg_manycore_simulator.o %/V$(BSG_DESIGN_TOP)__ALL.a
	$(LD) -o $@ $(LDFLAGS) $^

.PRECIOUS: $(SIMSCS)

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
	rm -rf *.debug *.profile *.saifgen *.exec

link.clean: platform.link.clean ;

