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



//=================================================
// SH ocl bus multiplexer
//=================================================
localparam sh_ocl_slot_num_lp = 1;
`declare_bsg_axil_bus_s(sh_ocl_slot_num_lp, bsg_axil_mosi_bus_s, bsg_axil_miso_bus_s);

bsg_axil_mosi_bus_s sh_ocl_mosi_bus, sh_ocl_0_mosi_bus, sh_ocl_1_mosi_bus, sh_ocl_2_mosi_bus, sh_ocl_3_mosi_bus;
bsg_axil_miso_bus_s sh_ocl_miso_bus, sh_ocl_0_miso_bus, sh_ocl_1_miso_bus, sh_ocl_2_miso_bus, sh_ocl_3_miso_bus;


assign sh_ocl_mosi_bus.awvalid = sh_ocl_awvalid;
assign sh_ocl_mosi_bus.awaddr  = sh_ocl_awaddr;
assign ocl_sh_awready          = sh_ocl_miso_bus.awready;

assign sh_ocl_mosi_bus.wvalid = sh_ocl_wvalid;
assign sh_ocl_mosi_bus.wdata  = sh_ocl_wdata;
assign sh_ocl_mosi_bus.wstrb  = sh_ocl_wstrb;
assign ocl_sh_wready          = sh_ocl_miso_bus.wready;

assign ocl_sh_bresp           = sh_ocl_miso_bus.bresp;
assign ocl_sh_bvalid          = sh_ocl_miso_bus.bvalid;
assign sh_ocl_mosi_bus.bready = sh_ocl_bready;

assign sh_ocl_mosi_bus.araddr  = sh_ocl_araddr;
assign sh_ocl_mosi_bus.arvalid = sh_ocl_arvalid;
assign ocl_sh_arready          = sh_ocl_miso_bus.arready;

assign ocl_sh_rdata           = sh_ocl_miso_bus.rdata;
assign ocl_sh_rresp           = sh_ocl_miso_bus.rresp;
assign ocl_sh_rvalid          = sh_ocl_miso_bus.rvalid;
assign sh_ocl_mosi_bus.rready = sh_ocl_rready;

//-------------------------------------------------
// PCIe OCL AXI-L (SH to CL, from AppPF BAR0)
// this interface has 4 address ranges to use:
// 0x0000_0000 ~ 0x0000_0FFF : ctrl axil adapter  SH <-> CL
// 0x0000_1000 ~ 0x0000_1FFF : config 2 adapters  CL --> SH
// 0x0000_2000 ~ 0x0000_2FFF : config axi adapter SH <-> CL
// 0x0000_3000 ~             : to be determined
//-------------------------------------------------
(* dont_touch = "true" *) logic axi_crossbar_sync_rst_n;
lib_pipe #(.WIDTH(1), .STAGES(4)) AXI_CROSSBAR_RST_N (
  .clk    (clk)
  ,.rst_n  (1'b1)
  ,.in_bus (sync_rst_n)
  ,.out_bus(axi_crossbar_sync_rst_n)
);
cl_axil_mux4 ocl_cfg_slv (
  .clk_i      (clk),
  .reset_i    (~axi_crossbar_sync_rst_n),
  .mst_bus_i  (sh_ocl_mosi_bus  ),
  .mst_bus_o  (sh_ocl_miso_bus  ),
  .slv_0_bus_i(sh_ocl_0_miso_bus),
  .slv_0_bus_o(sh_ocl_0_mosi_bus),
  .slv_1_bus_i(sh_ocl_1_miso_bus),
  .slv_1_bus_o(sh_ocl_1_mosi_bus),
  .slv_2_bus_i(sh_ocl_2_miso_bus),
  .slv_2_bus_o(sh_ocl_2_mosi_bus),
  .slv_3_bus_i(sh_ocl_3_miso_bus),
  .slv_3_bus_o(sh_ocl_3_mosi_bus)
);



//=================================================
// SH Read and Write to FSB via AXI-Lite interface
//=================================================
logic m_fsb_v_i, s_fsb_v_o;
logic [79:0] m_fsb_data_i, s_fsb_data_o;
logic m_fsb_r_o, s_fsb_r_i;

(* dont_touch = "true" *) logic ocl_slv_sync_rst_n;
lib_pipe #(.WIDTH(1), .STAGES(4)) OCL_SLV_SLC_RST_N (
  .clk    (clk)
  ,.rst_n  (1'b1)
  ,.in_bus (sync_rst_n)
  ,.out_bus(ocl_slv_sync_rst_n)
);

s_axil_fsb_adapter s_axil_fsb (
  .clk_i       (clk               ),
  .resetn_i    (ocl_slv_sync_rst_n),
  .sh_ocl_bus_i(sh_ocl_0_mosi_bus ),
  .sh_ocl_bus_o(sh_ocl_0_miso_bus ),
  .m_fsb_v_i   (m_fsb_v_i         ),
  .m_fsb_data_i(m_fsb_data_i      ),
  .m_fsb_r_o   (m_fsb_r_o         ),
  .s_fsb_v_o   (s_fsb_v_o         ),
  .s_fsb_data_o(s_fsb_data_o      ),
  .s_fsb_r_i   (s_fsb_r_i         )
);

bsg_test_node_client #(
  .ring_width_p(80),
  .master_id_p (0 ),
  .client_id_p (0 )
) fsb_client_node (
  .clk_i  (clk)
  ,.reset_i(~ocl_slv_sync_rst_n)
  ,.en_i   (1'b1)
  // input channel
  ,.v_i    (s_fsb_v_o)
  ,.data_i (s_fsb_data_o)
  ,.ready_o(s_fsb_r_i)
  // output channel
  ,.v_o    (m_fsb_v_i)
  ,.data_o (m_fsb_data_i)
  ,.yumi_i ((m_fsb_r_o&&m_fsb_v_i))
);


//=================================================
// AXI4 to FSB slave 
//=================================================
localparam sh_pcis_slot_num_lp = 1;
localparam sh_pcis_id_width_lp = 6;
localparam sh_pcis_addr_width_lp = 64;
localparam sh_pcis_data_width_lp = 512;
`declare_bsg_axi_bus_s(
  sh_pcis_slot_num_lp
  ,sh_pcis_id_width_lp
  ,sh_pcis_addr_width_lp
  ,sh_pcis_data_width_lp
  ,axi_pcis_mosi_bus_s
  ,axi_pcis_miso_bus_s
  );

axi_pcis_mosi_bus_s sh_pcis_mosi_bus;
axi_pcis_miso_bus_s sh_pcis_miso_bus;

assign sh_pcis_mosi_bus.awid    = sh_cl_dma_pcis_awid;
assign sh_pcis_mosi_bus.awaddr  = sh_cl_dma_pcis_awaddr;
assign sh_pcis_mosi_bus.awlen   = sh_cl_dma_pcis_awlen;
assign sh_pcis_mosi_bus.awsize  = sh_cl_dma_pcis_awsize;
assign sh_pcis_mosi_bus.awvalid = sh_cl_dma_pcis_awvalid;
assign cl_sh_dma_pcis_awready   = sh_pcis_miso_bus.awready;

assign sh_pcis_mosi_bus.wdata  = sh_cl_dma_pcis_wdata;
assign sh_pcis_mosi_bus.wstrb  = sh_cl_dma_pcis_wstrb;
assign sh_pcis_mosi_bus.wlast  = sh_cl_dma_pcis_wlast;
assign sh_pcis_mosi_bus.wvalid = sh_cl_dma_pcis_wvalid;
assign cl_sh_dma_pcis_wready   = sh_pcis_miso_bus.wready;

assign cl_sh_dma_pcis_bid      = sh_pcis_miso_bus.bid;
assign cl_sh_dma_pcis_bresp    = sh_pcis_miso_bus.bresp;
assign cl_sh_dma_pcis_bvalid   = sh_pcis_miso_bus.bvalid;
assign sh_pcis_mosi_bus.bready = sh_cl_dma_pcis_bready;

assign sh_pcis_mosi_bus.arid    = sh_cl_dma_pcis_arid;
assign sh_pcis_mosi_bus.araddr  = sh_cl_dma_pcis_araddr;
assign sh_pcis_mosi_bus.arlen   = sh_cl_dma_pcis_arlen;
assign sh_pcis_mosi_bus.arsize  = sh_cl_dma_pcis_arsize;
assign sh_pcis_mosi_bus.arvalid = sh_cl_dma_pcis_arvalid;
assign cl_sh_dma_pcis_arready   = sh_pcis_miso_bus.arready;

assign cl_sh_dma_pcis_rid      = sh_pcis_miso_bus.rid;
assign cl_sh_dma_pcis_rdata    = sh_pcis_miso_bus.rdata;
assign cl_sh_dma_pcis_rresp    = sh_pcis_miso_bus.rresp;
assign cl_sh_dma_pcis_rlast    = sh_pcis_miso_bus.rlast;
assign cl_sh_dma_pcis_rvalid   = sh_pcis_miso_bus.rvalid;
assign sh_pcis_mosi_bus.rready = sh_cl_dma_pcis_rready;

(* dont_touch = "true" *) logic dma_pcis_sync_rst_n;
lib_pipe #(.WIDTH(1), .STAGES(4)) DMA_PCIS_SLC_RST_N (
 .clk    (clk)
 ,.rst_n  (1'b1)
 ,.in_bus (sync_rst_n)
 ,.out_bus(dma_pcis_sync_rst_n)
);
// Simply loop back 4x128bits without FSB client.
// TODO: AXI4-512bit bus should be able to write single FSB packet (128bit,80bit).
s_axi4_fsb_adapter s_axi4_fsb (
  .clk_i        (clk                ),
  .resetn_i     (dma_pcis_sync_rst_n),
  .sh_ocl_bus_i (sh_ocl_2_mosi_bus  ),
  .sh_ocl_bus_o (sh_ocl_2_miso_bus  ),
  .sh_pcis_bus_i(sh_pcis_mosi_bus   ),
  .sh_pcis_bus_o(sh_pcis_miso_bus   ),  
  .m_fsb_v_i    (          ),
  .m_fsb_data_i (       ),
  .m_fsb_r_o    (          ),
  .s_fsb_v_o    (          ),
  .s_fsb_data_o (       ),
  .s_fsb_r_i    (          )
);


//=================================================
// CL write to host via AXI-4 pcim Interface
//=================================================
localparam sh_pcim_slot_num_lp = 1;
localparam sh_pcim_id_width_lp = 6;
localparam sh_pcim_addr_width_lp = 64;
localparam sh_pcim_data_width_lp = 512;
`declare_bsg_axi_bus_s(
  sh_pcis_slot_num_lp
  ,sh_pcis_id_width_lp
  ,sh_pcis_addr_width_lp
  ,sh_pcis_data_width_lp
  ,axi_pcim_mosi_bus_s
  ,axi_pcim_miso_bus_s
  );

axi_pcim_mosi_bus_s sh_pcim_mosi_bus;
axi_pcim_miso_bus_s sh_pcim_miso_bus;

assign cl_sh_pcim_awid        = {10'b0, sh_pcim_mosi_bus.awid};
assign cl_sh_pcim_awaddr      = sh_pcim_mosi_bus.awaddr;
assign cl_sh_pcim_awlen       = sh_pcim_mosi_bus.awlen;
assign cl_sh_pcim_awsize      = sh_pcim_mosi_bus.awsize;
assign cl_sh_pcim_awvalid     = sh_pcim_mosi_bus.awvalid;
assign sh_pcim_miso_bus.awready = sh_cl_pcim_awready;

assign cl_sh_pcim_wdata       = sh_pcim_mosi_bus.wdata;
assign cl_sh_pcim_wstrb       = sh_pcim_mosi_bus.wstrb;
assign cl_sh_pcim_wlast       = sh_pcim_mosi_bus.wlast;
assign cl_sh_pcim_wvalid      = sh_pcim_mosi_bus.wvalid;
assign sh_pcim_miso_bus.wready  = sh_cl_pcim_wready;

assign sh_pcim_miso_bus.bid     = sh_cl_pcim_bid[5:0];
assign sh_pcim_miso_bus.bresp   = sh_cl_pcim_bresp;
assign sh_pcim_miso_bus.bvalid  = sh_cl_pcim_bvalid;
assign cl_sh_pcim_bready      = sh_pcim_mosi_bus.bready;

assign cl_sh_pcim_arid        = {10'b0, sh_pcim_mosi_bus.arid};
assign cl_sh_pcim_araddr      = sh_pcim_mosi_bus.araddr;
assign cl_sh_pcim_arlen       = sh_pcim_mosi_bus.arlen;
assign cl_sh_pcim_arsize      = sh_pcim_mosi_bus.arsize;
assign cl_sh_pcim_arvalid     = sh_pcim_mosi_bus.arvalid;
assign sh_pcim_miso_bus.arready = sh_cl_pcim_arready;

assign sh_pcim_miso_bus.rid     = sh_cl_pcim_rid[5:0];
assign sh_pcim_miso_bus.rdata   = sh_cl_pcim_rdata;
assign sh_pcim_miso_bus.rresp   = sh_cl_pcim_rresp;
assign sh_pcim_miso_bus.rlast   = sh_cl_pcim_rlast;
assign sh_pcim_miso_bus.rvalid  = sh_cl_pcim_rvalid;
assign cl_sh_pcim_rready      = sh_pcim_mosi_bus.rready;


(* dont_touch = "true" *) logic pcim_mstr_sync_rst_n;
lib_pipe #(.WIDTH(1), .STAGES(4)) PCIM_MSTR_SLC_RST_N (
  .clk    (clk                 ),
  .rst_n  (1'b1                ),
  .in_bus (sync_rst_n          ),
  .out_bus(pcim_mstr_sync_rst_n)
);

logic fsb_wvalid;
logic [`FSB_WIDTH-1:0] fsb_wdata;
logic fsb_yumi;

bsg_test_node_master #(
  .ring_width_p(`FSB_WIDTH),
  .master_id_p (4'hF      ),
  .client_id_p (4'hF      )
) fsb_node_master (
  .clk_i  (clk                  ),
  .reset_i(~pcim_mstr_sync_rst_n),
  .en_i   (1'b1                 ),
  .v_i    (1'b0                 ),
  .data_i ({`FSB_WIDTH{1'b0}}   ),
  .ready_o(                     ),
  .v_o    (fsb_wvalid           ),
  .data_o (fsb_wdata            ),
  .yumi_i (fsb_yumi             )
);


`declare_bsg_axis_bus_s(512, bsg_axisx512_mosi_bus_s, bsg_axisx512_miso_bus_s);

bsg_axisx512_mosi_bus_s mosi_axisx512_bus;
bsg_axisx512_miso_bus_s miso_axisx512_bus;

cl_axis_test_master #(.data_width_p(512)) axis_master (
  .clk_i       (clk                  ),
  .reset_i     (~pcim_mstr_sync_rst_n),
  .en_i        (1'b1                 ),
  .m_axis_bus_i(miso_axisx512_bus    ),
  .m_axis_bus_o(mosi_axisx512_bus    ),
  .loop_done   (                     )
);


cl_to_axi4_adapter axi4_pcim_write (
  .clk_i           (clk                 ),
  .reset_i        (~pcim_mstr_sync_rst_n),
  .sh_cl_flr_assert(sh_cl_flr_assert_q  ),
  .fsb_wvalid      (fsb_wvalid          ),
  .fsb_wdata       (fsb_wdata           ),
  .fsb_yumi        (fsb_yumi            ),
  
  .s_ocl_bus_i    (sh_ocl_1_mosi_bus        ),
  .s_ocl_bus_o    (sh_ocl_1_miso_bus        ),
  .s_pcis_bus_i   (       ),
  .s_pcis_bus_o   (       ),
  .m_pcim_bus_i   (sh_pcim_miso_bus       ),
  .m_pcim_bus_o   (sh_pcim_mosi_bus       ),
  .s_axis_bus_i    (mosi_axisx512_bus        ),
  .s_axis_bus_o    (miso_axisx512_bus        )
);


endmodule
