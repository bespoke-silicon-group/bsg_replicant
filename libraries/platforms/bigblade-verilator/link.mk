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

# VHEADERS must be compiled before VSOURCES.
VDEFINES += BSG_MACHINE_GLOBAL_X=$(BSG_MACHINE_GLOBAL_X)
VDEFINES += BSG_MACHINE_GLOBAL_Y=$(BSG_MACHINE_GLOBAL_Y)
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

DIRS  = $(foreach t,exec profile threaded debug pc-histogram,$(BSG_MACHINExPLATFORM_PATH)/$t)
FRAGS = $(foreach d,$(DIRS),$d/V$(BSG_DESIGN_TOP).mk)
SIMOS = $(foreach d,$(DIRS),$d/bsg_manycore_simulator.o)
LIBS  = $(foreach d,$(DIRS),$d/V$(BSG_DESIGN_TOP)__ALL.a)
SIMSCS = $(foreach d,$(DIRS),$d/simsc)

# Generic Verilator source files that are compiiled into libmachine.so
VERILATOR_SRCS := verilated.cpp verilated_dpi.cpp
THREADED_SRCS  := $(VERILATOR_SRCS) verilated_threads.cpp
WAVEFORM_SRCS  := verilated_fst_c.cpp $(VERILATOR_SRCS) 

THREADED_OBJS = $(foreach o,$(THREADED_SRCS:.cpp=.o),$(BSG_MACHINExPLATFORM_PATH)/threaded/$o)
VERILATOR_OBJS = $(foreach o,$(VERILATOR_SRCS:.cpp=.o),$(BSG_MACHINExPLATFORM_PATH)/$o)
WAVEFORM_OBJS = $(foreach o,$(WAVEFORM_SRCS:.cpp=.o),$(BSG_MACHINExPLATFORM_PATH)/debug/$o)

$(WAVEFORM_OBJS) $(THREADED_OBJS) $(VERILATOR_OBJS): DEFINES := -DVL_PRINTF=printf
$(WAVEFORM_OBJS) $(THREADED_OBJS) $(VERILATOR_OBJS): DEFINES += -DVM_SC=0
$(WAVEFORM_OBJS) $(THREADED_OBJS) $(VERILATOR_OBJS): DEFINES += -DVM_TRACE=0
$(THREADED_OBJS): DEFINES += -DVL_THREADED=1
$(WAVEFORM_OBJS) $(THREADED_OBJS) $(VERILATOR_OBJS): INCLUDES := -I$(VERILATOR_ROOT)/include
$(WAVEFORM_OBJS) $(THREADED_OBJS) $(VERILATOR_OBJS): INCLUDES += -I$(VERILATOR_ROOT)/include/vltstd
$(WAVEFORM_OBJS) $(THREADED_OBJS) $(VERILATOR_OBJS): INCLUDES += -I$(LIBRARIES_PATH)/platforms/common/dpi
$(WAVEFORM_OBJS) $(THREADED_OBJS) $(VERILATOR_OBJS): CFLAGS    := -std=c11 -fPIC $(INCLUDES) $(DEFINES)
$(WAVEFORM_OBJS) $(THREADED_OBJS) $(VERILATOR_OBJS): CXXFLAGS  := -std=c++11 -fPIC $(INCLUDES) $(DEFINES)
# Uncomment to enable Verilator profiling with operf
# $(THREADED_OBJS) $(VERILATOR_OBJS): CFLAGS    += -g -pg
# $(THREADED_OBJS) $(VERILATOR_OBJS): CXXFLAGS  += -g -pg
$(THREADED_OBJS): $(BSG_MACHINExPLATFORM_PATH)/threaded/%.o : $(VERILATOR_ROOT)/include/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^
$(VERILATOR_OBJS): $(BSG_MACHINExPLATFORM_PATH)/%.o : $(VERILATOR_ROOT)/include/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^
$(WAVEFORM_OBJS): $(BSG_MACHINExPLATFORM_PATH)/debug/%.o : $(VERILATOR_ROOT)/include/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

# Build directory rule
$(DIRS):
	mkdir -p $@

# Verilation Rules.
# Disable profiling to turbocharge the exec binary
$(BSG_MACHINExPLATFORM_PATH)/exec/V$(BSG_DESIGN_TOP).mk: VDEFINES += BSG_MACHINE_DISABLE_VCORE_PROFILING
$(BSG_MACHINExPLATFORM_PATH)/exec/V$(BSG_DESIGN_TOP).mk: VDEFINES += BSG_MACHINE_DISABLE_CACHE_PROFILING
$(BSG_MACHINExPLATFORM_PATH)/exec/V$(BSG_DESIGN_TOP).mk: VDEFINES += BSG_MACHINE_DISABLE_ROUTER_PROFILING
$(BSG_MACHINExPLATFORM_PATH)/exec/V$(BSG_DESIGN_TOP).mk: VDEFINES += BSG_MACHINE_DISABLE_PC_HISTOGRAM

# See comment in bsg_nonsynth_manycore_testbench.v
$(BSG_MACHINExPLATFORM_PATH)/exec/V$(BSG_DESIGN_TOP).mk: VDEFINES += VERILATOR_WORKAROUND_DISABLE_VCORE_TRACE
$(BSG_MACHINExPLATFORM_PATH)/exec/V$(BSG_DESIGN_TOP).mk: VDEFINES += VERILATOR_WORKAROUND_DISABLE_VCORE_COVERAGE
$(BSG_MACHINExPLATFORM_PATH)/exec/V$(BSG_DESIGN_TOP).mk: VDEFINES += VERILATOR_WORKAROUND_DISABLE_VCACHE_PROFILING
$(BSG_MACHINExPLATFORM_PATH)/exec/V$(BSG_DESIGN_TOP).mk: VDEFINES += VERILATOR_WORKAROUND_DISABLE_VCORE_PROFILING
$(BSG_MACHINExPLATFORM_PATH)/exec/V$(BSG_DESIGN_TOP).mk: VDEFINES += VERILATOR_WORKAROUND_DISABLE_ROUTER_PROFILER
$(BSG_MACHINExPLATFORM_PATH)/exec/V$(BSG_DESIGN_TOP).mk: VDEFINES += VERILATOR_WORKAROUND_DISABLE_PC_HISTOGRAM

# Disable profiling to turbocharge the exec binary
$(BSG_MACHINExPLATFORM_PATH)/pc-histogram/V$(BSG_DESIGN_TOP).mk: VDEFINES += BSG_MACHINE_DISABLE_VCORE_PROFILING
$(BSG_MACHINExPLATFORM_PATH)/pc-histogram/V$(BSG_DESIGN_TOP).mk: VDEFINES += BSG_MACHINE_DISABLE_CACHE_PROFILING
$(BSG_MACHINExPLATFORM_PATH)/pc-histogram/V$(BSG_DESIGN_TOP).mk: VDEFINES += BSG_MACHINE_DISABLE_ROUTER_PROFILING

# See comment in bsg_nonsynth_manycore_testbench.v
$(BSG_MACHINExPLATFORM_PATH)/pc-histogram/V$(BSG_DESIGN_TOP).mk: VDEFINES += VERILATOR_WORKAROUND_DISABLE_VCORE_TRACE
$(BSG_MACHINExPLATFORM_PATH)/pc-histogram/V$(BSG_DESIGN_TOP).mk: VDEFINES += VERILATOR_WORKAROUND_DISABLE_VCORE_COVERAGE
$(BSG_MACHINExPLATFORM_PATH)/pc-histogram/V$(BSG_DESIGN_TOP).mk: VDEFINES += VERILATOR_WORKAROUND_DISABLE_VCACHE_PROFILING
$(BSG_MACHINExPLATFORM_PATH)/pc-histogram/V$(BSG_DESIGN_TOP).mk: VDEFINES += VERILATOR_WORKAROUND_DISABLE_VCORE_PROFILING
$(BSG_MACHINExPLATFORM_PATH)/pc-histogram/V$(BSG_DESIGN_TOP).mk: VDEFINES += VERILATOR_WORKAROUND_DISABLE_ROUTER_PROFILER

# Defines to generate waveforms. These are specific to Verilator.
$(BSG_MACHINExPLATFORM_PATH)/debug/V$(BSG_DESIGN_TOP).mk: VERILATOR_VFLAGS += --trace-fst --trace-structs
$(BSG_MACHINExPLATFORM_PATH)/debug/V$(BSG_DESIGN_TOP).mk: VDEFINES += BSG_VERILATOR_WAVEFORM

$(BSG_MACHINExPLATFORM_PATH)/threaded/V$(BSG_DESIGN_TOP).mk: VERILATOR_VFLAGS += --threads 4
$(BSG_MACHINExPLATFORM_PATH)/threaded/V$(BSG_DESIGN_TOP).mk: VDEFINES += BSG_MACHINE_DISABLE_VCORE_PROFILING
$(BSG_MACHINExPLATFORM_PATH)/threaded/V$(BSG_DESIGN_TOP).mk: VDEFINES += BSG_MACHINE_DISABLE_CACHE_PROFILING
$(BSG_MACHINExPLATFORM_PATH)/threaded/V$(BSG_DESIGN_TOP).mk: VDEFINES += BSG_MACHINE_DISABLE_ROUTER_PROFILING
$(BSG_MACHINExPLATFORM_PATH)/threaded/V$(BSG_DESIGN_TOP).mk: VDEFINES += BSG_MACHINE_DISABLE_PC_HISTOGRAM

# See comment in bsg_nonsynth_manycore_testbench.v
$(BSG_MACHINExPLATFORM_PATH)/threaded/V$(BSG_DESIGN_TOP).mk: VDEFINES += VERILATOR_WORKAROUND_DISABLE_VCORE_TRACE
$(BSG_MACHINExPLATFORM_PATH)/threaded/V$(BSG_DESIGN_TOP).mk: VDEFINES += VERILATOR_WORKAROUND_DISABLE_VCORE_COVERAGE
$(BSG_MACHINExPLATFORM_PATH)/threaded/V$(BSG_DESIGN_TOP).mk: VDEFINES += VERILATOR_WORKAROUND_DISABLE_VCACHE_PROFILING
$(BSG_MACHINExPLATFORM_PATH)/threaded/V$(BSG_DESIGN_TOP).mk: VDEFINES += VERILATOR_WORKAROUND_DISABLE_VCORE_PROFILING
$(BSG_MACHINExPLATFORM_PATH)/threaded/V$(BSG_DESIGN_TOP).mk: VDEFINES += VERILATOR_WORKAROUND_DISABLE_ROUTER_PROFILER
$(BSG_MACHINExPLATFORM_PATH)/threaded/V$(BSG_DESIGN_TOP).mk: VDEFINES += VERILATOR_WORKAROUND_DISABLE_PC_HISTOGRAM

# TODO: A target for C/C++ profiling (to diagnose where time is being
# spent) will be difficult It is better just to go find all the
# annotations where I said "Uncomment to enable Verilator profiling
# with operf" Uncomment to enable Verilator profiling with operf
$(BSG_MACHINExPLATFORM_PATH)/operf/V$(BSG_DESIGN_TOP).mk: VERILATOR_VFLAGS += --prof-cfuncs

# TODO: Don't like pattern matching. Better way?
$(FRAGS): %/V$(BSG_DESIGN_TOP).mk : | %
$(FRAGS): $(VHEADERS) $(VSOURCES)
	$(info BSG_INFO: Running verilator)
	@$(VERILATOR) -Mdir $(dir $@) --cc $(VERILATOR_CFLAGS) $(VERILATOR_VFLAGS) $^ --top-module $(BSG_DESIGN_TOP)

# Static library build rules
$(LIBS): %/V$(BSG_DESIGN_TOP)__ALL.a : %/V$(BSG_DESIGN_TOP).mk
	$(MAKE) OPT_FAST="-O2 -march=native" -C $(dir $@) -f $(notdir $<) default

# bsg_manycore_simulator.cpp is the interface between
# libbsg_manycore_runtime.so and libmachine.so that hides the
# implementation of the Vmanycore_tb_top and allows
# libbsg_manycore_runtime to be compiled independently from the
# machine.
$(BSG_MACHINExPLATFORM_PATH)/threaded/bsg_manycore_simulator.o: DEFINES  += -DVL_THREADED
$(BSG_MACHINExPLATFORM_PATH)/debug/bsg_manycore_simulator.o: DEFINES  += -DBSG_VERILATOR_WAVEFORM
$(SIMOS): INCLUDES := -I$(BSG_PLATFORM_PATH)
$(SIMOS): INCLUDES := -I$(LIBRARIES_PATH)
$(SIMOS): INCLUDES += -I$(BSG_MACHINE_PATH)/notrace
$(SIMOS): INCLUDES += -I$(BASEJUMP_STL_DIR)/bsg_test
$(SIMOS): INCLUDES += -I$(VERILATOR_ROOT)/include
$(SIMOS): INCLUDES += -I$(VERILATOR_ROOT)/include/vltstd
$(SIMOS): INCLUDES += -I$(LIBRARIES_PATH)/platforms/common/dpi
$(SIMOS): CXXFLAGS := -std=c++11 -fPIC $(INCLUDES) $(DEFINES)
# TODO: Don't like pattern matching. Better way?
$(SIMOS): %/bsg_manycore_simulator.o : %/V$(BSG_DESIGN_TOP)__ALL.a
$(SIMOS): $(BSG_PLATFORM_PATH)/bsg_manycore_simulator.cpp 
	$(CXX) -c $(CXXFLAGS) -I$(dir $@) $^ -o $@ 


# simsc binary build rules
$(BSG_MACHINExPLATFORM_PATH)/threaded/simsc: $(THREADED_OBJS)
$(BSG_MACHINExPLATFORM_PATH)/exec/simsc: $(VERILATOR_OBJS)
$(BSG_MACHINExPLATFORM_PATH)/debug/simsc: $(WAVEFORM_OBJS)
$(BSG_MACHINExPLATFORM_PATH)/profile/simsc: $(VERILATOR_OBJS)
$(BSG_MACHINExPLATFORM_PATH)/pc-histogram/simsc: $(VERILATOR_OBJS)

$(SIMSCS): LD = $(CXX)
$(SIMSCS): LDFLAGS  = -L$(BSG_PLATFORM_PATH) -Wl,-rpath=$(BSG_PLATFORM_PATH) -lbsg_manycore_regression -lbsg_manycore_runtime
$(SIMSCS): LDFLAGS += -L$(LIBRARIES_PATH)/features/dma/simulation -Wl,-rpath=$(LIBRARIES_PATH)/features/dma/simulation -ldramsim3 -ldmamem -ltracer
$(SIMSCS): LDFLAGS += -L$(LIBRARIES_PATH)/features/tracer/simulation -Wl,-rpath=$(LIBRARIES_PATH)/features/tracer/simulation -ltracer
$(SIMSCS): LDFLAGS += -L$(LIBRARIES_PATH)/features/pc_histogram/simulation -Wl,-rpath=$(LIBRARIES_PATH)/features/pc_histogram/simulation -lpc_histogram
$(SIMSCS): LDFLAGS += -lm
$(SIMSCS): LDFLAGS += -lz
$(SIMSCS): LDFLAGS += -ldl
$(SIMSCS): LDFLAGS += -lpthread
$(SIMSCS): $(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so
$(SIMSCS): $(BSG_PLATFORM_PATH)/libbsgmc_cuda_legacy_pod_repl.so
$(SIMSCS): $(BSG_PLATFORM_PATH)/libbsg_manycore_regression.so
$(SIMSCS): $(LIBRARIES_PATH)/features/dma/simulation/libdramsim3.so
$(SIMSCS): $(LIBRARIES_PATH)/features/dma/simulation/libdmamem.so
$(SIMSCS): $(LIBRARIES_PATH)/features/tracer/simulation/libtracer.so
$(SIMSCS): $(LIBRARIES_PATH)/features/pc_histogram/simulation/libpc_histogram.so
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
REGRESSION_PREBUILD += $(BSG_MACHINExPLATFORM_PATH)/pc-histogram/simsc
REGRESSION_PREBUILD += $(BSG_PLATFORM_PATH)/libbsgmc_cuda_legacy_pod_repl.so
REGRESSION_PREBUILD += $(BSG_PLATFORM_PATH)/libbsg_manycore_runtime.so
REGRESSION_PREBUILD += $(BSG_PLATFORM_PATH)/libbsg_manycore_regression.so

.PHONY: platform.link.clean
platform.link.clean:
	rm -rf $(BSG_MACHINE_PATH)/libmachine.so
	rm -rf $(BSG_MACHINE_PATH)/$(BSG_PLATFORM)/
	rm -rf *.debug *.profile *.saifgen *.exec

link.clean: platform.link.clean ;

