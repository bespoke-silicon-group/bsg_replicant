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

// `include "unused_flr_template.inc" 
// Function level reset done indication is handled by CL
`include "unused_ddr_a_b_d_template.inc"
`include "unused_ddr_c_template.inc"
// `include "unused_pcim_template.inc"
// `include "unused_dma_pcis_template.inc"
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


//-------------------------------------------------
// PCIe OCL AXI-L (SH to CL, from AppPF BAR0)
//-------------------------------------------------
axil_bus_t sh_ocl_bus();

assign sh_ocl_bus.awvalid      = sh_ocl_awvalid;
assign sh_ocl_bus.awaddr[31:0] = sh_ocl_awaddr;
assign ocl_sh_awready          = sh_ocl_bus.awready;
assign sh_ocl_bus.wvalid       = sh_ocl_wvalid;
assign sh_ocl_bus.wdata[31:0]  = sh_ocl_wdata;
assign sh_ocl_bus.wstrb[3:0]   = sh_ocl_wstrb;
assign ocl_sh_wready           = sh_ocl_bus.wready;
assign ocl_sh_bvalid           = sh_ocl_bus.bvalid;
assign ocl_sh_bresp            = sh_ocl_bus.bresp;
assign sh_ocl_bus.bready       = sh_ocl_bready;
assign sh_ocl_bus.arvalid      = sh_ocl_arvalid;
assign sh_ocl_bus.araddr[31:0] = sh_ocl_araddr;
assign ocl_sh_arready          = sh_ocl_bus.arready;
assign ocl_sh_rvalid           = sh_ocl_bus.rvalid;
assign ocl_sh_rresp            = sh_ocl_bus.rresp;
assign ocl_sh_rdata            = sh_ocl_bus.rdata[31:0];
assign sh_ocl_bus.rready       = sh_ocl_rready;


axil_bus_t #(.NUM_SLOTS(1)) sh_ocl_mux00();
axil_bus_t #(.NUM_SLOTS(1)) sh_ocl_mux01();
axil_bus_t #(.NUM_SLOTS(1)) sh_ocl_mux02();
axil_bus_t #(.NUM_SLOTS(1)) sh_ocl_mux03();

(* dont_touch = "true" *) logic axi_crossbar_sync_rst_n;
lib_pipe #(.WIDTH(1), .STAGES(4)) AXI_CROSSBAR_RST_N (
  .clk    (clk)
  ,.rst_n  (1'b1)
  ,.in_bus (sync_rst_n)
  ,.out_bus(axi_crossbar_sync_rst_n)
);

// s_axil_crossbar_1_2 axil_s1_m2 (
//   .aclk          (clk)
//   ,.aresetn       (axi_crossbar_sync_rst_n)
//   ,.axil_m_bus    (sh_ocl_bus)
//   ,.axil_s_m00_bus(sh_ocl_mux00)
//   ,.axil_s_m01_bus(sh_ocl_mux01)
// );

s_axil_crossbar_1_4 axil_s1_m4 (
  .aclk          (clk)
  ,.aresetn       (axi_crossbar_sync_rst_n)
  ,.axil_m_bus    (sh_ocl_bus)
  ,.axil_s_m00_bus(sh_ocl_mux00) // -> s_axil_fsb
  ,.axil_s_m01_bus(sh_ocl_mux01) // -> cl_ocl_slv -> m_axi4_fsb_cfg
  ,.axil_s_m02_bus(sh_ocl_mux02) // -> s_axi4_fsb_cfg
  ,.axil_s_m03_bus(sh_ocl_mux03) // -> to be determined
);

// m_axi4_crossbar_2_1 axi4_m2_s1 ();

//=================================================
// FSB master to AXI-4 
//=================================================

// control module
// --------------------------------------------
cfg_bus_t pcim_tst_cfg_bus ();

(* dont_touch = "true" *) logic ocl_cfg_sync_rst_n;
lib_pipe #(.WIDTH(1), .STAGES(4)) OCL_CFG_SLC_RST_N (
  .clk    (clk)
  ,.rst_n  (1'b1)
  ,.in_bus (sync_rst_n)
  ,.out_bus(ocl_cfg_sync_rst_n)
);

cl_ocl_slv CL_OCL_SLV (
  .clk               (clk)
  ,.sync_rst_n        (ocl_cfg_sync_rst_n)
  ,.sh_cl_flr_assert_q(sh_cl_flr_assert_q)
  ,.sh_ocl_bus        (sh_ocl_mux01)
  ,.pcim_tst_cfg_bus  (pcim_tst_cfg_bus)
);


axi_bus_t cl_sh_pcim_bus();

assign cl_sh_pcim_awid        = cl_sh_pcim_bus.awid;
assign cl_sh_pcim_awaddr      = cl_sh_pcim_bus.awaddr;
assign cl_sh_pcim_awlen       = cl_sh_pcim_bus.awlen;
assign cl_sh_pcim_awsize      = cl_sh_pcim_bus.awsize;
assign cl_sh_pcim_awvalid     = cl_sh_pcim_bus.awvalid;
assign cl_sh_pcim_bus.awready = sh_cl_pcim_awready;

assign cl_sh_pcim_wdata       = cl_sh_pcim_bus.wdata;
assign cl_sh_pcim_wstrb       = cl_sh_pcim_bus.wstrb;
assign cl_sh_pcim_wlast       = cl_sh_pcim_bus.wlast;
assign cl_sh_pcim_wvalid      = cl_sh_pcim_bus.wvalid;
assign cl_sh_pcim_bus.wready  = sh_cl_pcim_wready;

assign cl_sh_pcim_bus.bid     = sh_cl_pcim_bid;
assign cl_sh_pcim_bus.bresp   = sh_cl_pcim_bresp;
assign cl_sh_pcim_bus.bvalid  = sh_cl_pcim_bvalid;
assign cl_sh_pcim_bready      = cl_sh_pcim_bus.bready;

assign cl_sh_pcim_arid        = cl_sh_pcim_bus.arid;
assign cl_sh_pcim_araddr      = cl_sh_pcim_bus.araddr;
assign cl_sh_pcim_arlen       = cl_sh_pcim_bus.arlen;
assign cl_sh_pcim_arsize      = cl_sh_pcim_bus.arsize;
assign cl_sh_pcim_arvalid     = cl_sh_pcim_bus.arvalid;
assign cl_sh_pcim_bus.arready = sh_cl_pcim_arready;

assign cl_sh_pcim_bus.rid     = sh_cl_pcim_rid;
assign cl_sh_pcim_bus.rdata   = sh_cl_pcim_rdata;
assign cl_sh_pcim_bus.rresp   = sh_cl_pcim_rresp;
assign cl_sh_pcim_bus.rlast   = sh_cl_pcim_rlast;
assign cl_sh_pcim_bus.rvalid  = sh_cl_pcim_rvalid;
assign cl_sh_pcim_rready      = cl_sh_pcim_bus.rready;

// note: cl_sh_pcim_aruser/awuser are ignored by the shell
// and the axi4 size is set fixed for 64-bytes
//  cl_sh_pcim_arsize/awsize = 3'b6;

(* dont_touch = "true" *) logic pcim_mstr_sync_rst_n;
lib_pipe #(.WIDTH(1), .STAGES(4)) PCIM_MSTR_SLC_RST_N (
  .clk    (clk)
  ,.rst_n  (1'b1)
  ,.in_bus (sync_rst_n)
  ,.out_bus(pcim_mstr_sync_rst_n)
);

  logic fsb_wvalid;
  logic [79:0] fsb_wdata;
  logic fsb_yumi;

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

bsg_test_node_master #(
  .ring_width_p(80)
  ,.master_id_p (4'hF)
  ,.client_id_p (4'hF)
) fsb_node_master (
  .clk_i  (clk)
  ,.reset_i(~pcim_mstr_sync_rst_n)
  ,.en_i   (1'b1)
  ,.v_i    (0)
  ,.data_i (80'h0)
  ,.ready_o()
  ,.v_o    (fsb_wvalid)
  ,.data_o (fsb_wdata)
  ,.yumi_i (fsb_yumi)
);


//=================================================
// AXI-L to FSB slave 
//=================================================
// loopback test
(* dont_touch = "true" *) logic ocl_slv_sync_rst_n;
lib_pipe #(.WIDTH(1), .STAGES(4)) OCL_SLV_SLC_RST_N (
  .clk    (clk)
  ,.rst_n  (1'b1)
  ,.in_bus (sync_rst_n)
  ,.out_bus(ocl_slv_sync_rst_n)
);

logic adpt_slave_v;
logic [79:0] adpt_slave_data;
logic adpt_slave_r;

logic adpt_master_v;
logic [79:0] adpt_master_data;
logic adpt_master_r;

s_axil_fsb_adapter s_axil_fsb (
  .clk_i           (clk)
  ,.resetn_i        (ocl_slv_sync_rst_n)
  ,.sh_ocl_bus      (sh_ocl_mux00)
  ,.adpt_slave_v    (adpt_slave_v)
  ,.adpt_slave_data (adpt_slave_data)
  ,.adpt_slave_r    (adpt_slave_r)
  ,.adpt_master_v   (adpt_master_v)
  ,.adpt_master_data(adpt_master_data)
  ,.adpt_master_r   (adpt_master_r));


bsg_test_node_client #(
  .ring_width_p(80),
  .master_id_p (0 ),
  .client_id_p (0 )
) fsb_client_node (
  .clk_i  (clk)
  ,.reset_i(~ocl_slv_sync_rst_n)
  ,.en_i   (1'b1)
  // input channel
  ,.v_i    (adpt_master_v)
  ,.data_i (adpt_master_data)
  ,.ready_o(adpt_master_r)
  // output channel
  ,.v_o    (adpt_slave_v)
  ,.data_o (adpt_slave_data)
  ,.yumi_i ((adpt_slave_r&&adpt_slave_v))
);

//=================================================
// AXI4 to FSB slave 
//=================================================

axi_bus_t sh_cl_dma_pcis_bus();

assign sh_cl_dma_pcis_bus.awid[5:0] = sh_cl_dma_pcis_awid;
assign sh_cl_dma_pcis_bus.awaddr    = sh_cl_dma_pcis_awaddr;
assign sh_cl_dma_pcis_bus.awlen     = sh_cl_dma_pcis_awlen;
assign sh_cl_dma_pcis_bus.awsize    = sh_cl_dma_pcis_awsize;
assign sh_cl_dma_pcis_bus.awvalid   = sh_cl_dma_pcis_awvalid;
assign cl_sh_dma_pcis_awready       = sh_cl_dma_pcis_bus.awready;

assign sh_cl_dma_pcis_bus.wdata     = sh_cl_dma_pcis_wdata;
assign sh_cl_dma_pcis_bus.wstrb     = sh_cl_dma_pcis_wstrb;
assign sh_cl_dma_pcis_bus.wlast     = sh_cl_dma_pcis_wlast;
assign sh_cl_dma_pcis_bus.wvalid    = sh_cl_dma_pcis_wvalid;
assign cl_sh_dma_pcis_wready        = sh_cl_dma_pcis_bus.wready;

assign cl_sh_dma_pcis_bid           = {2'b0, sh_cl_dma_pcis_bus.bid[3:0]};
assign cl_sh_dma_pcis_bresp         = sh_cl_dma_pcis_bus.bresp;
assign cl_sh_dma_pcis_bvalid        = sh_cl_dma_pcis_bus.bvalid;
assign sh_cl_dma_pcis_bus.bready    = sh_cl_dma_pcis_bready;

assign sh_cl_dma_pcis_bus.arid[5:0] = sh_cl_dma_pcis_arid;
assign sh_cl_dma_pcis_bus.araddr    = sh_cl_dma_pcis_araddr;
assign sh_cl_dma_pcis_bus.arlen     = sh_cl_dma_pcis_arlen;
assign sh_cl_dma_pcis_bus.arsize    = sh_cl_dma_pcis_arsize;
assign sh_cl_dma_pcis_bus.arvalid   = sh_cl_dma_pcis_arvalid;
assign cl_sh_dma_pcis_arready       = sh_cl_dma_pcis_bus.arready;

assign cl_sh_dma_pcis_rid           = {2'b0, sh_cl_dma_pcis_bus.rid[3:0]};
assign cl_sh_dma_pcis_rdata         = sh_cl_dma_pcis_bus.rdata;
assign cl_sh_dma_pcis_rresp         = sh_cl_dma_pcis_bus.rresp;
assign cl_sh_dma_pcis_rlast         = sh_cl_dma_pcis_bus.rlast;
assign cl_sh_dma_pcis_rvalid        = sh_cl_dma_pcis_bus.rvalid;
assign sh_cl_dma_pcis_bus.rready    = sh_cl_dma_pcis_rready;

(* dont_touch = "true" *) logic dma_pcis_sync_rst_n;
lib_pipe #(.WIDTH(1), .STAGES(4)) DMA_PCIS_SLC_RST_N (
  .clk    (clk)
  ,.rst_n  (1'b1)
  ,.in_bus (sync_rst_n)
  ,.out_bus(dma_pcis_sync_rst_n)
);


// Simple loop back 4x128bits without FSB client.
// TODO: AXI4-512bit bus should be able to write single FSB packet (128bit,80bit) .
s_axi4_fsb_adapter s_axi4_fsb (
  .clk_i           (clk),
  .resetn_i        (dma_pcis_sync_rst_n),
  .sh_ocl_bus      (sh_ocl_mux02),
  .sh_cl_dma_pcis  (sh_cl_dma_pcis_bus),
  .adpt_slave_v    (),
  .adpt_slave_data (),
  .adpt_slave_r    (),
  .adpt_master_v   (),
  .adpt_master_data(),
  .adpt_master_r   ()
);

endmodule
