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
-include ${HDK_COMMON_DIR}/verif/include
-include ${HDK_SHELL_DESIGN_DIR}/lib
-include ${HDK_SHELL_DESIGN_DIR}/interfaces
-include ${HDK_SHELL_DESIGN_DIR}/sh_ddr/sim
-include ${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/ip/ip_0/sim
-include ${HDK_SHELL_DESIGN_DIR}/ip/cl_debug_bridge/bd_0/ip/ip_0/hdl/verilog
-include ${HDK_SHELL_DESIGN_DIR}/ip/axi_register_slice/hdl
-include ${HDK_SHELL_DESIGN_DIR}/ip/axi_register_slice_light/hdl

# Custom Logic (CL) design files
${CL_DIR}/hardware/cl_fsb_bus_pkg.sv
${CL_DIR}/hardware/s_axil_fsb_adapter.sv
${CL_DIR}/hardware/s_axi4_fsb_adapter.sv
${CL_DIR}/hardware/m_axi4_fsb_adapter.sv
${CL_DIR}/hardware/m_axi4_axis_adapter.sv
${CL_DIR}/hardware/cl_to_axi4_adapter.sv
${CL_DIR}/hardware/cl_axis_test_master.sv
${CL_DIR}/hardware/cl_ocl_slv.sv
${CL_DIR}/hardware/cl_fsb.sv
${CL_DIR}/../../bsg_ip_cores/bsg_misc/bsg_defines.v
${CL_DIR}/../../bsg_ip_cores/bsg_fsb/bsg_fsb_pkg.v
${CL_DIR}/../../bsg_ip_cores/bsg_dataflow/bsg_two_fifo.v
${CL_DIR}/../../bsg_ip_cores/bsg_mem/bsg_mem_1r1w_synth.v
${CL_DIR}/../../bsg_ip_cores/bsg_mem/bsg_mem_1r1w.v
${CL_DIR}/../../bsg_designs/modules/bsg_guts/loopback/bsg_test_node_client.v
${CL_DIR}/../../bsg_ip_cores/bsg_test/test_bsg_data_gen.v
${CL_DIR}/../../bsg_designs/modules/bsg_guts/loopback/bsg_test_node_master.v

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

${HDK_SHELL_DESIGN_DIR}/ip/cl_axi_interconnect/ipshared/c631/hdl/axi_crossbar_v2_1_vl_rfs.v
${HDK_SHELL_DESIGN_DIR}/ip/cl_axi_interconnect/ipshared/9909/hdl/axi_data_fifo_v2_1_vl_rfs.v

# Simulator-specific design files
-f ${HDK_COMMON_DIR}/verif/tb/filelists/tb.${SIMULATOR}.f

# Testbench design files
${CL_DIR}/testbenches/cosim/cosim_wrapper.sv
