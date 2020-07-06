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

################################################################################
# F1-Specific Sources
################################################################################
# F1 Header file. Defines the DUT design macro (CL_NAME) for top.sv in aws-fpga
VHEADERS += $(BSG_PLATFORM_PATH)/hardware/cl_manycore_defines.vh
# PCIe Macro Definitions
VHEADERS += $(BSG_PLATFORM_PATH)/hardware/cl_id_defines.vh

# AXI Bus Definitions
VHEADERS += $(BSG_PLATFORM_PATH)/hardware/bsg_axi_bus_pkg.vh

# Manycore architecture definitions
# cl_manycore_pkg.v depends on f1_parameters.vh
VSOURCES += $(BSG_PLATFORM_PATH)/hardware/cl_manycore_pkg.v

# Wrapper for bsg_manycore. Depends on sources in arch_filelist.mk
VSOURCES += $(BSG_PLATFORM_PATH)/hardware/bsg_manycore_wrapper_mesh.v

# Cache to AXI Sources (For F1 Memory)
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_to_axi_rx.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_to_axi_tx.v
VSOURCES += $(BSG_MANYCORE_DIR)/v/vanilla_bean/hash_function_reverse.v
VSOURCES += $(BSG_MANYCORE_DIR)/v/vanilla_bean/bsg_cache_to_axi_hashed.v

# AXI-Lite to Manycore link sources
VSOURCES += $(BSG_PLATFORM_PATH)/hardware/bsg_manycore_link_to_axil_pkg.v
VSOURCES += $(BSG_PLATFORM_PATH)/hardware/bsg_manycore_link_to_axil.v
VSOURCES += $(BSG_PLATFORM_PATH)/hardware/bsg_mcl_axil_fifos_master.v
VSOURCES += $(BSG_PLATFORM_PATH)/hardware/bsg_mcl_axil_fifos_slave.v
VSOURCES += $(BSG_PLATFORM_PATH)/hardware/bsg_manycore_endpoint_to_fifos_pkg.v
VSOURCES += $(BSG_PLATFORM_PATH)/hardware/bsg_manycore_endpoint_to_fifos.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_serial_in_parallel_out_full.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_round_robin_1_to_n.v
VSOURCES += $(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_one_fifo.v

################################################################################
# Top-level file
################################################################################
VSOURCES += $(BSG_PLATFORM_PATH)/hardware/cl_manycore.sv

# Replace any xilinx-unsynthesizable sources with xilinx-synthesizable sources
VHEADERS := $(filter-out $(BASEJUMP_STL_DIR)/bsg_mem/bsg_mem_1rw_sync_mask_write_bit.v,$(VHEADERS))
VSOURCES := $(filter-out $(BASEJUMP_STL_DIR)/bsg_mem/bsg_mem_1rw_sync_mask_write_bit.v,$(VSOURCES))
VSOURCES += $(BASEJUMP_STL_DIR)/hard/ultrascale_plus/bsg_mem/bsg_mem_1rw_sync_mask_write_bit.v
