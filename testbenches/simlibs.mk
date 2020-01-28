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

# -------------------- Arguments --------------------
# This Makefile has several optional "arguments" that are passed as Variables
#
# EXTRA_TURBO: Disables VPD Generation, and more optimization flags: Default 0
# 
# If you need additional speed, you can set EXTRA_TURBO=1 during compilation. 
# This is a COMPILATION ONLY option. Any subsequent runs, without compilation
# will retain this setting
EXTRA_TURBO      ?= 0

# The following variables are set by $(BSG_F1_DIR)/hdk.mk, which will fail if
# hdk_setup.sh has not been run, or environment.mk is not included
#
# HDK_SHELL_DESIGN_DIR: Path to the directory containing all the AWS "shell" IP
# AWS_FPGA_REPO_DIR: Path to the clone of the aws-fpga repo
# HDK_COMMON_DIR: Path to HDK 'common' directory w/ libraries for cosimluation.
# SDK_DIR: Path to the SDK directory in the aws-fpga repo
include $(BSG_F1_DIR)/hdk.mk

# libraries.mk defines the sources and targets for the BSG Manycore Runtime
# library.
include $(LIBRARIES_PATH)/libraries.mk

# libbsg_manycore_runtime will be compiled in $(LIBRARIES_PATH)
LDFLAGS    += -L$(LIBRARIES_PATH) -Wl,-rpath=$(LIBRARIES_PATH)

# So that we can limit tool-specific to a few specific spots we use VDEFINES,
# VINCLUDES, and VSOURCES to hold lists of macros, include directores, and
# verilog headers, and sources (respectively). These are used during simulation
# compilation, but transformed into a tool-specific syntax where necesssary.
VINCLUDES += $(TESTBENCH_PATH)

# $(HARDWARE_PATH)/hardware.mk adds to VSOURCES which is a list of verilog
# source files for cosimulation and compilation, and VHEADERS, which is similar,
# but for header files. It also adds to CLEANS, a list of clean rules for
# cleaning hardware targets.
include $(HARDWARE_PATH)/hardware.mk

# Name of the cosimulation wrapper system verilog file (usually in
# TESTBENCHES_PATH)
WRAPPER_NAME = cosim_wrapper

VSOURCES += $(TESTBENCH_PATH)/$(WRAPPER_NAME).sv

# The manycore architecture unsynthesizable simulation sources (for tracing, etc).
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_nonsynth_mem_infinite.v

VSOURCES += $(BASEJUMP_STL_DIR)/bsg_mem/bsg_nonsynth_mem_1rw_sync_mask_write_byte_assoc.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_mem/bsg_nonsynth_mem_1rw_sync_assoc.v

VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_clock_gen.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_reset_gen.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_ramulator_hbm.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_test_dram_channel.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_serial_in_parallel_out_full.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_round_robin_1_to_n.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_one_fifo.v

VSOURCES += $(BASEJUMP_STL_DIR)/bsg_misc/bsg_cycle_counter.v

VSOURCES += $(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_to_ramulator_hbm.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_to_ramulator_hbm_rx.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_to_ramulator_hbm_tx.v
# Add files needed by DRAMSim3
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dramsim3.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dramsim3_map.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dramsim3_unmap.v

VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/instr_trace.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vanilla_core_trace.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_manycore_link_to_cache_tracer.v

VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_manycore_profile_pkg.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vanilla_core_profiler.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vcache_profiler.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vcache_non_blocking_profiler.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/infinite_mem_profiler.v

VSOURCES += $(BSG_MANYCORE_DIR)/v/bsg_manycore_link_sif_async_buffer.v

# Include makefile for ramulator sources
include $(TESTBENCH_PATH)/ramulator.mk
include $(TESTBENCH_PATH)/dramsim3.mk

# -------------------- TARGETS --------------------
# This makefile defines two variables for External Use: 
#
# SIMLIBS: Targets for building hardware/software simulation libraries
SIMLIBS += $(TESTBENCH_PATH)/libfpga_mgmt.so
SIMLIBS += $(LIBRARIES_PATH)/libbsg_manycore_runtime.so
SIMLIBS += $(TESTBENCH_PATH)/synopsys_sim.setup
SIMLIBS += $(WORKDIR)/AN.DB
WORKDIR = $(TESTBENCH_PATH)/vcs_simlibs/$(PROJECT)

# Using the generic variables VSOURCES, VINCLUDES, and VDEFINES, we create
# tool-specific versions of the same variables. VHEADERS must be compiled before
# VSOURCES.
VDEFINES   += VCS_SIM
VDEFINES   += COSIM
VDEFINES   += DISABLE_VJTAG_DEBUG
VDEFINES   += ENABLE_PROTOCOL_CHK

include $(BSG_MACHINE_PATH)/Makefile.machine.include
# Setting CL_MANYCORE_MEM_CFG to e_vcache_blocking_axi4_f1_dram
# directs simulation to use the slower, but more accurate, DDR
# Model. The default is e_vcache_blocking_axi4_f1_model uses an
# (infinite) AXI memory model with low (1-2 cycle) latency in
# simulation.
ifeq ($(CL_MANYCORE_MEM_CFG),e_vcache_blocking_axi4_f1_model)
VDEFINES   += AXI_MEMORY_MODEL=1
VDEFINES   += ECC_DIRECT_EN
VDEFINES   += RND_ECC_EN
VDEFINES   += ECC_ADDR_LO=0
VDEFINES   += ECC_ADDR_HI=0
VDEFINES   += RND_ECC_WEIGHT=0
endif

ifeq ($(CL_MANYCORE_MEM_CFG),e_vcache_non_blocking_axi4_f1_model)
VDEFINES   += AXI_MEMORY_MODEL=1
VDEFINES   += ECC_DIRECT_EN
VDEFINES   += RND_ECC_EN
VDEFINES   += ECC_ADDR_LO=0
VDEFINES   += ECC_ADDR_HI=0
VDEFINES   += RND_ECC_WEIGHT=0
endif

ifeq ($(EXTRA_TURBO), 1)
VLOGAN_VFLAGS += -undef_vcs_macro
endif

VLOGAN_SOURCES  += $(VHEADERS) $(VSOURCES) 
VLOGAN_INCLUDES += $(foreach inc,$(VINCLUDES),+incdir+"$(inc)")
VLOGAN_DEFINES  += $(foreach def,$(VDEFINES),+define+"$(def)")
VLOGAN_FILELIST += $(TESTBENCH_PATH)/aws.vcs.f
VLOGAN_VFLAGS   += -ntb_opts tb_timescale=1ps/1ps -timescale=1ps/1ps \
                   -sverilog +systemverilogext+.svh +systemverilogext+.sv \
                   +libext+.sv +libext+.v +libext+.vh +libext+.svh \
                   -full64 -lca -v2005 +v2k +lint=TFIPC-L -assert svaext

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

$(WORKDIR)/AN.DB: $(TESTBENCH_PATH)/synopsys_sim.setup $(BSG_MACHINE_PATH)/Makefile.machine.include $(VHEADERS) $(VSOURCES)
	echo "$(PROJECT) : $(WORKDIR)/64" >> $(TESTBENCH_PATH)/synopsys_sim.setup
	cd $(TESTBENCH_PATH) && \
	XILINX_IP=$(XILINX_IP) \
	HDK_COMMON_DIR=$(HDK_COMMON_DIR) \
	HDK_SHELL_DESIGN_DIR=$(HDK_SHELL_DESIGN_DIR) \
	HDK_SHELL_DIR=$(HDK_SHELL_DIR) \
	XILINX_VIVADO=$(XILINX_VIVADO) \
	vlogan -work $(PROJECT) $(VLOGAN_VFLAGS) $(VLOGAN_DEFINES) $(VLOGAN_SOURCES) \
		-f $(TESTBENCH_PATH)/aws.vcs.f $(VLOGAN_DEFINES)  \
		$(VLOGAN_INCLUDES) -l compile.vlogan.log

# The applications link against the BSG Manycore Libraries, and the FPGA
# Management libaries, so we build them as necessary. They do NOT need to be
# re-built every time a regression test is compiled

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
	rm -rf $(TESTBENCH_PATH)/synopsys_sim.setup
	rm -rf $(TESTBENCH_PATH)/vcs_simlibs
	rm -rf $(TESTBENCH_PATH)/libfpga_mgmt.so
	rm -rf $(LIBRARIES_PATH)/libbsg_manycore_runtime.so
	rm -rf $(LIBRARIES_PATH)/libbsg_manycore_runtime.so.1
