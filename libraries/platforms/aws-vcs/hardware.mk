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

# hardware.mk: Platform-specific HDL listing. 
#
# For simulation platforms, it also describes how to build the
# simulation "libraries" that are required by CAD tools.
#
# This file should be included from bsg_replicant/hardware/hardware.mk. It checks
# BSG_PLATFORM_PATH, BASEJUMP_STL_DIR, BSG_MANYCORE_DIR, etc.

# XILINX_VIVADO is set by Vivado's configuration script. We use this
# as a quick check instead of running Vivado.
ifndef XILINX_VIVADO
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: XILINX_VIVADO environment variable undefined. Are you sure Vivado is installed?$(NC)"))
endif

# BSG_MACHINE_NAME: The name of the target machine. Should be defined
# in $(BSG_MACHINE_PATH)/Makefile.machine.include, which is included
# by hardware.mk
ifndef BSG_MACHINE_NAME
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_MACHINE_NAME is not defined$(NC)"))
endif

# The following variables are set by $(BSG_F1_DIR)/hdk.mk, which will fail if
# hdk_setup.sh has not been run, or environment.mk is not included
#
# HDK_SHELL_DESIGN_DIR: Path to the directory containing all the AWS "shell" IP
# AWS_FPGA_REPO_DIR: Path to the clone of the aws-fpga repo
# HDK_COMMON_DIR: Path to HDK 'common' directory w/ libraries for cosimluation.
# SDK_DIR: Path to the SDK directory in the aws-fpga repo
include $(BSG_F1_DIR)/hdk.mk

################################################################################
# Simulation Sources
################################################################################
# The aws-vcs platform uses unsynthesizable sources for simulation

# VCS NS Modules
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_clock_gen.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_reset_gen.v

# DMA Interface
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_mem/bsg_nonsynth_mem_1r1w_sync_dma.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_mem/bsg_nonsynth_mem_1r1w_sync_mask_write_byte_dma.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_mem/bsg_nonsynth_mem_1rw_sync_mask_write_byte_dma.v

# Cache / Simulation DRAM Interface
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_to_test_dram.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_to_test_dram_rx.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_to_test_dram_rx_reorder.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_to_test_dram_tx.v

# DRAMSim3
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_dramsim3_pkg.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dramsim3.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dramsim3_map.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dramsim3_unmap.v

# Infinite Memory
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_nonsynth_mem_infinite.v

# Profiling
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_manycore_profile_pkg.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_misc/bsg_cycle_counter.v

# Core Profiler/Trace
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dpi_gpio.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/instr_trace.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vanilla_core_trace.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vanilla_core_profiler.v
VSOURCES += $(BSG_PLATFORM_PATH)/hardware/bsg_print_stat_snoop.v

# Memory Profilers
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vcache_profiler.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vcache_non_blocking_profiler.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/infinite_mem_profiler.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_manycore_link_to_cache_tracer.v

################################################################################
# F1-Specific Sources (reused from aws-fpga platform directory)
################################################################################
# F1 Header file. Defines the DUT design macro (CL_NAME) for top.sv in aws-fpga
VHEADERS += $(LIBRARIES_PATH)/platforms/aws-fpga/hardware/cl_manycore_defines.vh
# PCIe Macro Definitions
VHEADERS += $(LIBRARIES_PATH)/platforms/aws-fpga/hardware/cl_id_defines.vh

# AXI Bus Definitions
VHEADERS += $(LIBRARIES_PATH)/platforms/aws-fpga/hardware/bsg_axi_bus_pkg.vh

# Manycore architecture definitions
# cl_manycore_pkg.v depends on f1_parameters.vh
VSOURCES += $(LIBRARIES_PATH)/platforms/aws-fpga/hardware/cl_manycore_pkg.v

# Crossbar sources
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_misc/bsg_arb_round_robin.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_misc/bsg_crossbar_control_basic_o_by_i.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_noc/bsg_router_crossbar_o_by_i.v

VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_manycore_top_crossbar.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_manycore_crossbar.v
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_manycore_link_to_crossbar.v

# Wrapper for bsg_manycore. Depends on sources in arch_filelist.mk
VSOURCES += $(LIBRARIES_PATH)/platforms/aws-fpga/hardware/bsg_manycore_wrapper_crossbar.v
VSOURCES += $(LIBRARIES_PATH)/platforms/aws-fpga/hardware/bsg_manycore_wrapper_mesh.v

# Cache to AXI Sources (For F1 Memory)
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_to_axi_rx.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_to_axi_tx.v
VSOURCES += $(BSG_MANYCORE_DIR)/v/vanilla_bean/hash_function_reverse.v
VSOURCES += $(BSG_MANYCORE_DIR)/v/vanilla_bean/bsg_cache_to_axi_hashed.v

# AXI-Lite to Manycore link sources
VSOURCES += $(BSG_MANYCORE_DIR)/v/bsg_manycore_link_sif_async_buffer.v
VSOURCES += $(LIBRARIES_PATH)/platforms/aws-fpga/hardware/bsg_manycore_link_to_axil_pkg.v
VSOURCES += $(LIBRARIES_PATH)/platforms/aws-fpga/hardware/bsg_manycore_link_to_axil.v
VSOURCES += $(LIBRARIES_PATH)/platforms/aws-fpga/hardware/bsg_mcl_axil_fifos_master.v
VSOURCES += $(LIBRARIES_PATH)/platforms/aws-fpga/hardware/bsg_mcl_axil_fifos_slave.v
VSOURCES += $(LIBRARIES_PATH)/platforms/aws-fpga/hardware/bsg_manycore_endpoint_to_fifos_pkg.v
VSOURCES += $(LIBRARIES_PATH)/platforms/aws-fpga/hardware/bsg_manycore_endpoint_to_fifos.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_serial_in_parallel_out_full.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_round_robin_1_to_n.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_one_fifo.v

################################################################################
# Top-level files
################################################################################
VSOURCES += $(LIBRARIES_PATH)/platforms/aws-fpga/hardware/cl_manycore.sv

# machine_wrapper.sv is the VCS machine library file for each machine.
# BSG_MACHINE_NAME must be defined as a macro when compiling it.
VSOURCES += $(BSG_PLATFORM_PATH)/machine_wrapper.sv

# Using the generic variables VSOURCES, VINCLUDES, and VDEFINES, we create
# tool-specific versions of the same variables. 
# So that we can limit tool-specific to a few specific spots we use VDEFINES,
# VINCLUDES, and VSOURCES to hold lists of macros, include directores, and
# verilog headers, and sources (respectively). These are used during simulation
# compilation, but transformed into a tool-specific syntax where necesssary.
VINCLUDES += $(LIBRARIES_PATH)/platforms/aws-fpga/hardware
VINCLUDES += $(BSG_PLATFORM_PATH)/hardware
VINCLUDES += $(BSG_PLATFORM_PATH)

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
# cd to $(BSG_PLATFORM_PATH) so that vcs_simlibs can be reused across invocations
# of cosimulation.
#
# This output is reused between regression test suites, so it should be removed
# with `make squeakyclean` or an equivalent
$(BSG_PLATFORM_PATH)/synopsys_sim.setup: $(BSG_PLATFORM_PATH)/gen_simlibs.tcl
	cd $(BSG_PLATFORM_PATH) && vivado -mode batch -source $<

$(BSG_MACHINE_PATH)/synopsys_sim.setup: $(BSG_PLATFORM_PATH)/synopsys_sim.setup
	cp $< $@
	echo "$(BSG_MACHINE_NAME) : $(BSG_PLATFORM_PATH)/vcs_simlibs/$(BSG_MACHINE_NAME)/64" >> $@;

# We check synopsys_sim.setup for the library location before
# modifying it. This avoids repeating the line at the end of the file
# every single time.
$(BSG_PLATFORM_PATH)/vcs_simlibs/$(BSG_MACHINE_NAME)/AN.DB: $(BSG_MACHINE_PATH)/synopsys_sim.setup $(VHEADERS) $(VSOURCES)
	cd $(BSG_PLATFORM_PATH) && \
	XILINX_IP=$(XILINX_IP) \
	XILINX_VIVADO=$(XILINX_VIVADO) \
	HDK_COMMON_DIR=$(HDK_COMMON_DIR) \
	HDK_SHELL_DESIGN_DIR=$(HDK_SHELL_DESIGN_DIR) \
	HDK_SHELL_DIR=$(HDK_SHELL_DIR) \
	SYNOPSYS_SIM_SETUP=$(BSG_MACHINE_PATH)/synopsys_sim.setup \
	vlogan -work $(BSG_MACHINE_NAME) $(VLOGAN_VFLAGS) $(VLOGAN_DEFINES) \
		$(VLOGAN_SOURCES) -f $(BSG_PLATFORM_PATH)/aws.vcs.f \
		$(VLOGAN_INCLUDES) -l $(BSG_MACHINE_NAME).vlogan.log

.PHONY: platform.hardware.clean platform.hardware.bleach
platform.hardware.clean:
	rm -rf $(BSG_MACHINE_PATH)/synopsys_sim.setup
	rm -rf $(BSG_PLATFORM_PATH)/vcs_simlibs/BSG_*
	rm -rf $(BSG_PLATFORM_PATH)/.cxl*
	rm -rf $(BSG_PLATFORM_PATH)/*.jou
	rm -rf $(BSG_PLATFORM_PATH)/*.log 
	rm -rf $(BSG_PLATFORM_PATH)/*.log.bak

platform.hardware.bleach:
	rm -rf $(BSG_PLATFORM_PATH)/synopsys_sim.setup
	rm -rf $(BSG_PLATFORM_PATH)/vcs_simlibs

hardware.clean: platform.hardware.clean
hardware.bleach: platform.hardware.bleach
