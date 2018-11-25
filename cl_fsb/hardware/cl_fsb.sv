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

module cl_fsb

(
   `include "cl_ports.vh" // Fixed port definition

);

`define TEST_AXI4_MASTER

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

// `include "unused_flr_template.inc" // Function level reset done indication is handled by CL
`include "unused_ddr_a_b_d_template.inc"
`include "unused_ddr_c_template.inc"
// `include "unused_pcim_template.inc"
`include "unused_dma_pcis_template.inc"
`include "unused_cl_sda_template.inc"
`include "unused_sh_bar1_template.inc"
`include "unused_apppf_irq_template.inc"


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
lib_pipe #(.WIDTH(1), .STAGES(4)) PIPE_RST_N (.clk(clk), .rst_n(1'b1), .in_bus(rst_main_n), .out_bus(pipe_rst_n));
always_ff @(negedge pipe_rst_n or posedge clk)
   if (!pipe_rst_n)
   begin
      pre_sync_rst_n <= 0;
      sync_rst_n <= 0;
   end
   else
   begin
      pre_sync_rst_n <= 1;
      sync_rst_n <= pre_sync_rst_n;
   end

//FLR response 
always_ff @(negedge sync_rst_n or posedge clk)
   if (!sync_rst_n)
   begin
      sh_cl_flr_assert_q <= 0;
      cl_sh_flr_done <= 0;
   end
   else
   begin
      sh_cl_flr_assert_q <= sh_cl_flr_assert;
      cl_sh_flr_done <= sh_cl_flr_assert_q && !cl_sh_flr_done;
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


//-------------------------------------------------
// PCIe OCL AXI-L (SH to CL, from AppPF BAR0)
//-------------------------------------------------
axi_bus_t sh_ocl_bus();

assign sh_ocl_bus.awvalid = sh_ocl_awvalid;
assign sh_ocl_bus.awaddr[31:0] = sh_ocl_awaddr;
assign ocl_sh_awready = sh_ocl_bus.awready;
assign sh_ocl_bus.wvalid = sh_ocl_wvalid;
assign sh_ocl_bus.wdata[31:0] = sh_ocl_wdata;
assign sh_ocl_bus.wstrb[3:0] = sh_ocl_wstrb;
assign ocl_sh_wready = sh_ocl_bus.wready;
assign ocl_sh_bvalid = sh_ocl_bus.bvalid;
assign ocl_sh_bresp = sh_ocl_bus.bresp;
assign sh_ocl_bus.bready = sh_ocl_bready;
assign sh_ocl_bus.arvalid = sh_ocl_arvalid;
assign sh_ocl_bus.araddr[31:0] = sh_ocl_araddr;
assign ocl_sh_arready = sh_ocl_bus.arready;
assign ocl_sh_rvalid = sh_ocl_bus.rvalid;
assign ocl_sh_rresp = sh_ocl_bus.rresp;
assign ocl_sh_rdata = sh_ocl_bus.rdata[31:0];
assign sh_ocl_bus.rready = sh_ocl_rready;


`ifdef TEST_AXI4_MASTER
//=================================================
// Test AXI4 master to SH
//=================================================

// generate *tst_cfg_us to control modules

cfg_bus_t pcim_tst_cfg_bus();

(* dont_touch = "true" *) logic ocl_slv_sync_rst_n;
lib_pipe #(.WIDTH(1), .STAGES(4)) OCL_SLV_SLC_RST_N (.clk(clk), .rst_n(1'b1), .in_bus(sync_rst_n), .out_bus(ocl_slv_sync_rst_n));
cl_ocl_slv CL_OCL_SLV (
   .clk(clk)
   ,.sync_rst_n(ocl_slv_sync_rst_n)

   ,.sh_cl_flr_assert_q(sh_cl_flr_assert_q)

   ,.sh_ocl_bus  (sh_ocl_bus)

   ,.pcim_tst_cfg_bus(pcim_tst_cfg_bus)
   // ,.ddra_tst_cfg_bus() // ddra_tst_cfg_bus
   // ,.ddrb_tst_cfg_bus() // ddrb_tst_cfg_bus
   // ,.ddrc_tst_cfg_bus() // ddrc_tst_cfg_bus
   // ,.ddrd_tst_cfg_bus() // ddrd_tst_cfg_bus
   // ,.axi_mstr_cfg_bus() // axi_mstr_cfg_bus
   // ,.int_tst_cfg_bus()  // int_tst_cfg_bus
);


//-------------------------------------------------
// PCIM AXI-4 (CL to SH)
//-------------------------------------------------
axi_bus_t cl_sh_pcim_bus();

assign cl_sh_pcim_awid = cl_sh_pcim_bus.awid;
assign cl_sh_pcim_awaddr = cl_sh_pcim_bus.awaddr;
assign cl_sh_pcim_awlen = cl_sh_pcim_bus.awlen;
assign cl_sh_pcim_awsize = cl_sh_pcim_bus.awsize;
assign cl_sh_pcim_awvalid = cl_sh_pcim_bus.awvalid;
assign cl_sh_pcim_bus.awready = sh_cl_pcim_awready;

assign cl_sh_pcim_wdata = cl_sh_pcim_bus.wdata;
assign cl_sh_pcim_wstrb = cl_sh_pcim_bus.wstrb;
assign cl_sh_pcim_wlast = cl_sh_pcim_bus.wlast;
assign cl_sh_pcim_wvalid = cl_sh_pcim_bus.wvalid;
assign cl_sh_pcim_bus.wready = sh_cl_pcim_wready;

assign cl_sh_pcim_bus.bid = sh_cl_pcim_bid;
assign cl_sh_pcim_bus.bresp = sh_cl_pcim_bresp;
assign cl_sh_pcim_bus.bvalid = sh_cl_pcim_bvalid;
assign cl_sh_pcim_bready = cl_sh_pcim_bus.bready;

assign cl_sh_pcim_arid = cl_sh_pcim_bus.arid;
assign cl_sh_pcim_araddr = cl_sh_pcim_bus.araddr;
assign cl_sh_pcim_arlen = cl_sh_pcim_bus.arlen;
assign cl_sh_pcim_arsize = cl_sh_pcim_bus.arsize;
assign cl_sh_pcim_arvalid = cl_sh_pcim_bus.arvalid;
assign cl_sh_pcim_bus.arready = sh_cl_pcim_arready;

assign cl_sh_pcim_bus.rid = sh_cl_pcim_rid;
assign cl_sh_pcim_bus.rdata = sh_cl_pcim_rdata;
assign cl_sh_pcim_bus.rresp = sh_cl_pcim_rresp;
assign cl_sh_pcim_bus.rlast = sh_cl_pcim_rlast;
assign cl_sh_pcim_bus.rvalid = sh_cl_pcim_rvalid;
assign cl_sh_pcim_rready = cl_sh_pcim_bus.rready;

// note: cl_sh_pcim_aruser/awuser are ignored by the shell
// and the axi4 size is set fixed for 64-bytes
//  cl_sh_pcim_arsize/awsize = 3'b6;

(* dont_touch = "true" *) logic pcim_mstr_sync_rst_n;
lib_pipe #(.WIDTH(1), .STAGES(4)) PCIM_MSTR_SLC_RST_N (.clk(clk), .rst_n(1'b1), .in_bus(sync_rst_n), .out_bus(pcim_mstr_sync_rst_n));

// cl_pcim_mstr CL_PCIM_MSTR (

//      .aclk(clk),
//      .aresetn(pcim_mstr_sync_rst_n),

//      .cfg_bus(pcim_tst_cfg_bus),

//      .cl_sh_pcim_bus(cl_sh_pcim_bus)
// );

  logic fsb_wvalid;
  logic [79:0] fsb_wdata;
  logic fsb_yumi;
  
bsg_test_node_master #(
  .ring_width_p(80)
  ,.master_id_p (4'hF)
  ,.client_id_p (4'hF)
) fsb_node_master (
  .clk_i  (clk),
  .reset_i(~pcim_mstr_sync_rst_n),
  .en_i   (1'b1),
  .v_i    (0),
  .data_i (80'h0),
  .ready_o(),
  .v_o    (fsb_wvalid),
  .data_o (fsb_wdata),
  .yumi_i (fsb_yumi)
);


m_axi4_fsb_adapter #(
  .DATA_WIDTH(512),
  .NUM_RD_TAG(512),
  .FSB_WIDTH (80 )
) m_axi4_fsb (
  .clk_i         (clk)
  ,.resetn_i      (pcim_mstr_sync_rst_n)
  ,.cfg_bus       (pcim_tst_cfg_bus)
  ,.cl_sh_pcim_bus(cl_sh_pcim_bus)
  ,.atg_dst_sel   ()
  ,.fsb_wvalid    (fsb_wvalid)
  ,.fsb_wdata     (fsb_wdata)
  ,.fsb_yumi      (fsb_yumi)
);

`else
//=================================================
// Test AXI-L Slave to CL
//=================================================

// Loopback data sent to fsb, only supports single-beat accesses
(* dont_touch = "true" *) logic ocl_slv_sync_rst_n;
lib_pipe #(.WIDTH(1), .STAGES(4)) S_AXIL_FSB_ADAPTER_RST_N (.clk(clk), .rst_n(1'b1), .in_bus(sync_rst_n), .out_bus(ocl_slv_sync_rst_n));

s_axil_fsb_adapter s_axil_fsb (
  .clk_i      (clk)
  ,.resetn_i  (ocl_slv_sync_rst_n)
  ,.sh_ocl_bus(sh_ocl_bus)
);

`endif

endmodule





