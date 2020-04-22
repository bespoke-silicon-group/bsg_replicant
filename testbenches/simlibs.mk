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

# This Makefile fragment is for building hardware and software libraries for
# cosimulation
ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

# This file REQUIRES several variables to be set. They are typically
# set by the Makefile that includes this makefile..
# 

# TESTBENCH_PATH: The path to the testbenches folder in BSG F1
ifndef TESTBENCH_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: TESTBENCH_PATH is not defined$(NC)"))
endif

# LIBRARIES_PATH: The path to the regression folder in BSG F1
ifndef LIBRARIES_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: LIBRARIES_PATH is not defined$(NC)"))
endif

# HARDWARE_PATH: The path to the hardware folder in BSG F1
ifndef HARDWARE_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: HARDWARE_PATH is not defined$(NC)"))
endif

# PROJECT: The project name, used to as the work directory of the hardware
# library during analysis
ifndef PROJECT
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: PROJECT is not defined$(NC)"))
endif

# The following variables are set by $(BSG_F1_DIR)/hdk.mk, which will fail if
# hdk_setup.sh has not been run, or environment.mk is not included
#
# HDK_SHELL_DESIGN_DIR: Path to the directory containing all the AWS "shell" IP
# AWS_FPGA_REPO_DIR: Path to the clone of the aws-fpga repo
# HDK_COMMON_DIR: Path to HDK 'common' directory w/ libraries for cosimluation.
# SDK_DIR: Path to the SDK directory in the aws-fpga repo
include $(BSG_F1_DIR)/hdk.mk

# libraries.mk defines targets for the BSG Manycore Runtime library.
include $(LIBRARIES_PATH)/libraries.mk

# $(HARDWARE_PATH)/hardware.mk adds to VSOURCES which is a list of verilog
# source files for cosimulation and compilation, and VHEADERS, which is similar,
# but for header files. It also adds to CLEANS, a list of clean rules for
# cleaning hardware targets.
include $(HARDWARE_PATH)/hardware.mk

# BSG_MACHINE_NAME: The name of the target machine. Should be defined
# in $(BSG_MACHINE_PATH)/Makefile.machine.include, which is included
# by hardware.mk
ifndef BSG_MACHINE_NAME
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_MACHINE_NAME is not defined$(NC)"))
endif

# Include makefile for dramsim3 sources
include $(TESTBENCH_PATH)/dramsim3.mk
include $(TESTBENCH_PATH)/infmem.mk
include $(TESTBENCH_PATH)/libdmamem.mk

# The manycore architecture uses unsynthesizable sources for simulation

# machine_wrapper.sv is the top-level work library for each individual
# machine.
# VSOURCES += $(TESTBENCH_PATH)/machine_wrapper.sv

VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_nonsynth_mem_infinite.v

VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dpi_clock_gen.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_reset_gen.v

VSOURCES += $(BASEJUMP_STL_DIR)/bsg_misc/bsg_cycle_counter.v

# Test DRAM
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_mem/bsg_nonsynth_mem_1r1w_sync_dma.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_mem/bsg_nonsynth_mem_1r1w_sync_mask_write_byte_dma.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_mem/bsg_nonsynth_mem_1rw_sync_mask_write_byte_dma.v

VSOURCES += $(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_to_test_dram.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_to_test_dram_rx.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_to_test_dram_rx_reorder.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_to_test_dram_tx.v

VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dramsim3.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dramsim3_map.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dramsim3_unmap.v

# Profiling (These can't be used, yet)
# VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/instr_trace.v
# VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vanilla_core_trace.v
# VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_manycore_link_to_cache_tracer.v

# VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_manycore_profile_pkg.v
# VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vanilla_core_profiler.v
# VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vcache_profiler.v
# VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vcache_non_blocking_profiler.v
# VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/infinite_mem_profiler.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/nb_waw_detector.v

# DPI/IO Interface
VSOURCES += $(BSG_MANYCORE_DIR)/v/bsg_manycore_link_sif_async_buffer.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dpi_from_fifo.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dpi_to_fifo.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dpi_rom.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/dpi/bsg_nonsynth_dpi_manycore.v

VSOURCES += $(TESTBENCH_PATH)/verilator_top.sv

SIMLIBS += $(LIBRARIES_PATH)/libbsg_manycore_runtime.so
# SIMLIBS += $(BSG_MACHINE_PATH)/libmachine.so 
# Using the generic variables VSOURCES, VINCLUDES, and VDEFINES, we create
# tool-specific versions of the same variables. 
# So that we can limit tool-specific to a few specific spots we use VDEFINES,
# VINCLUDES, and VSOURCES to hold lists of macros, include directores, and
# verilog headers, and sources (respectively). These are used during simulation
# compilation, but transformed into a tool-specific syntax where necesssary.

VDEFINES   += BSG_MACHINE_NAME=$(BSG_MACHINE_NAME)
VDEFINES   += COSIM

# VHEADERS must be compiled before VSOURCES.
VLOGAN_SOURCES  += $(VHEADERS) $(VSOURCES) 
VLOGAN_INCLUDES += $(foreach inc,$(VINCLUDES),+incdir+"$(inc)")
VLOGAN_DEFINES  += $(foreach def,$(VDEFINES),+define+"$(def)")
VLOGAN_VFLAGS   += -ntb_opts tb_timescale=1ps/1ps -timescale=1ps/1ps
VLOGAN_VFLAGS   += -sverilog -v2005 +v2k
VLOGAN_VFLAGS   += +systemverilogext+.svh +systemverilogext+.sv
VLOGAN_VFLAGS   += +libext+.sv +libext+.v +libext+.vh +libext+.svh
VLOGAN_VFLAGS   += -full64 -lca -assert svaext
VLOGAN_VFLAGS   += -undef_vcs_macro

# The applications link against the BSG Manycore Libraries, and the FPGA
# Management libaries, so we build them as necessary. They do NOT need to be
# re-built every time a regression test is compiled

# libbsg_manycore_runtime will be compiled in $(LIBRARIES_PATH)
LDFLAGS    += -L$(LIBRARIES_PATH) -Wl,-rpath=$(LIBRARIES_PATH)

# Define the COSIM macro so that the DPI Versions of functions are called
$(LIB_OBJECTS): CXXFLAGS += -DCOSIM
$(LIB_OBJECTS): CFLAGS   += -DCOSIM
$(LIB_OBJECTS): CXXFLAGS += -DVERILATOR
$(LIB_OBJECTS): CFLAGS   += -DVERILATOR

# libfpga_mgmt will be compiled in $(TESTBENCH_PATH), so direct the linker there
$(LIB_OBJECTS): INCLUDES += -I$(VCS_HOME)/linux64/lib/
$(LIBRARIES_PATH)/libbsg_manycore_runtime.so.1.0: LDFLAGS +=-L$(TESTBENCH_PATH) 
$(LIBRARIES_PATH)/libbsg_manycore_runtime.so.1.0: LDFLAGS +=-Wl,-rpath=$(TESTBENCH_PATH)
$(LIBRARIES_PATH)/libbsg_manycore_runtime.so.1: %: %.0
	ln -sf $@.0 $@
$(LIBRARIES_PATH)/libbsg_manycore_runtime.so: %: %.1
	ln -sf $@.1 $@

BSG_DESIGN_TOP := manycore_tb_top
$(LIBRARIES_PATH)/bsg_manycore.cpp: % : $(BSG_MACHINE_PATH)/notrace/V$(BSG_DESIGN_TOP).mk

VERILATOR_INCLUDES += -I$(VERILATOR_ROOT)/include
VERILATOR_INCLUDES += -I$(VERILATOR_ROOT)/include/vltstd
VERILATOR_CXXSRCS := verilated.cpp verilated_vcd_c.cpp verilated_dpi.cpp
VERILATOR_CSRCS = 
VERILATOR_OBJS += $(VERILATOR_CXXSRCS:%.cpp=%.o)
VERILATOR_OBJS += $(VERILATOR_CSRCS:%.c=%.o)

# Verilator object files (added to libmachine.so)
VERILATOR_LIBMACHINE_OBJS = $(VERILATOR_OBJS:%.o=$(MACHINES_PATH)/%.o)
$(MACHINES_PATH)/%.o: $(VERILATOR_ROOT)/include/%.cpp
	g++ $(VERILATOR_INCLUDES) -DVL_PRINTF=printf -DVM_COVERAGE=0 -DVM_SC=0 -DVM_TRACE=0 -fPIC -std=c++11 -c -o $@ $^

VCFLAGS = -fPIC
VERILATOR_CFLAGS    += $(foreach vcf,$(VCFLAGS),-CFLAGS "$(vcf)")
VERILATOR_VINCLUDES += $(foreach inc,$(VINCLUDES),+incdir+"$(inc)")
VERILATOR_VDEFINES  += $(foreach def,$(VDEFINES),+define+"$(def)")
VERILATOR_VFLAGS = $(VERILATOR_VINCLUDES) $(VERILATOR_VDEFINES)
VERILATOR_VFLAGS += -Wno-lint -Wno-widthconcat -Wno-unoptflat
%/V$(BSG_DESIGN_TOP).mk: $(VHEADERS) $(VSOURCES) 
	$(info BSG_INFO: Running verilator)
	@$(VERILATOR) -Mdir $(dir $@) --cc $(VERILATOR_CFLAGS) $(VERILATOR_VFLAGS) $^ --top-module $(BSG_DESIGN_TOP)

%__ALL.a: %.mk
	$(MAKE) -j -C $(dir $@) -f $(notdir $<) default

%/libmachine.so: LD = $(CXX)
%/libmachine.so: %/notrace/V$(BSG_DESIGN_TOP)__ALL.a $(VERILATOR_LIBMACHINE_OBJS)
	$(LD) -shared -Wl,--whole-archive,-soname,$@ -o $@ $^ $(LDFLAGS) -Wl,--no-whole-archive

.PHONY: simlibs.clean
simlibs.clean: libraries.clean hardware.clean
	rm -rf $(BSG_MACHINE_PATH)/libmachine.so $(BSG_MACHINE_PATH)/{notrace,trace}
	rm -rf $(TESTBENCH_PATH)/vcs_simlibs/BSG_*
	rm -rf $(TESTBENCH_PATH)/vcs_simlibs/cosim_wrapper
	rm -rf $(TESTBENCH_PATH)/libfpga_mgmt.so
	rm -rf $(LIBRARIES_PATH)/libbsg_manycore_runtime.so
	rm -rf $(LIBRARIES_PATH)/libbsg_manycore_runtime.so.1

bleach:
	rm -rf $(TESTBENCH_PATH)/vcs_simlibs $(TESTBENCH_PATH)/synopsys_sim.setup

.PRECIOUS: %.mk %__ALL.a $(MACHINES_PATH)/%.o %/bsg_nonsynth_manycore_rom_dpi.vh %/libmachine.so
