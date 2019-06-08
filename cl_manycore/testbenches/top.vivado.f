# Amazon FPGA Hardware Development Kit
#
# Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Amazon Software License (the "License"). You may not use
# this file except in compliance with the License. A copy of the License is
# located at
#
#    http://aws.amazon.com/asl/
#
# or in the "license" file accompanying this file. This file is distributed on
# an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, express or
# implied. See the License for the specific language governing permissions and
# limitations under the License.

-define VIVADO_SIM

-sourcelibext .v
-sourcelibext .vh
-sourcelibext .sv
-sourcelibext .svh

# Custom Logic (CL) source directories
-sourcelibdir ${CL_DIR}/hardware

# AWS source library directories
-sourcelibdir ${HDK_SHELL_DESIGN_DIR}/lib
-sourcelibdir ${HDK_SHELL_DESIGN_DIR}/interfaces
-sourcelibdir ${HDK_SHELL_DESIGN_DIR}/sh_ddr/sim
-sourcelibdir ${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/hdl
-sourcelibdir ${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/sim

# Custom Logic (CL) include directories
-include ${CL_DIR}/hardware

# AWS include directories
#-include ${HDK_COMMON_DIR}/verif/include
-include ${CL_DIR}/testbenches
-include ${HDK_SHELL_DIR}/hlx/verif
-include ${HDK_SHELL_DESIGN_DIR}/lib
-include ${HDK_SHELL_DESIGN_DIR}/interfaces
-include ${HDK_SHELL_DESIGN_DIR}/sh_ddr/sim
-include ${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/ip/ip_0/sim
-include ${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/ip/ip_0/hdl/verilog
-include ${HDK_SHELL_DESIGN_DIR}/ip/axi_register_slice/hdl
-include ${HDK_SHELL_DESIGN_DIR}/ip/axi_register_slice_light/hdl
-include ${BSG_IP_CORES_DIR}/bsg_misc
-include ${BSG_IP_CORES_DIR}/bsg_noc
-include ${BSG_IP_CORES_DIR}/bsg_cache
-include ${BSG_MANYCORE_DIR}/v/
-include ${BSG_MANYCORE_DIR}/v/vanilla_bean
-include ${CL_DIR}/../hdl/

# Custom Logic (CL) design files
${CL_DIR}/hardware/cl_manycore_pkg.v
${CL_DIR}/hardware/cl_manycore.sv
${CL_DIR}/hardware/bsg_bladerunner_configuration.v
#${CL_DIR}/../hdl/axil_to_mcl.v
#${CL_DIR}/../hdl/s_axil_mcl_adapter.v
#${CL_DIR}/../hdl/axil_to_mem.sv
${CL_DIR}/../hdl/bsg_manycore_link_to_axil.v
${CL_DIR}/../hdl/bsg_manycore_endpoint_to_fifos.v
${CL_DIR}/../hdl/bsg_axil_to_fifos.v
${CL_DIR}/../hdl/bsg_bladerunner_rom.v
${BSG_IP_CORES_DIR}/bsg_noc/bsg_noc_pkg.v
${BSG_IP_CORES_DIR}/bsg_noc/bsg_mesh_stitch.v
${BSG_IP_CORES_DIR}/bsg_noc/bsg_mesh_router_buffered.v
${BSG_IP_CORES_DIR}/bsg_noc/bsg_mesh_router.v

${BSG_IP_CORES_DIR}/bsg_cache/bsg_cache_pkg.v
${BSG_IP_CORES_DIR}/bsg_cache/bsg_cache_pkt_decode.v
${BSG_IP_CORES_DIR}/bsg_cache/bsg_manycore_link_to_cache.v
${BSG_IP_CORES_DIR}/bsg_cache/bsg_cache_to_axi_rx.v
${BSG_IP_CORES_DIR}/bsg_cache/bsg_cache_to_axi_tx.v
${BSG_IP_CORES_DIR}/bsg_cache/bsg_cache_to_axi.v
${BSG_IP_CORES_DIR}/bsg_cache/bsg_cache_dma.v
${BSG_IP_CORES_DIR}/bsg_cache/bsg_cache_miss.v
${BSG_IP_CORES_DIR}/bsg_cache/bsg_cache_sbuf.v
${BSG_IP_CORES_DIR}/bsg_cache/bsg_cache_sbuf_queue.v
${BSG_IP_CORES_DIR}/bsg_cache/bsg_cache.v

${BSG_IP_CORES_DIR}/bsg_dataflow/bsg_round_robin_n_to_1.v
${BSG_IP_CORES_DIR}/bsg_dataflow/bsg_fifo_tracker.v
${BSG_IP_CORES_DIR}/bsg_dataflow/bsg_fifo_1r1w_small.v
${BSG_IP_CORES_DIR}/bsg_dataflow/bsg_serial_in_parallel_out.v
${BSG_IP_CORES_DIR}/bsg_dataflow/bsg_parallel_in_serial_out.v
${BSG_IP_CORES_DIR}/bsg_dataflow/bsg_two_fifo.v
${BSG_IP_CORES_DIR}/bsg_dataflow/bsg_fifo_1r1w_large.v
${BSG_IP_CORES_DIR}/bsg_dataflow/bsg_fifo_1rw_large.v
${BSG_IP_CORES_DIR}/bsg_dataflow/bsg_round_robin_2_to_2.v


${BSG_IP_CORES_DIR}/bsg_misc/bsg_decode.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_decode_with_v.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_counter_clear_up.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_counter_up_down.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_circular_ptr.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_mux_segmented.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_thermometer_count.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_round_robin_arb.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_crossbar_o_by_i.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_mux.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_mux_one_hot.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_imul_iterative.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_idiv_iterative.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_idiv_iterative_controller.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_buf.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_buf_ctrl.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_dff_en.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_dff_reset.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_xnor.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_nor2.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_adder_cin.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_transpose.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_arb_fixed.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_priority_encode_one_hot_out.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_scan.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_dlatch.v
${BSG_IP_CORES_DIR}/bsg_misc/bsg_clkgate_optional.v

${BSG_IP_CORES_DIR}/bsg_mem/bsg_mem_banked_crossbar.v
${BSG_IP_CORES_DIR}/bsg_mem/bsg_mem_1r1w_synth.v
${BSG_IP_CORES_DIR}/bsg_mem/bsg_mem_1r1w.v
${BSG_IP_CORES_DIR}/bsg_mem/bsg_mem_1rw_sync_mask_write_bit_synth.v
${BSG_IP_CORES_DIR}/bsg_mem/bsg_mem_1rw_sync_mask_write_byte_synth.v
${BSG_IP_CORES_DIR}/bsg_mem/bsg_mem_1rw_sync_mask_write_bit.v
${BSG_IP_CORES_DIR}/bsg_mem/bsg_mem_1rw_sync_mask_write_byte.v
${BSG_IP_CORES_DIR}/bsg_mem/bsg_mem_1rw_sync.v
${BSG_IP_CORES_DIR}/bsg_mem/bsg_mem_1rw_sync_synth.v
${BSG_IP_CORES_DIR}/bsg_mem/bsg_mem_2r1w_sync.v
${BSG_IP_CORES_DIR}/bsg_mem/bsg_mem_2r1w_sync_synth.v


${BSG_MANYCORE_DIR}/v/vanilla_bean/bsg_manycore_proc_vanilla.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/alu.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/cl_decode.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/regfile.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/scoreboard.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/icache.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/imul_idiv_iterative.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/load_packer.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/hobbit.v

${BSG_MANYCORE_DIR}/v/bsg_manycore_endpoint_standard.v
${BSG_MANYCORE_DIR}/v/bsg_manycore_endpoint.v
${BSG_MANYCORE_DIR}/v/bsg_manycore_lock_ctrl.v
${BSG_MANYCORE_DIR}/v/bsg_1hold.v
${BSG_MANYCORE_DIR}/v/bsg_manycore_pkt_encode.v
${BSG_MANYCORE_DIR}/v/bsg_manycore_link_sif_tieoff.v
${BSG_MANYCORE_DIR}/v/bsg_manycore_mesh_node.v
${BSG_MANYCORE_DIR}/v/bsg_manycore_hetero_socket.v
${BSG_MANYCORE_DIR}/v/bsg_manycore_tile.v
${BSG_MANYCORE_DIR}/v/bsg_manycore.v

${BSG_MANYCORE_DIR}/v/bladerunner/bsg_manycore_wrapper.v
${BSG_MANYCORE_DIR}/v/bladerunner/bsg_cache_wrapper_axi.v

# Vivado library files
#${XILINX_VIVADO}/data/ip/xilinx/ila_v6_2/hdl/ila_v6_2_syn_rfs.v

${XILINX_VIVADO}/data/ip/xilinx/axi_crossbar_v2_1/hdl/axi_crossbar_v2_1_vl_rfs.v
${XILINX_VIVADO}/data/ip/xilinx/generic_baseblocks_v2_1/hdl/generic_baseblocks_v2_1_vl_rfs.v
${XILINX_VIVADO}/data/ip/xilinx/axi_data_fifo_v2_1/hdl/axi_data_fifo_v2_1_vl_rfs.v
${XILINX_VIVADO}/data/ip/xilinx/axi_register_slice_v2_1/hdl/axi_register_slice_v2_1_vl_rfs.v
${XILINX_VIVADO}/data/ip/xilinx/axi_dwidth_converter_v2_1/hdl/axi_dwidth_converter_v2_1_vl_rfs.v

# AWS design files
${HDK_SHELL_DESIGN_DIR}/ip/ila_vio_counter/sim/ila_vio_counter.v
${HDK_SHELL_DESIGN_DIR}/ip/ila_0/sim/ila_0.v
${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/sim/bd_a493.v
${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/ip/ip_0/sim/bd_a493_xsdbm_0.v
${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/ip/ip_0/hdl/xsdbm_v3_0_vl_rfs.v
${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/ip/ip_0/hdl/ltlib_v1_0_vl_rfs.v
${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/ip/ip_1/sim/bd_a493_lut_buffer_0.v
${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/ip/ip_1/hdl/lut_buffer_v2_0_vl_rfs.v
${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/hdl/bd_a493_wrapper.v
${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/sim/cl_debug_bridge.v
${HDK_SHELL_DESIGN_DIR}/ip/vio_0/sim/vio_0.v
${HDK_SHELL_DESIGN_DIR}/ip/axi_register_slice_light/sim/axi_register_slice_light.v
${HDK_SHELL_DESIGN_DIR}/ip/axi_register_slice/sim/axi_register_slice.v
${HDK_SHELL_DESIGN_DIR}/ip/axi_register_slice_light/hdl/axi_register_slice_v2_1_vl_rfs.v
${HDK_SHELL_DESIGN_DIR}/ip/axi_register_slice_light/hdl/axi_infrastructure_v1_1_vl_rfs.v
${HDK_SHELL_DESIGN_DIR}/ip/axi_clock_converter_0/hdl/axi_clock_converter_v2_1_vl_rfs.v
${HDK_SHELL_DESIGN_DIR}/ip/axi_clock_converter_0/hdl/fifo_generator_v13_2_rfs.v
${HDK_SHELL_DESIGN_DIR}/ip/axi_clock_converter_0/sim/axi_clock_converter_0.v

# Simulator-specific design files
-f ${HDK_COMMON_DIR}/verif/tb/filelists/tb.${SIMULATOR}.f

# Testbench design files
# Should be added in separate folder

