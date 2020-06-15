+define+CARD_1=card

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
