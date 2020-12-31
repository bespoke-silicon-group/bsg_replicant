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

# Reuse the source list from dpi-verilator
include $(LIBRARIES_PATH)/platforms/dpi-verilator/hardware.mk

# Replace dpi_clock_gen with the normal clock generator (see
# dpi_top.sv for a description of the issue)
VSOURCES := $(subst bsg_nonsynth_dpi_clock_gen,bsg_nonsynth_clock_gen,$(VSOURCES))

# TODO: Extract from BlackParrot flist
VINCLUDES += $(BASEJUMP_STL_DIR)/bsg_dataflow
VINCLUDES += $(BASEJUMP_STL_DIR)/bsg_mem
VINCLUDES += $(BASEJUMP_STL_DIR)/bsg_misc
VINCLUDES += $(BASEJUMP_STL_DIR)/bsg_test
VINCLUDES += $(BASEJUMP_STL_DIR)/bsg_noc
VINCLUDES += $(BLACKPARROT_DIR)/external/HardFloat/source
VINCLUDES += $(BLACKPARROT_DIR)/external/HardFloat/source/RISCV
VINCLUDES += $(BLACKPARROT_DIR)/bp_common/src/include
VINCLUDES += $(BLACKPARROT_DIR)/bp_fe/src/include
VINCLUDES += $(BLACKPARROT_DIR)/bp_be/src/include
VINCLUDES += $(BLACKPARROT_DIR)/bp_me/src/include
VINCLUDES += $(BLACKPARROT_DIR)/bp_top/src/include

# Prepend packages to VSOURCES
#VSOURCES :=$(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_pkg.v $(VSOURCES)
VSOURCES :=$(BASEJUMP_STL_DIR)/bsg_noc/bsg_noc_pkg.v $(VSOURCES)
VSOURCES :=$(BASEJUMP_STL_DIR)/bsg_noc/bsg_wormhole_router_pkg.v $(VSOURCES)
VSOURCES :=$(BLACKPARROT_DIR)/bp_fe/src/include/bp_fe_pkg.sv $(VSOURCES)
VSOURCES :=$(BLACKPARROT_DIR)/bp_be/src/include/bp_be_pkg.sv $(VSOURCES)
VSOURCES :=$(BLACKPARROT_DIR)/bp_me/src/include/bp_me_pkg.sv $(VSOURCES)
VSOURCES :=$(BLACKPARROT_DIR)/bp_common/src/include/bp_common_aviary_pkg.sv $(VSOURCES)
VSOURCES :=$(BLACKPARROT_DIR)/bp_common/src/include/bp_common_pkg.sv $(VSOURCES)
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_async/bsg_async_fifo.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_async/bsg_launch_sync_sync.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_async/bsg_async_ptr_gray.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_dma.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_miss.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_decode.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_sbuf.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_cache/bsg_cache_sbuf_queue.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_channel_tunnel.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_channel_tunnel_in.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_channel_tunnel_out.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_1_to_n_tagged_fifo.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_1_to_n_tagged.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_fifo_1r1w_large.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_fifo_1r1w_pseudo_large.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_fifo_1r1w_small.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_fifo_1r1w_small_unhardened.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_fifo_1rw_large.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_fifo_reorder.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_fifo_tracker.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_flow_counter.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_one_fifo.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_parallel_in_serial_out.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_parallel_in_serial_out_dynamic.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_parallel_in_serial_out_passthrough.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_round_robin_1_to_n.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_round_robin_2_to_2.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_round_robin_n_to_1.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_serial_in_parallel_out.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_serial_in_parallel_out_dynamic.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_serial_in_parallel_out_full.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_serial_in_parallel_out_passthrough.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_shift_reg.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_dataflow/bsg_two_fifo.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_mem/bsg_cam_1r1w.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_mem/bsg_cam_1r1w_replacement.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_mem/bsg_cam_1r1w_sync.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_mem/bsg_cam_1r1w_tag_array.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_mem/bsg_mem_1r1w.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_mem/bsg_mem_1r1w_one_hot.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_mem/bsg_mem_1r1w_sync.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_mem/bsg_mem_1r1w_sync_synth.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_mem/bsg_mem_1r1w_synth.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_mem/bsg_mem_1rw_sync.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_mem/bsg_mem_1rw_sync_mask_write_bit.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_mem/bsg_mem_1rw_sync_mask_write_bit_synth.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_mem/bsg_mem_1rw_sync_mask_write_byte.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_mem/bsg_mem_1rw_sync_mask_write_byte_synth.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_mem/bsg_mem_1rw_sync_synth.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_mem/bsg_mem_2r1w_sync.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_mem/bsg_mem_2r1w_sync_synth.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_mem/bsg_mem_3r1w_sync.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_mem/bsg_mem_3r1w_sync_synth.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_adder_cin.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_adder_one_hot.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_adder_ripple_carry.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_arb_fixed.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_array_concentrate_static.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_buf.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_buf_ctrl.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_circular_ptr.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_concentrate_static.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_counter_clear_up.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_counter_clear_up_one_hot.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_counter_set_down.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_counter_set_en.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_counter_up_down.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_counter_up_down_variable.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_crossbar_o_by_i.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_cycle_counter.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_decode.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_decode_with_v.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_dff.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_dff_en_bypass.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_dff_reset_en_bypass.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_dff_chain.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_dff_en.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_dff_reset.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_dff_reset_en.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_dff_reset_set_clear.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_edge_detect.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_encode_one_hot.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_expand_bitmask.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_hash_bank.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_hash_bank_reverse.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_idiv_iterative.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_idiv_iterative_controller.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_lfsr.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_lru_pseudo_tree_decode.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_lru_pseudo_tree_encode.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_mux.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_mux_bitwise.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_mux_butterfly.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_mux_one_hot.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_mux_segmented.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_muxi2_gatestack.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_nor2.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_nor3.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_nand.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_priority_encode.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_priority_encode_one_hot_out.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_reduce.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_reduce_segmented.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_rotate_left.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_rotate_right.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_round_robin_arb.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_scan.v
VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_strobe.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_swap.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_thermometer_count.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_transpose.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_unconcentrate_static.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_misc/bsg_xnor.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_noc/bsg_mesh_stitch.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_noc/bsg_noc_repeater_node.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_noc/bsg_wormhole_concentrator.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_noc/bsg_wormhole_concentrator_in.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_noc/bsg_wormhole_concentrator_out.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_noc/bsg_wormhole_router.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_noc/bsg_wormhole_router_adapter.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_noc/bsg_wormhole_router_adapter_in.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_noc/bsg_wormhole_router_adapter_out.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_noc/bsg_wormhole_router_decoder_dor.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_noc/bsg_wormhole_router_input_control.v
#VSOURCES +=$(BASEJUMP_STL_DIR)/bsg_noc/bsg_wormhole_router_output_control.v
#VSOURCES +=$(BLACKPARROT_DIR)/external/HardFloat/source/addRecFN.v
#VSOURCES +=$(BLACKPARROT_DIR)/external/HardFloat/source/compareRecFN.v
#VSOURCES +=$(BLACKPARROT_DIR)/external/HardFloat/source/divSqrtRecFN_small.v
#VSOURCES +=$(BLACKPARROT_DIR)/external/HardFloat/source/fNToRecFN.v
#VSOURCES +=$(BLACKPARROT_DIR)/external/HardFloat/source/HardFloat_primitives.v
#VSOURCES +=$(BLACKPARROT_DIR)/external/HardFloat/source/HardFloat_rawFN.v
#VSOURCES +=$(BLACKPARROT_DIR)/external/HardFloat/source/iNToRecFN.v
#VSOURCES +=$(BLACKPARROT_DIR)/external/HardFloat/source/isSigNaNRecFN.v
#VSOURCES +=$(BLACKPARROT_DIR)/external/HardFloat/source/mulAddRecFN.v
#VSOURCES +=$(BLACKPARROT_DIR)/external/HardFloat/source/mulRecFN.v
#VSOURCES +=$(BLACKPARROT_DIR)/external/HardFloat/source/recFNToFN.v
#VSOURCES +=$(BLACKPARROT_DIR)/external/HardFloat/source/recFNToIN.v
VSOURCES +=$(BLACKPARROT_DIR)/external/HardFloat/source/recFNToRecFN.v
#VSOURCES +=$(BLACKPARROT_DIR)/external/HardFloat/source/RISCV/HardFloat_specialize.v
VSOURCES +=$(BLACKPARROT_DIR)/bp_common/src/v/bsg_fifo_1r1w_rolly.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_common/src/v/bsg_bus_pack.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_common/src/v/bp_mmu.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_common/src/v/bp_pma.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_common/src/v/bp_tlb.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_top.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_calculator/bp_be_bypass.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_calculator/bp_be_calculator_top.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_calculator/bp_be_csr.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_calculator/bp_be_fp_to_rec.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_calculator/bp_be_pipe_int.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_calculator/bp_be_pipe_aux.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_calculator/bp_be_pipe_ctl.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_calculator/bp_be_pipe_fma.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_calculator/bp_be_pipe_long.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_calculator/bp_be_pipe_mem.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_calculator/bp_be_pipe_sys.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_calculator/bp_be_ptw.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_calculator/bp_be_rec_to_fp.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_checker/bp_be_detector.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_checker/bp_be_director.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_checker/bp_be_instr_decoder.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_checker/bp_be_issue_queue.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_checker/bp_be_regfile.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_checker/bp_be_scheduler.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_dcache/bp_be_dcache.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_dcache/bp_be_dcache_decoder.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_dcache/bp_be_dcache_wbuf.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_be/src/v/bp_be_dcache/bp_be_dcache_wbuf_queue.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_fe/src/v/bp_fe_bht.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_fe/src/v/bp_fe_btb.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_fe/src/v/bp_fe_icache.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_fe/src/v/bp_fe_instr_scan.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_fe/src/v/bp_fe_pc_gen.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_fe/src/v/bp_fe_top.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/lce/bp_lce.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/lce/bp_lce_req.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/lce/bp_lce_cmd.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cache/bp_me_cache_dma_to_cce.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cache/bp_me_cache_slice.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cache/bp_me_cce_to_cache.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce_alu.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce_arbitrate.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce_branch.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce_dir.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce_dir_lru_extract.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce_dir_segment.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce_dir_tag_checker.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce_gad.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce_inst_decode.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce_inst_predecode.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce_inst_ram.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce_inst_stall.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce_msg.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce_pending_bits.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce_reg.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce_spec_bits.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce_src_sel.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_io_cce.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce_fsm.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce_wrapper.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_cce_loopback.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/cce/bp_uce.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/wormhole/bp_me_addr_to_cce_id.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/wormhole/bp_me_cce_id_to_cord.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/wormhole/bp_me_cce_to_mem_link_bidir.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/wormhole/bp_me_cce_to_mem_link_client.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/wormhole/bp_me_cce_to_mem_link_master.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/wormhole/bp_me_cord_to_id.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/wormhole/bp_me_lce_id_to_cord.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/wormhole/bp_me_wormhole_packet_encode_lce_cmd.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/wormhole/bp_me_wormhole_packet_encode_lce_req.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/wormhole/bp_me_wormhole_packet_encode_lce_resp.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/wormhole/bp_me_wormhole_packet_encode_mem_cmd.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/wormhole/bp_me_wormhole_packet_encode_mem_resp.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/wormhole/bp_lite_to_stream.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/wormhole/bp_stream_to_lite.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/wormhole/bp_lite_to_burst.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_me/src/v/wormhole/bp_burst_to_lite.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_nd_socket.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_cacc_vdp.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_cacc_tile.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_cacc_tile_node.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_cacc_complex.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_sacc_vdp.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_sacc_tile.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_sacc_tile_node.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_sacc_complex.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_cfg.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_core.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_core_minimal.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_core_complex.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_clint_slice.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_l2e_tile.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_l2e_tile_node.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_io_complex.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_io_link_to_lce.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_io_tile.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_io_tile_node.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_mem_complex.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_multicore.sv
#VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_unicore.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_unicore_lite.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_tile.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bp_tile_node.sv
VSOURCES +=$(BLACKPARROT_DIR)/bp_top/src/v/bsg_async_noc_link.sv

