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

# The manycore architecture uses unsynthesizable sources for simulation

# machine_wrapper.sv is the top-level work library for each individual
# machine.
VSOURCES += $(TESTBENCH_PATH)/machine_wrapper.sv

VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_nonsynth_mem_infinite.v

VSOURCES += $(BASEJUMP_STL_DIR)/bsg_mem/bsg_nonsynth_mem_1rw_sync_mask_write_byte_assoc.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_mem/bsg_nonsynth_mem_1rw_sync_assoc.v

VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_clock_gen.v
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


# Profiling
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/instr_trace.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vanilla_core_trace.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_manycore_link_to_cache_tracer.v

VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_manycore_profile_pkg.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vanilla_core_profiler.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vcache_profiler.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vcache_non_blocking_profiler.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/infinite_mem_profiler.v

VSOURCES += $(BSG_MANYCORE_DIR)/v/bsg_manycore_link_sif_async_buffer.v

# Include makefile for dramsim3 sources
include $(TESTBENCH_PATH)/dramsim3.mk
include $(TESTBENCH_PATH)/infmem.mk
include $(TESTBENCH_PATH)/libdmamem.mk

SIMLIBS += $(TESTBENCH_PATH)/libfpga_mgmt.so
SIMLIBS += $(LIBRARIES_PATH)/libbsg_manycore_runtime.so
SIMLIBS += $(TESTBENCH_PATH)/vcs_simlibs/$(BSG_MACHINE_NAME)/AN.DB

# Using the generic variables VSOURCES, VINCLUDES, and VDEFINES, we create
# tool-specific versions of the same variables. 
# So that we can limit tool-specific to a few specific spots we use VDEFINES,
# VINCLUDES, and VSOURCES to hold lists of macros, include directores, and
# verilog headers, and sources (respectively). These are used during simulation
# compilation, but transformed into a tool-specific syntax where necesssary.
VINCLUDES += $(TESTBENCH_PATH)

VDEFINES   += BSG_MACHINE_NAME=$(BSG_MACHINE_NAME)
VDEFINES   += VCS_SIM
VDEFINES   += COSIM
VDEFINES   += DISABLE_VJTAG_DEBUG
VDEFINES   += ENABLE_PROTOCOL_CHK

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

# vcs_simlibs is generated by running Vivado. It contains all of the hardware
# Xilinx hardware library files necessary for simulation. This rule also
# generates synopsys_sim.setup, but it generates it in the run directory, so we
# cd to $(TESTBENCH_PATH) so that vcs_simlibs can be reused across invocations
# of cosimulation.
#
# This output is reused between regression test suites, so it should be removed
# with `make squeakyclean` or an equivalent
$(TESTBENCH_PATH)/synopsys_sim.setup: $(TESTBENCH_PATH)/gen_simlibs.tcl
	cd $(TESTBENCH_PATH) && vivado -mode batch -source $<

$(BSG_MACHINE_PATH)/synopsys_sim.setup: $(TESTBENCH_PATH)/synopsys_sim.setup
	cp $< $@
	echo "$(BSG_MACHINE_NAME) : $(TESTBENCH_PATH)/vcs_simlibs/$(BSG_MACHINE_NAME)/64" >> $@;

# We check synopsys_sim.setup for the library location before
# modifying it. This avoids repeating the line at the end of the file
# every single time.
$(TESTBENCH_PATH)/vcs_simlibs/$(BSG_MACHINE_NAME)/AN.DB: $(BSG_MACHINE_PATH)/synopsys_sim.setup $(VHEADERS) $(VSOURCES)
	cd $(TESTBENCH_PATH) && \
	XILINX_IP=$(XILINX_IP) \
	XILINX_VIVADO=$(XILINX_VIVADO) \
	HDK_COMMON_DIR=$(HDK_COMMON_DIR) \
	HDK_SHELL_DESIGN_DIR=$(HDK_SHELL_DESIGN_DIR) \
	HDK_SHELL_DIR=$(HDK_SHELL_DIR) \
	SYNOPSYS_SIM_SETUP=$(BSG_MACHINE_PATH)/synopsys_sim.setup \
	vlogan -work $(BSG_MACHINE_NAME) $(VLOGAN_VFLAGS) $(VLOGAN_DEFINES) \
		$(VLOGAN_SOURCES) -f $(TESTBENCH_PATH)/aws.vcs.f \
		$(VLOGAN_INCLUDES) -l $(BSG_MACHINE_NAME).vlogan.log

# The applications link against the BSG Manycore Libraries, and the FPGA
# Management libaries, so we build them as necessary. They do NOT need to be
# re-built every time a regression test is compiled

# libbsg_manycore_runtime will be compiled in $(LIBRARIES_PATH)
LDFLAGS    += -L$(LIBRARIES_PATH) -Wl,-rpath=$(LIBRARIES_PATH)

# Define the COSIM macro so that the DPI Versions of functions are called
$(LIB_OBJECTS): CXXFLAGS += -DCOSIM
$(LIB_OBJECTS): CFLAGS   += -DCOSIM

# libfpga_mgmt will be compiled in $(TESTBENCH_PATH), so direct the linker there
$(LIBRARIES_PATH)/libbsg_manycore_runtime.so.1.0: LDFLAGS +=-L$(TESTBENCH_PATH) 
$(LIBRARIES_PATH)/libbsg_manycore_runtime.so.1.0: LDFLAGS +=-Wl,-rpath=$(TESTBENCH_PATH)
$(LIBRARIES_PATH)/libbsg_manycore_runtime.so.1.0: $(TESTBENCH_PATH)/libfpga_mgmt.so
$(LIBRARIES_PATH)/libbsg_manycore_runtime.so.1: %: %.0
	ln -sf $@.0 $@
$(LIBRARIES_PATH)/libbsg_manycore_runtime.so: %: %.1
	ln -sf $@.1 $@

# libfpga_mgmt will be compiled in $(TESTBENCH_PATH)
$(TESTBENCH_PATH)/libfpga_mgmt.so: INCLUDES += -I$(SDK_DIR)/userspace/include
$(TESTBENCH_PATH)/libfpga_mgmt.so: INCLUDES += -I$(HDK_DIR)/common/software/include
$(TESTBENCH_PATH)/libfpga_mgmt.so: CFLAGS = -std=c11 -D_GNU_SOURCE -fPIC -shared
$(TESTBENCH_PATH)/libfpga_mgmt.so: % : $(SDK_DIR)/userspace/utils/sh_dpi_tasks.c
$(TESTBENCH_PATH)/libfpga_mgmt.so: % : $(HDK_DIR)/common/software/src/fpga_pci_sv.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -Wl,-soname,$(notdir $@) -o $@

.PHONY: simlibs.clean
simlibs.clean: libraries.clean hardware.clean
	rm -rf $(TESTBENCH_PATH)/vcs_simlibs/BSG_*
	rm -rf $(TESTBENCH_PATH)/vcs_simlibs/cosim_wrapper
	rm -rf $(TESTBENCH_PATH)/libfpga_mgmt.so
	rm -rf $(LIBRARIES_PATH)/libbsg_manycore_runtime.so
	rm -rf $(LIBRARIES_PATH)/libbsg_manycore_runtime.so.1

bleach:
	rm -rf $(TESTBENCH_PATH)/vcs_simlibs $(TESTBENCH_PATH)/synopsys_sim.setup
