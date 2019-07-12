+define+VCS_SIM
+define+CARD_1=card

+libext+.v
+libext+.vh
+libext+.sv
+libext+.svh

# Custom Logic (CL) source directories
-y ${CL_DIR}/hardware

# AWS source library directories
-y ${HDK_SHELL_DESIGN_DIR}/lib
-y ${HDK_SHELL_DESIGN_DIR}/interfaces
-y ${HDK_SHELL_DESIGN_DIR}/sh_ddr/sim
-y ${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/hdl
-y ${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/sim

# AWS include directories
+incdir+${HDK_SHELL_DIR}/hlx/verif
+incdir+${HDK_SHELL_DESIGN_DIR}/lib
+incdir+${HDK_SHELL_DESIGN_DIR}/interfaces
+incdir+${HDK_SHELL_DESIGN_DIR}/sh_ddr/sim
+incdir+${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/ip/ip_0/sim
+incdir+${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/ip/ip_0/hdl/verilog
+incdir+${HDK_SHELL_DESIGN_DIR}/ip/axi_register_slice/hdl
+incdir+${HDK_SHELL_DESIGN_DIR}/ip/axi_register_slice_light/hdl

# Custom Logic (CL) include directories
+incdir+${BASEJUMP_STL_DIR}/bsg_misc
+incdir+${BASEJUMP_STL_DIR}/bsg_noc
+incdir+${BASEJUMP_STL_DIR}/bsg_cache
+incdir+${BSG_MANYCORE_DIR}/v/
+incdir+${BSG_MANYCORE_DIR}/v/vanilla_bean
+incdir+${CL_DIR}/../hdl/
+incdir+${CL_DIR}/hardware
+incdir+${CL_DIR}/testbenches

# Custom Logic (CL) design files
${CL_DIR}/hardware/bsg_bladerunner_configuration.v
${CL_DIR}/hardware/cl_manycore_pkg.v
${CL_DIR}/hardware/cl_manycore.sv
${CL_DIR}/../hdl/bsg_bladerunner_rom.v
${CL_DIR}/../hdl/axil_to_mcl.v
${CL_DIR}/../hdl/s_axil_mcl_adapter.v
${CL_DIR}/../hdl/axil_to_mem.sv

${BASEJUMP_STL_DIR}/bsg_noc/bsg_noc_pkg.v
${BASEJUMP_STL_DIR}/bsg_noc/bsg_mesh_stitch.v
${BASEJUMP_STL_DIR}/bsg_noc/bsg_mesh_router_buffered.v
${BASEJUMP_STL_DIR}/bsg_noc/bsg_mesh_router.v

${BASEJUMP_STL_DIR}/bsg_cache/bsg_cache_pkg.v
${BASEJUMP_STL_DIR}/bsg_cache/bsg_cache_pkt_decode.v
${BASEJUMP_STL_DIR}/bsg_cache/bsg_manycore_link_to_cache.v
${BASEJUMP_STL_DIR}/bsg_cache/bsg_cache_to_axi_rx.v
${BASEJUMP_STL_DIR}/bsg_cache/bsg_cache_to_axi_tx.v
${BASEJUMP_STL_DIR}/bsg_cache/bsg_cache_dma.v
${BASEJUMP_STL_DIR}/bsg_cache/bsg_cache_miss.v
${BASEJUMP_STL_DIR}/bsg_cache/bsg_cache_sbuf.v
${BASEJUMP_STL_DIR}/bsg_cache/bsg_cache_sbuf_queue.v
${BASEJUMP_STL_DIR}/bsg_cache/bsg_cache.v

${BASEJUMP_STL_DIR}/bsg_dataflow/bsg_round_robin_n_to_1.v
${BASEJUMP_STL_DIR}/bsg_dataflow/bsg_fifo_tracker.v
${BASEJUMP_STL_DIR}/bsg_dataflow/bsg_fifo_1r1w_small.v
${BASEJUMP_STL_DIR}/bsg_dataflow/bsg_serial_in_parallel_out.v
${BASEJUMP_STL_DIR}/bsg_dataflow/bsg_parallel_in_serial_out.v
${BASEJUMP_STL_DIR}/bsg_dataflow/bsg_two_fifo.v
${BASEJUMP_STL_DIR}/bsg_dataflow/bsg_fifo_1r1w_large.v
${BASEJUMP_STL_DIR}/bsg_dataflow/bsg_fifo_1rw_large.v
${BASEJUMP_STL_DIR}/bsg_dataflow/bsg_round_robin_2_to_2.v

${BASEJUMP_STL_DIR}/bsg_misc/bsg_decode.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_decode_with_v.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_counter_clear_up.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_counter_up_down.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_circular_ptr.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_mux_segmented.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_thermometer_count.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_round_robin_arb.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_crossbar_o_by_i.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_mux.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_mux_one_hot.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_imul_iterative.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_idiv_iterative.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_idiv_iterative_controller.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_buf.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_buf_ctrl.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_dff_en.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_dff_reset.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_xnor.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_nor2.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_adder_cin.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_transpose.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_arb_fixed.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_priority_encode_one_hot_out.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_scan.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_dlatch.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_clkgate_optional.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_less_than.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_dff_en_bypass.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_dff.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_mul_synth.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_priority_encode.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_reduce.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_encode_one_hot.v
${BASEJUMP_STL_DIR}/bsg_misc/bsg_abs.v

${BASEJUMP_STL_DIR}/bsg_mem/bsg_mem_banked_crossbar.v
${BASEJUMP_STL_DIR}/bsg_mem/bsg_mem_1r1w_synth.v
${BASEJUMP_STL_DIR}/bsg_mem/bsg_mem_1r1w.v
${BASEJUMP_STL_DIR}/bsg_mem/bsg_mem_1rw_sync_mask_write_bit_synth.v
${BASEJUMP_STL_DIR}/bsg_mem/bsg_mem_1rw_sync_mask_write_byte_synth.v
${BASEJUMP_STL_DIR}/bsg_mem/bsg_mem_1rw_sync_mask_write_bit.v
${BASEJUMP_STL_DIR}/bsg_mem/bsg_mem_1rw_sync_mask_write_byte.v
${BASEJUMP_STL_DIR}/bsg_mem/bsg_mem_1rw_sync.v
${BASEJUMP_STL_DIR}/bsg_mem/bsg_mem_1rw_sync_synth.v
${BASEJUMP_STL_DIR}/bsg_mem/bsg_mem_2r1w_sync.v
${BASEJUMP_STL_DIR}/bsg_mem/bsg_mem_2r1w_sync_synth.v

${BASEJUMP_STL_DIR}/bsg_fpu/bsg_fpu_pkg.v
${BASEJUMP_STL_DIR}/bsg_fpu/bsg_fpu_add_sub.v
${BASEJUMP_STL_DIR}/bsg_fpu/bsg_fpu_classify.v
${BASEJUMP_STL_DIR}/bsg_fpu/bsg_fpu_clz.v
${BASEJUMP_STL_DIR}/bsg_fpu/bsg_fpu_cmp.v
${BASEJUMP_STL_DIR}/bsg_fpu/bsg_fpu_f2i.v
${BASEJUMP_STL_DIR}/bsg_fpu/bsg_fpu_i2f.v
${BASEJUMP_STL_DIR}/bsg_fpu/bsg_fpu_mul.v
${BASEJUMP_STL_DIR}/bsg_fpu/bsg_fpu_preprocess.v
${BASEJUMP_STL_DIR}/bsg_fpu/bsg_fpu_sticky.v

${BSG_MANYCORE_DIR}/v/vanilla_bean/bsg_manycore_proc_vanilla.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/alu.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/cl_decode.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/regfile.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/scoreboard.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/icache.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/imul_idiv_iterative.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/load_packer.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/network_rx.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/network_tx.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/lsu.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/fpu_int.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/fpu_float.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/fpu_float_aux.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/vanilla_core.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/hash_function.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/hash_function_reverse.v
${BSG_MANYCORE_DIR}/v/vanilla_bean/bsg_cache_to_axi_hashed.v

${BSG_MANYCORE_DIR}/v/bsg_manycore_endpoint_standard.v
${BSG_MANYCORE_DIR}/v/bsg_manycore_endpoint.v
${BSG_MANYCORE_DIR}/v/bsg_manycore_lock_ctrl.v
${BSG_MANYCORE_DIR}/v/bsg_1hold.v
${BSG_MANYCORE_DIR}/v/bsg_manycore_link_sif_tieoff.v
${BSG_MANYCORE_DIR}/v/bsg_manycore_mesh_node.v
${BSG_MANYCORE_DIR}/v/bsg_manycore_hetero_socket.v
${BSG_MANYCORE_DIR}/v/bsg_manycore_tile.v
${BSG_MANYCORE_DIR}/v/bsg_manycore.v

${BSG_MANYCORE_DIR}/testbenches/common/v/vanilla_core_trace.v

${CL_DIR}/hardware/bsg_manycore_wrapper.v
${CL_DIR}/hardware/bsg_cache_wrapper_axi.v

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
-f ${HDK_COMMON_DIR}/verif/tb/filelists/tb.vcs.f
