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

# BSG_MACHINE_NAME: The name of the target machine. Should be defined
# in $(BSG_MACHINE_PATH)/Makefile.machine.include, which is included
# by hardware.mk
ifndef BSG_MACHINE_NAME
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_MACHINE_NAME is not defined$(NC)"))
endif

################################################################################
# Simulation Sources
################################################################################
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_clock_gen.sv
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_reset_gen.sv

POD_TRACE_GEN_PY = $(BSG_MANYCORE_DIR)/testbenches/py/pod_trace_gen.py
$(BSG_MACHINE_PATH)/bsg_tag_boot_rom.tr: $(BSG_MACHINE_PATH)/Makefile.machine.include
	env python2 $(POD_TRACE_GEN_PY) $(BSG_MACHINE_PODS_X) $(BSG_MACHINE_PODS_Y) $(BSG_MACHINE_NOC_COORD_X_WIDTH) > $@

ASCII_TO_ROM_PY = $(BASEJUMP_STL_DIR)/bsg_mem/bsg_ascii_to_rom.py
$(BSG_MACHINE_PATH)/bsg_tag_boot_rom.v: $(BSG_MACHINE_PATH)/bsg_tag_boot_rom.tr
	env python2 $(ASCII_TO_ROM_PY) $< bsg_tag_boot_rom > $@

VSOURCES += $(BSG_MACHINE_PATH)/bsg_tag_boot_rom.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_trace_replay.sv
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_tag/bsg_tag_trace_replay.sv
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_tag/bsg_tag_master.sv

# DMA Interface
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vcache_dma_to_dram_channel_map.sv
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_mem/bsg_nonsynth_mem_1r1w_sync_dma.sv
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_mem/bsg_nonsynth_mem_1r1w_sync_mask_write_byte_dma.sv
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_mem/bsg_nonsynth_mem_1rw_sync_mask_write_byte_dma.sv

# DRAMSim3
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_dramsim3_pkg.sv
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dramsim3.sv
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dramsim3_map.sv
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dramsim3_unmap.sv

VSOURCES += $(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_to_test_dram.sv
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_to_test_dram_tx.sv
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_to_test_dram_rx.sv
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_to_test_dram_rx_reorder.sv

# Infinite Memory
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_nonsynth_mem_infinite.sv

# Profiling
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_manycore_profile_pkg.sv
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_misc/bsg_cycle_counter.sv

# Core Profiler/Trace
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vanilla_scoreboard_tracker_pkg.sv
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vanilla_exe_bubble_classifier_pkg.sv
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vanilla_exe_bubble_classifier.sv
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vanilla_scoreboard_tracker.sv

VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dpi_gpio.sv
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/instr_trace.sv
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vanilla_core_trace.sv
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vanilla_core_profiler.sv
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vanilla_core_pc_histogram.sv
VSOURCES += $(HARDWARE_PATH)/bsg_print_stat_snoop.v

VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/router_profiler.sv
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/remote_load_trace.sv

# Memory Profilers
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vcache_profiler.sv
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/infinite_mem_profiler.sv

VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_nonsynth_manycore_tag_master.sv
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_nonsynth_manycore_io_complex.sv
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_nonsynth_manycore_spmd_loader.sv
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_nonsynth_manycore_monitor.sv
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_nonsynth_wormhole_test_mem.sv
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_nonsynth_manycore_testbench.sv
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_cache/bsg_wormhole_to_cache_dma_fanout.sv

VSOURCES += $(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_serial_in_parallel_out_full.sv
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_round_robin_1_to_n.sv
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_one_fifo.sv

VSOURCES += $(HARDWARE_PATH)/bsg_manycore_endpoint_to_fifos_pkg.v
VSOURCES += $(HARDWARE_PATH)/bsg_manycore_endpoint_to_fifos.v

VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/vanilla_core_saif_dumper.sv

################################################################################
# DPI-Specific Sources
################################################################################

VSOURCES += $(BSG_MANYCORE_DIR)/v/bsg_manycore_link_sif_async_buffer.sv
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dpi_from_fifo.sv
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dpi_to_fifo.sv
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dpi_rom.sv
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/dpi/bsg_nonsynth_dpi_manycore.sv

VSOURCES += $(BASEJUMP_STL_DIR)/bsg_test/bsg_nonsynth_dpi_cycle_counter.sv

################################################################################
# Top-level files
################################################################################
# Top-level module name
BSG_DESIGN_TOP := replicant_tb_top

VSOURCES += $(LIBRARIES_PATH)/platforms/bigblade-vcs/hardware/dpi_top.sv

VINCLUDES += $(BSG_PLATFORM_PATH)/hardware
VINCLUDES += $(BSG_PLATFORM_PATH)


hardware.clean: machine.hardware.clean

machine.hardware.clean:
	rm -rf $(BSG_MACHINE_PATH)/bsg_tag_boot_rom.tr $(BSG_MACHINE_PATH)/bsg_tag_boot_rom.v
