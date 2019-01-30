// Amazon FPGA Hardware Development Kit
//
// Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Amazon Software License (the "License"). You may not use
// this file except in compliance with the License. A copy of the License is
// located at
//
//    http://aws.amazon.com/asl/
//
// or in the "license" file accompanying this file. This file is distributed on
// an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, express or
// implied. See the License for the specific language governing permissions and
// limitations under the License.

`define FSB_LEGACY
`include "bsg_fsb_pkg.v"
// `undef FSB_LEGACY

module cl_fsb

(
   `include "cl_ports.vh" // Fixed port definition

);

`include "cl_common_defines.vh"     // CL Defines for all examples
`include "cl_id_defines.vh"         // Defines for ID0 and ID1 (PCI ID's)
`include "cl_fsb_defines.vh"        // CL Defines for cl_fsb


//---------------------------------------------
// Start with Tie-Off of Unused Interfaces
//---------------------------------------------
// the developer should use the next set of `include
// to properly tie-off any unused interface
// The list is put in the top of the module
// to avoid cases where developer may forget to
// remove it from the end of the file

// `include "unused_flr_template.inc" 
// Function level reset done indication is handled by CL
`include "unused_ddr_a_b_d_template.inc"
`include "unused_ddr_c_template.inc"
// `include "unused_pcim_template.inc"
// `include "unused_dma_pcis_template.inc"
`include "unused_cl_sda_template.inc"
`include "unused_sh_bar1_template.inc"
`include "unused_apppf_irq_template.inc"

`include "bsg_axi_bus_pkg.vh"

// `undef FSB_LEGACY
//-------------------------------------------------
// global signals
//-------------------------------------------------
logic clk;
assign clk = clk_main_a0;

(* dont_touch = "true" *) logic pipe_rst_n;
logic pre_sync_rst_n;
(* dont_touch = "true" *) logic sync_rst_n;
logic sh_cl_flr_assert_q;


//reset synchronizer

// this is simple pipeline module provided by shell, with FPGA rst optimization --XL
lib_pipe #(.WIDTH(1), .STAGES(4)) PIPE_RST_N (
  .clk    (clk),
  .rst_n  (1'b1),
  .in_bus (rst_main_n),
  .out_bus(pipe_rst_n)
);

always_ff @(negedge pipe_rst_n or posedge clk)
  if (!pipe_rst_n)
    begin
      pre_sync_rst_n <= 0;
      sync_rst_n     <= 0;
    end
  else
    begin
      pre_sync_rst_n <= 1;
      sync_rst_n     <= pre_sync_rst_n;
    end

//FLR response
always_ff @(negedge sync_rst_n or posedge clk)
  if (!sync_rst_n)
    begin
      sh_cl_flr_assert_q <= 0;
      cl_sh_flr_done     <= 0;
    end
  else
    begin
      sh_cl_flr_assert_q <= sh_cl_flr_assert;
      cl_sh_flr_done     <= sh_cl_flr_assert_q && !cl_sh_flr_done;
    end


// Tie-Off Unused Global Signals
//-------------------------------------------
// The functionality for these signals is TBD so they can can be tied-off.
assign cl_sh_status0[31:0] = 32'h0;
assign cl_sh_status1[31:0] = 32'h0;


// ID Values (cl_fsb_defines.vh)
//-------------------------------------------------
assign cl_sh_id0[31:0] = `CL_SH_ID0;
assign cl_sh_id1[31:0] = `CL_SH_ID1;


localparam axil_slv_num_lp = 10;
localparam fsb_width_lp = 80;

// fsb slave
//-------------------------------------------------
logic [axil_slv_num_lp-1:0] m0_fsb_v_i, m0_fsb_v_o;
logic [(fsb_width_lp*axil_slv_num_lp)-1:0] m0_fsb_data_i, m0_fsb_data_o;
logic [axil_slv_num_lp-1:0] m0_fsb_yumi_o, m0_fsb_ready_i;

(* dont_touch = "true" *) logic axi_fsb_rstn;
lib_pipe #(.WIDTH(1), .STAGES(4)) AXI_FSB_RST_N (.clk(clk), .rst_n(1'b1), .in_bus(sync_rst_n), .out_bus(axi_fsb_rstn));
axil_to_mcl #(.axil_slv_num_p(10)) DUT (
  .clk_i         (clk         ),
  .reset_i       (~axi_fsb_rstn ),
  .sh_ocl_awvalid(sh_ocl_awvalid),
  .sh_ocl_awaddr (sh_ocl_awaddr ),
  .ocl_sh_awready(ocl_sh_awready),
  .sh_ocl_wvalid (sh_ocl_wvalid ),
  .sh_ocl_wdata  (sh_ocl_wdata  ),
  .sh_ocl_wstrb  (sh_ocl_wstrb  ),
  .ocl_sh_wready (ocl_sh_wready ),
  .ocl_sh_bresp  (ocl_sh_bresp  ),
  .ocl_sh_bvalid (ocl_sh_bvalid ),
  .sh_ocl_bready (sh_ocl_bready ),
  .sh_ocl_araddr (sh_ocl_araddr ),
  .sh_ocl_arvalid(sh_ocl_arvalid),
  .ocl_sh_arready(ocl_sh_arready),
  .ocl_sh_rdata  (ocl_sh_rdata  ),
  .ocl_sh_rresp  (ocl_sh_rresp  ),
  .ocl_sh_rvalid (ocl_sh_rvalid ),
  .sh_ocl_rready (sh_ocl_rready ),
  .m0_fsb_v_i    (m0_fsb_v_i    ),
  .m0_fsb_data_i (m0_fsb_data_i ),
  .m0_fsb_yumi_o (m0_fsb_yumi_o ),
  .m0_fsb_v_o    (m0_fsb_v_o    ),
  .m0_fsb_data_o (m0_fsb_data_o ),
  .m0_fsb_ready_i(m0_fsb_ready_i)
);

//---------------------------------------------------------------
//                    axi - fsb adapters                        |
//                                                              |
//---------------------------------------------------------------
genvar i;
for (i=0;i<axil_slv_num_lp;i=i+1)
begin: bsg_test_node_slv
  cl_simple_loopback #(
    .data_width_p(80        ),
    .mask_p      ({80{1'b1}})
  ) fsb_loopback_node (
    .clk_i  (clk                                        ),
    .reset_i(~axi_fsb_rstn                              ),
    .en_i   (1'b1                                       ),
    // input channel
    .v_i    (m0_fsb_v_o[i]                              ),
    .data_i (m0_fsb_data_o[fsb_width_lp*i+:fsb_width_lp]),
    .ready_o(m0_fsb_ready_i[i]                          ),
    // output channel
    .v_o    (m0_fsb_v_i[i]                              ),
    .data_o (m0_fsb_data_i[fsb_width_lp*i+:fsb_width_lp]),
    .yumi_i (m0_fsb_yumi_o[i]                           )
  );
end


// //---------------------------------------------------------------
// //                    axi - fsb adapters                        |
// //                                                              |
// //---------------------------------------------------------------
// `ifndef DISABLE_VJTAG_DEBUG

// // Flop for timing global clock counter
// logic[63:0] sh_cl_glcount0_q;

// always_ff @(posedge clk_main_a0)
//    if (!sync_rst_n)
//       sh_cl_glcount0_q <= 0;
//    else
//       sh_cl_glcount0_q <= sh_cl_glcount0;


//    ila_0 CL_ILA_0 (
//                    .clk    (clk_main_a0),
//                    .probe0 (sh_ocl_mosi_bus.awvalid),
//                    .probe1 ({sh_ocl_mosi_bus.wdata, sh_ocl_mosi_bus.awaddr}),
//                    .probe2 (sh_ocl_miso_bus.awready),
//                    .probe3 (sh_ocl_mosi_bus.wvalid),
//                    .probe4 ({sh_ocl_miso_bus.rdata, sh_ocl_mosi_bus.araddr}),
//                    .probe5 (sh_ocl_miso_bus.wready)
//                    );


// // // Integrated Logic Analyzers (ILA)
// //   cl_ila_axil axil_analyser (
// //     .clk    (clk_main_a0            ),
// //     .probe0 (sh_ocl_mosi_bus.awvalid),
// //     .probe1 (sh_ocl_mosi_bus.awaddr ),
// //     .probe2 (sh_ocl_miso_bus.awready),
// //     .probe3 (sh_ocl_mosi_bus.wvalid ),
// //     .probe4 (sh_ocl_mosi_bus.wdata  ),
// //     .probe5 (sh_ocl_mosi_bus
// //       .wstrb  ),
// //     .probe6 (sh_ocl_miso_bus.wready ),
// //     .probe7 (sh_ocl_miso_bus.bresp  ),
// //     .probe8 (sh_ocl_miso_bus.bvalid ),
// //     .probe9 (sh_ocl_mosi_bus.bready ),
// //     .probe10(sh_ocl_mosi_bus.araddr ),
// //     .probe11(sh_ocl_mosi_bus.arvalid),
// //     .probe12(sh_ocl_miso_bus.arready),
// //     .probe13(sh_ocl_miso_bus.rdata  ),
// //     .probe14(sh_ocl_miso_bus.rresp  ),
// //     .probe15(sh_ocl_miso_bus.rvalid ),
// //     .probe16(sh_ocl_rready          ),
// //     .probe17(0                      ),
// //     .probe18(0                      )
// //   );

// // Debug Bridge 
//  cl_debug_bridge CL_DEBUG_BRIDGE (
//       .clk(clk_main_a0),
//       .S_BSCAN_drck(drck),
//       .S_BSCAN_shift(shift),
//       .S_BSCAN_tdi(tdi),
//       .S_BSCAN_update(update),
//       .S_BSCAN_sel(sel),
//       .S_BSCAN_tdo(tdo),
//       .S_BSCAN_tms(tms),
//       .S_BSCAN_tck(tck),
//       .S_BSCAN_runtest(runtest),
//       .S_BSCAN_reset(reset),
//       .S_BSCAN_capture(capture),
//       .S_BSCAN_bscanid_en(bscanid_en)
//    );

// `endif //  `ifndef DISABLE_VJTAG_DEBUG

endmodule
