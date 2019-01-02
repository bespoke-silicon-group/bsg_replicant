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


# TODO:
# Add check if CL_DIR and HDK_SHELL_DIR directories exist
# Add check if /build and /build/src_port_encryption directories exist
# Add check if the vivado_keyfile exist

set HDK_SHELL_DIR $::env(HDK_SHELL_DIR)
set CL_DIR $::env(CL_DIR)
set BSG_IP_DIR $::env(BSG_IP_DIR)
set BSG_DESIGNS_DIR $::env(BSG_DESIGNS_DIR)
set VIVADO_IP_DIR $::env(XILINX_VIVADO)/data/ip/xilinx/

set TARGET_DIR $CL_DIR/build/src_post_encryption
set UNUSED_TEMPLATES_DIR $HDK_SHELL_DIR/design/interfaces

# Remove any previously encrypted files, that may no longer be used
if {[llength [glob -nocomplain -dir $TARGET_DIR *]] != 0} {
  eval file delete -force [glob $TARGET_DIR/*]
}

# List your design files below. Include any .inc files, but do NOT include
# AWS source files.
file copy -force $CL_DIR/hardware/cl_id_defines.vh                      $TARGET_DIR
file copy -force $CL_DIR/hardware/cl_fsb_defines.vh                     $TARGET_DIR
file copy -force $CL_DIR/hardware/cl_common_defines.vh                  $TARGET_DIR

file copy -force $CL_DIR/hardware/cl_fsb.sv                             $TARGET_DIR
file copy -force $CL_DIR/hardware/cl_fsb_bus_pkg.sv                     $TARGET_DIR
file copy -force $CL_DIR/hardware/s_axil_m_fsb_adapter.sv               $TARGET_DIR
file copy -force $CL_DIR/hardware/s_axi4_m_fsb_adapter.sv               $TARGET_DIR
file copy -force $CL_DIR/hardware/m_axi4_s_fsb_adapter.sv               $TARGET_DIR
file copy -force $CL_DIR/hardware/m_axi4_s_axis_adapter.sv              $TARGET_DIR
file copy -force $CL_DIR/hardware/bsg_axis_gen_master.sv                $TARGET_DIR

# BSG files
file copy -force $BSG_IP_DIR/bsg_misc/bsg_defines.v                                                 $TARGET_DIR
file copy -force $BSG_IP_DIR/bsg_fsb/bsg_fsb_pkg.v                                                  $TARGET_DIR

file copy -force $BSG_IP_DIR//bsg_misc/bsg_circular_ptr.v                                           $TARGET_DIR
file copy -force $BSG_IP_DIR/bsg_mem/bsg_mem_1r1w_synth.v                                           $TARGET_DIR
file copy -force $BSG_IP_DIR/bsg_mem/bsg_mem_1r1w.v                                                 $TARGET_DIR
file copy -force $BSG_IP_DIR/bsg_dataflow/bsg_fifo_1r1w_small.v                                     $TARGET_DIR
file copy -force $BSG_IP_DIR/bsg_dataflow/bsg_fifo_tracker.v                                        $TARGET_DIR
file copy -force $BSG_IP_DIR/bsg_dataflow/bsg_two_fifo.v                                            $TARGET_DIR
file copy -force $BSG_IP_DIR/bsg_test/test_bsg_data_gen.v                                           $TARGET_DIR

file copy -force $BSG_DESIGNS_DIR/modules/bsg_guts/loopback/bsg_test_node_client.v                  $TARGET_DIR
file copy -force $BSG_DESIGNS_DIR/modules/bsg_guts/loopback/bsg_test_node_master.v                  $TARGET_DIR

# Vivado library files
file copy -force $VIVADO_IP_DIR/generic_baseblocks_v2_1/hdl/generic_baseblocks_v2_1_vl_rfs.v        $TARGET_DIR
file copy -force $VIVADO_IP_DIR/axi_register_slice_v2_1/hdl/axi_register_slice_v2_1_vl_rfs.v        $TARGET_DIR
file copy -force $VIVADO_IP_DIR/axi_data_fifo_v2_1/hdl/axi_data_fifo_v2_1_vl_rfs.v                  $TARGET_DIR
file copy -force $VIVADO_IP_DIR/axi_dwidth_converter_v2_1/hdl/axi_dwidth_converter_v2_1_vlsyn_rfs.v $TARGET_DIR
file copy -force $VIVADO_IP_DIR/axi_crossbar_v2_1/hdl/axi_crossbar_v2_1_vl_rfs.v                    $TARGET_DIR

file copy -force $VIVADO_IP_DIR/axi_infrastructure_v1_1/hdl/axi_infrastructure_v1_1_0.vh            $TARGET_DIR
file copy -force $VIVADO_IP_DIR/axi_infrastructure_v1_1/hdl/axi_infrastructure_v1_1_vl_rfs.v        $TARGET_DIR
file copy -force $VIVADO_IP_DIR/axi_fifo_mm_s_v4_1/hdl/axi_fifo_mm_s_v4_1_rfs.vhd                   $TARGET_DIR
file copy -force $VIVADO_IP_DIR/axi_lite_ipif_v3_0/hdl/axi_lite_ipif_v3_0_vh_rfs.vhd                $TARGET_DIR

file copy -force $VIVADO_IP_DIR/blk_mem_gen_v8_4/hdl/blk_mem_gen_v8_4_vhsyn_rfs.vhd                 $TARGET_DIR
file copy -force $VIVADO_IP_DIR/lib_pkg_v1_0/hdl/lib_pkg_v1_0_rfs.vhd                               $TARGET_DIR
file copy -force $VIVADO_IP_DIR/lib_fifo_v1_0/hdl/lib_fifo_v1_0_rfs.vhd                             $TARGET_DIR

file copy -force $VIVADO_IP_DIR/fifo_generator_v13_2/hdl/fifo_generator_v13_2_vhsyn_rfs.vhd         $TARGET_DIR
file copy -force $VIVADO_IP_DIR/axis_register_slice_v1_1/hdl/axis_register_slice_v1_1_vl_rfs.v      $TARGET_DIR
file copy -force $VIVADO_IP_DIR/axis_infrastructure_v1_1/hdl/axis_infrastructure_v1_1_0.vh          $TARGET_DIR
file copy -force $VIVADO_IP_DIR/axis_infrastructure_v1_1/hdl/axis_infrastructure_v1_1_vl_rfs.v      $TARGET_DIR
file copy -force $VIVADO_IP_DIR/axis_data_fifo_v1_1/hdl/axis_data_fifo_v1_1_vl_rfs.v                $TARGET_DIR
file copy -force $VIVADO_IP_DIR/axis_dwidth_converter_v1_1/hdl/axis_dwidth_converter_v1_1_vl_rfs.v  $TARGET_DIR

file copy -force $UNUSED_TEMPLATES_DIR/unused_apppf_irq_template.inc  $TARGET_DIR
file copy -force $UNUSED_TEMPLATES_DIR/unused_cl_sda_template.inc     $TARGET_DIR
file copy -force $UNUSED_TEMPLATES_DIR/unused_ddr_a_b_d_template.inc  $TARGET_DIR
file copy -force $UNUSED_TEMPLATES_DIR/unused_ddr_c_template.inc      $TARGET_DIR
file copy -force $UNUSED_TEMPLATES_DIR/unused_dma_pcis_template.inc   $TARGET_DIR
file copy -force $UNUSED_TEMPLATES_DIR/unused_pcim_template.inc       $TARGET_DIR
file copy -force $UNUSED_TEMPLATES_DIR/unused_sh_bar1_template.inc    $TARGET_DIR
file copy -force $UNUSED_TEMPLATES_DIR/unused_flr_template.inc        $TARGET_DIR
# End of design files

# Make sure files have write permissions for the encryption
exec chmod +w {*}[glob $TARGET_DIR/*]

# encrypt .v/.sv/.vh/inc as verilog files
# encrypt -k $HDK_SHELL_DIR/build/scripts/vivado_keyfile.txt -lang verilog  [glob -nocomplain -- $TARGET_DIR/*.?v] [glob -nocomplain -- $TARGET_DIR/*.vh] [glob -nocomplain -- $TARGET_DIR/*.inc]

# encrypt *vhdl files
# encrypt -k $HDK_SHELL_DIR/build/scripts/vivado_vhdl_keyfile.txt -lang vhdl -quiet [ glob -nocomplain -- $TARGET_DIR/*.vhd? ]

