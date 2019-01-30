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



//---------------------------------------------------------------
//                    SH ocl bus multiplexer                    |
//                                                              |
//---------------------------------------------------------------
`declare_bsg_axil_bus_s(1, bsg_axil_mosi_bus_s, bsg_axil_miso_bus_s);
bsg_axil_mosi_bus_s sh_ocl_mosi_bus, sh_ocl_0_i, sh_ocl_1_i, sh_ocl_2_i, sh_ocl_3_i;
bsg_axil_miso_bus_s sh_ocl_miso_bus, sh_ocl_0_o, sh_ocl_1_o, sh_ocl_2_o, sh_ocl_3_o;


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


`declare_bsg_axil_bus_s(4, bsg_axil_mosi_busX4_s, bsg_axil_miso_busX4_s);
bsg_axil_mosi_busX4_s axil_mosi_busX4;
bsg_axil_miso_busX4_s axil_miso_busX4;

assign {sh_ocl_3_i.awaddr, sh_ocl_2_i.awaddr, sh_ocl_1_i.awaddr, sh_ocl_0_i.awaddr} = axil_mosi_busX4.awaddr;
assign {sh_ocl_3_i.awvalid, sh_ocl_2_i.awvalid, sh_ocl_1_i.awvalid, sh_ocl_0_i.awvalid} = axil_mosi_busX4.awvalid;
assign {sh_ocl_3_i.wdata, sh_ocl_2_i.wdata, sh_ocl_1_i.wdata, sh_ocl_0_i.wdata} = axil_mosi_busX4.wdata;
assign {sh_ocl_3_i.wstrb, sh_ocl_2_i.wstrb, sh_ocl_1_i.wstrb, sh_ocl_0_i.wstrb} = axil_mosi_busX4.wstrb;
assign {sh_ocl_3_i.wvalid, sh_ocl_2_i.wvalid, sh_ocl_1_i.wvalid, sh_ocl_0_i.wvalid} = axil_mosi_busX4.wvalid;
assign {sh_ocl_3_i.bready, sh_ocl_2_i.bready, sh_ocl_1_i.bready, sh_ocl_0_i.bready} = axil_mosi_busX4.bready;
assign {sh_ocl_3_i.araddr, sh_ocl_2_i.araddr, sh_ocl_1_i.araddr, sh_ocl_0_i.araddr} = axil_mosi_busX4.araddr;
assign {sh_ocl_3_i.arvalid, sh_ocl_2_i.arvalid, sh_ocl_1_i.arvalid, sh_ocl_0_i.arvalid} = axil_mosi_busX4.arvalid;
assign {sh_ocl_3_i.rready, sh_ocl_2_i.rready, sh_ocl_1_i.rready, sh_ocl_0_i.rready} = axil_mosi_busX4.rready;

assign axil_miso_busX4.awready = {sh_ocl_3_o.awready, sh_ocl_2_o.awready, sh_ocl_1_o.awready, sh_ocl_0_o.awready};
assign axil_miso_busX4.wready  = {sh_ocl_3_o.wready, sh_ocl_2_o.wready, sh_ocl_1_o.wready, sh_ocl_0_o.wready};
assign axil_miso_busX4.bresp   = {sh_ocl_3_o.bresp, sh_ocl_2_o.bresp, sh_ocl_1_o.bresp, sh_ocl_0_o.bresp};
assign axil_miso_busX4.bvalid  = {sh_ocl_3_o.bvalid, sh_ocl_2_o.bvalid, sh_ocl_1_o.bvalid, sh_ocl_0_o.bvalid};
assign axil_miso_busX4.arready = {sh_ocl_3_o.arready, sh_ocl_2_o.arready, sh_ocl_1_o.arready, sh_ocl_0_o.arready};
assign axil_miso_busX4.rdata   = {sh_ocl_3_o.rdata, sh_ocl_2_o.rdata, sh_ocl_1_o.rdata, sh_ocl_0_o.rdata};
assign axil_miso_busX4.rresp   = {sh_ocl_3_o.rresp, sh_ocl_2_o.rresp, sh_ocl_1_o.rresp, sh_ocl_0_o.rresp};
assign axil_miso_busX4.rvalid  = {sh_ocl_3_o.rvalid, sh_ocl_2_o.rvalid, sh_ocl_1_o.rvalid, sh_ocl_0_o.rvalid};


// PCIe OCL AXI-L (SH to CL, from AppPF BAR0)
// sh ocl axil bus
//          -> crossbar
//                   -> |ctrl   axil SH <-> CL (0x0000_0000 ~ 0x0000_0FFF)
//                   -> |config  FBS CL --> SH (0x0000_1000 ~ 0x0000_1FFF)
//                   -> |config axi4 SH <-> CL (0x0000_2000 ~ 0x0000_2FFF)
//                   -> |config axis CL --> SH (0x0000_3000 ~ 0x0000_3FFF)
//-------------------------------------------------
localparam C_NUM_MASTER_SLOTS         = 4                                                                           ;
localparam C_M_AXI_BASE_ADDR          = 256'h00000000_00003000_00000000_00002000_00000000_00001000_00000000_00000000;
localparam C_M_AXI_ADDR_WIDTH         = {C_NUM_MASTER_SLOTS{32'h0000_000c}}                                         ;
localparam C_M_AXI_WRITE_CONNECTIVITY = {C_NUM_MASTER_SLOTS{32'h0000_0001}}                                         ;
localparam C_M_AXI_READ_CONNECTIVITY  = {C_NUM_MASTER_SLOTS{32'h0000_0001}}                                         ;
localparam C_M_AXI_WRITE_ISSUING      = {C_NUM_MASTER_SLOTS{32'h0000_0001}}                                         ;
localparam C_M_AXI_READ_ISSUING       = {C_NUM_MASTER_SLOTS{32'h0000_0001}}                                         ;
localparam C_M_AXI_SECURE             = {C_NUM_MASTER_SLOTS{32'h0000_0000}}                                         ;

(* dont_touch = "true" *) logic crsbar_rstn;
lib_pipe #(.WIDTH(1), .STAGES(4)) CROSSBAR_RST_N (.clk(clk), .rst_n(1'b1), .in_bus(sync_rst_n), .out_bus(crsbar_rstn));
axi_crossbar_v2_1_18_axi_crossbar #(
  .C_FAMILY                   ("virtexuplus"             ),
  .C_NUM_SLAVE_SLOTS          (1                         ),
  .C_NUM_MASTER_SLOTS         (C_NUM_MASTER_SLOTS        ),
  .C_AXI_ID_WIDTH             (1                         ),
  .C_AXI_ADDR_WIDTH           (32                        ),
  .C_AXI_DATA_WIDTH           (32                        ),
  .C_AXI_PROTOCOL             (2                         ),
  .C_NUM_ADDR_RANGES          (1                         ),
  .C_M_AXI_BASE_ADDR          (C_M_AXI_BASE_ADDR         ),
  .C_M_AXI_ADDR_WIDTH         (C_M_AXI_ADDR_WIDTH        ),
  .C_S_AXI_BASE_ID            (32'H00000000              ),
  .C_S_AXI_THREAD_ID_WIDTH    (32'H00000000              ),
  .C_AXI_SUPPORTS_USER_SIGNALS(0                         ),
  .C_AXI_AWUSER_WIDTH         (1                         ),
  .C_AXI_ARUSER_WIDTH         (1                         ),
  .C_AXI_WUSER_WIDTH          (1                         ),
  .C_AXI_RUSER_WIDTH          (1                         ),
  .C_AXI_BUSER_WIDTH          (1                         ),
  .C_M_AXI_WRITE_CONNECTIVITY (C_M_AXI_WRITE_CONNECTIVITY),
  .C_M_AXI_READ_CONNECTIVITY  (C_M_AXI_READ_CONNECTIVITY ),
  .C_R_REGISTER               (1                         ),
  .C_S_AXI_SINGLE_THREAD      (32'H00000001              ),
  .C_S_AXI_WRITE_ACCEPTANCE   (32'H00000001              ),
  .C_S_AXI_READ_ACCEPTANCE    (32'H00000001              ),
  .C_M_AXI_WRITE_ISSUING      (C_M_AXI_WRITE_ISSUING     ),
  .C_M_AXI_READ_ISSUING       (C_M_AXI_READ_ISSUING      ),
  .C_S_AXI_ARB_PRIORITY       (32'H00000000              ),
  .C_M_AXI_SECURE             (C_M_AXI_SECURE            ),
  .C_CONNECTIVITY_MODE        (0                         )
) axil_crossbar_mux (
  .aclk          (clk                       ),
  .aresetn       (crsbar_rstn               ),
  .s_axi_awid    (1'H0                      ),
  .s_axi_awaddr  (sh_ocl_mosi_bus.awaddr    ),
  .s_axi_awlen   (8'H00                     ),
  .s_axi_awsize  (3'H0                      ),
  .s_axi_awburst (2'H0                      ),
  .s_axi_awlock  (1'H0                      ),
  .s_axi_awcache (4'H0                      ),
  .s_axi_awprot  (3'H0                      ),
  .s_axi_awqos   (4'H0                      ),
  .s_axi_awuser  (1'H0                      ),
  .s_axi_awvalid (sh_ocl_mosi_bus.awvalid   ),
  .s_axi_awready (sh_ocl_miso_bus.awready   ),
  .s_axi_wid     (1'H0                      ),
  .s_axi_wdata   (sh_ocl_mosi_bus.wdata     ),
  .s_axi_wstrb   (sh_ocl_mosi_bus.wstrb     ),
  .s_axi_wlast   (1'H1                      ),
  .s_axi_wuser   (1'H0                      ),
  .s_axi_wvalid  (sh_ocl_mosi_bus.wvalid    ),
  .s_axi_wready  (sh_ocl_miso_bus.wready    ),
  .s_axi_bid     (                          ),
  .s_axi_bresp   (sh_ocl_miso_bus.bresp     ),
  .s_axi_buser   (                          ),
  .s_axi_bvalid  (sh_ocl_miso_bus.bvalid    ),
  .s_axi_bready  (sh_ocl_mosi_bus.bready    ),
  .s_axi_arid    (1'H0                      ),
  .s_axi_araddr  (sh_ocl_mosi_bus.araddr    ),
  .s_axi_arlen   (8'H00                     ),
  .s_axi_arsize  (3'H0                      ),
  .s_axi_arburst (2'H0                      ),
  .s_axi_arlock  (1'H0                      ),
  .s_axi_arcache (4'H0                      ),
  .s_axi_arprot  (3'H0                      ),
  .s_axi_arqos   (4'H0                      ),
  .s_axi_aruser  (1'H0                      ),
  .s_axi_arvalid (sh_ocl_mosi_bus.arvalid   ),
  .s_axi_arready (sh_ocl_miso_bus.arready   ),
  .s_axi_rid     (                          ),
  .s_axi_rdata   (sh_ocl_miso_bus.rdata     ),
  .s_axi_rresp   (sh_ocl_miso_bus.rresp     ),
  .s_axi_rlast   (                          ),
  .s_axi_ruser   (                          ),
  .s_axi_rvalid  (sh_ocl_miso_bus.rvalid    ),
  .s_axi_rready  (sh_ocl_mosi_bus.rready    ),
  .m_axi_awid    (                          ),
  .m_axi_awaddr  (axil_mosi_busX4.awaddr    ),
  .m_axi_awlen   (                          ),
  .m_axi_awsize  (                          ),
  .m_axi_awburst (                          ),
  .m_axi_awlock  (                          ),
  .m_axi_awcache (                          ),
  .m_axi_awprot  (                          ),
  .m_axi_awregion(                          ),
  .m_axi_awqos   (                          ),
  .m_axi_awuser  (                          ),
  .m_axi_awvalid (axil_mosi_busX4.awvalid   ),
  .m_axi_awready (axil_miso_busX4.awready   ),
  .m_axi_wid     (                          ),
  .m_axi_wdata   (axil_mosi_busX4.wdata     ),
  .m_axi_wstrb   (axil_mosi_busX4.wstrb     ),
  .m_axi_wlast   (                          ),
  .m_axi_wuser   (                          ),
  .m_axi_wvalid  (axil_mosi_busX4.wvalid    ),
  .m_axi_wready  (axil_miso_busX4.wready    ),
  .m_axi_bid     ({C_NUM_MASTER_SLOTS{1'b0}}),
  .m_axi_bresp   (axil_miso_busX4.bresp     ),
  .m_axi_buser   ({C_NUM_MASTER_SLOTS{1'b0}}),
  .m_axi_bvalid  (axil_miso_busX4.bvalid    ),
  .m_axi_bready  (axil_mosi_busX4.bready    ),
  .m_axi_arid    (                          ),
  .m_axi_araddr  (axil_mosi_busX4.araddr    ),
  .m_axi_arlen   (                          ),
  .m_axi_arsize  (                          ),
  .m_axi_arburst (                          ),
  .m_axi_arlock  (                          ),
  .m_axi_arcache (                          ),
  .m_axi_arprot  (                          ),
  .m_axi_arregion(                          ),
  .m_axi_arqos   (                          ),
  .m_axi_aruser  (                          ),
  .m_axi_arvalid (axil_mosi_busX4.arvalid   ),
  .m_axi_arready (axil_miso_busX4.arready   ),
  .m_axi_rid     ({C_NUM_MASTER_SLOTS{1'b0}}),
  .m_axi_rdata   (axil_miso_busX4.rdata     ),
  .m_axi_rresp   (axil_miso_busX4.rresp     ),
  .m_axi_rlast   ({C_NUM_MASTER_SLOTS{1'b1}}),
  .m_axi_ruser   ({C_NUM_MASTER_SLOTS{1'b0}}),
  .m_axi_rvalid  (axil_miso_busX4.rvalid    ),
  .m_axi_rready  (axil_mosi_busX4.rready    )
);


//---------------------------------------------------------------
//                    axi pcis bus                              |
//                                                              |
//---------------------------------------------------------------
localparam sh_pcis_id_width_lp = 6;
localparam sh_pcis_addr_width_lp = 64;
localparam sh_pcis_data_width_lp = 512;
`declare_bsg_axi_bus_s(1, sh_pcis_id_width_lp ,sh_pcis_addr_width_lp ,sh_pcis_data_width_lp
  ,bsg_pcis_mosi_bus_s
  ,bsg_pcis_miso_bus_s);

bsg_pcis_mosi_bus_s sh_pcis_mosi_bus;
bsg_pcis_miso_bus_s sh_pcis_miso_bus;

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



//---------------------------------------------------------------
//                     axi pcim bus                             |
//                                                              |
//---------------------------------------------------------------

//       fsb-axi4 -> |
//      axis_axi4 -> |crossbar -> pcim bus
localparam sh_pcim_id_width_lp   = 6  ;
localparam sh_pcim_addr_width_lp = 64 ;
localparam sh_pcim_data_width_lp = 512;

`declare_bsg_axi_bus_s(1, sh_pcim_id_width_lp, sh_pcim_addr_width_lp, sh_pcim_data_width_lp
  ,bsg_pcim_mosi_s
  ,bsg_pcim_miso_s);
bsg_pcim_mosi_s sh_pcim_mosi_bus, pcim_0_o, pcim_1_o;
bsg_pcim_miso_s sh_pcim_miso_bus, pcim_0_i, pcim_1_i;

`declare_bsg_axi_bus_s(2, sh_pcim_id_width_lp, sh_pcim_addr_width_lp, sh_pcim_data_width_lp,
  bsg_axi_mosi_2bus_s, bsg_axi_miso_2bus_s);
bsg_axi_mosi_2bus_s axi_mosi_busX2;
bsg_axi_miso_2bus_s axi_miso_busX2;

// cast pcim bus
//-------------------------------------------------
assign cl_sh_pcim_awid          = {10'b0, sh_pcim_mosi_bus.awid};
assign cl_sh_pcim_awaddr        = sh_pcim_mosi_bus.awaddr;
assign cl_sh_pcim_awlen         = sh_pcim_mosi_bus.awlen;
assign cl_sh_pcim_awsize        = sh_pcim_mosi_bus.awsize;
assign cl_sh_pcim_awvalid       = sh_pcim_mosi_bus.awvalid;
assign sh_pcim_miso_bus.awready = sh_cl_pcim_awready;

assign cl_sh_pcim_wdata        = sh_pcim_mosi_bus.wdata;
assign cl_sh_pcim_wstrb        = sh_pcim_mosi_bus.wstrb;
assign cl_sh_pcim_wlast        = sh_pcim_mosi_bus.wlast;
assign cl_sh_pcim_wvalid       = sh_pcim_mosi_bus.wvalid;
assign sh_pcim_miso_bus.wready = sh_cl_pcim_wready;

assign sh_pcim_miso_bus.bid    = sh_cl_pcim_bid[5:0];
assign sh_pcim_miso_bus.bresp  = sh_cl_pcim_bresp;
assign sh_pcim_miso_bus.bvalid = sh_cl_pcim_bvalid;
assign cl_sh_pcim_bready       = sh_pcim_mosi_bus.bready;

assign cl_sh_pcim_arid          = {10'b0, sh_pcim_mosi_bus.arid};
assign cl_sh_pcim_araddr        = sh_pcim_mosi_bus.araddr;
assign cl_sh_pcim_arlen         = sh_pcim_mosi_bus.arlen;
assign cl_sh_pcim_arsize        = sh_pcim_mosi_bus.arsize;
assign cl_sh_pcim_arvalid       = sh_pcim_mosi_bus.arvalid;
assign sh_pcim_miso_bus.arready = sh_cl_pcim_arready;

assign sh_pcim_miso_bus.rid    = sh_cl_pcim_rid[5:0];
assign sh_pcim_miso_bus.rdata  = sh_cl_pcim_rdata;
assign sh_pcim_miso_bus.rresp  = sh_cl_pcim_rresp;
assign sh_pcim_miso_bus.rlast  = sh_cl_pcim_rlast;
assign sh_pcim_miso_bus.rvalid = sh_cl_pcim_rvalid;
assign cl_sh_pcim_rready       = sh_pcim_mosi_bus.rready;

// demux pcim bus
//-------------------------------------------------
assign axi_mosi_busX2.awid    = {pcim_1_o.awid, pcim_0_o.awid};
assign axi_mosi_busX2.awaddr  = {pcim_1_o.awaddr, pcim_0_o.awaddr};
assign axi_mosi_busX2.awlen   = {pcim_1_o.awlen, pcim_0_o.awlen};
assign axi_mosi_busX2.awsize  = {pcim_1_o.awsize, pcim_0_o.awsize};
assign axi_mosi_busX2.awvalid = {pcim_1_o.awvalid, pcim_0_o.awvalid};
assign {pcim_1_i.awready, pcim_0_i.awready} = axi_miso_busX2.awready;

assign axi_mosi_busX2.wdata  = {pcim_1_o.wdata, pcim_0_o.wdata};
assign axi_mosi_busX2.wstrb  = {pcim_1_o.wstrb, pcim_0_o.wstrb};
assign axi_mosi_busX2.wlast  = {pcim_1_o.wlast, pcim_0_o.wlast};
assign axi_mosi_busX2.wvalid = {pcim_1_o.wvalid, pcim_0_o.wvalid};
assign  {pcim_1_i.wready, pcim_0_i.wready} = axi_miso_busX2.wready;

assign {pcim_1_i.bid, pcim_0_i.bid} = axi_miso_busX2.bid;
assign {pcim_1_i.bresp, pcim_0_i.bresp} = axi_miso_busX2.bresp;
assign {pcim_1_i.bvalid, pcim_0_i.bvalid} = axi_miso_busX2.bvalid;
assign axi_mosi_busX2.bready = {pcim_1_o.bready, pcim_0_o.bready};

assign axi_mosi_busX2.arid    = {pcim_1_o.arid, pcim_0_o.arid};
assign axi_mosi_busX2.araddr  = {pcim_1_o.araddr, pcim_0_o.araddr};
assign axi_mosi_busX2.arlen   = {pcim_1_o.arlen, pcim_0_o.arlen};
assign axi_mosi_busX2.arsize  = {pcim_1_o.arsize, pcim_0_o.arsize};
assign axi_mosi_busX2.arvalid = {pcim_1_o.arvalid, pcim_0_o.arvalid};
assign {pcim_1_i.arready, pcim_0_i.arready} = axi_miso_busX2.arready;

assign {pcim_1_i.rid, pcim_0_i.rid} = axi_miso_busX2.rid;
assign {pcim_1_i.rdata, pcim_0_i.rdata} = axi_miso_busX2.rdata;
assign {pcim_1_i.rresp, pcim_0_i.rresp} = axi_miso_busX2.rresp;
assign {pcim_1_i.rlast, pcim_0_i.rlast} = axi_miso_busX2.rlast;
assign {pcim_1_i.rvalid, pcim_0_i.rvalid} = axi_miso_busX2.rvalid;
assign axi_mosi_busX2.rready = {pcim_1_o.rready, pcim_0_o.rready};


axi_crossbar_v2_1_18_axi_crossbar #(
  .C_FAMILY                   ("virtexuplus"       ),
  .C_NUM_SLAVE_SLOTS          (2                   ),
  .C_NUM_MASTER_SLOTS         (1                   ),
  .C_AXI_ID_WIDTH             (6                   ),
  .C_AXI_ADDR_WIDTH           (64                  ),
  .C_AXI_DATA_WIDTH           (512                 ),
  .C_AXI_PROTOCOL             (0                   ),
  .C_NUM_ADDR_RANGES          (1                   ),
  .C_M_AXI_BASE_ADDR          (64'H0000000000000000),
  .C_M_AXI_ADDR_WIDTH         (32'H00000040        ),
  .C_S_AXI_BASE_ID            (64'H0000000000000000),
  .C_S_AXI_THREAD_ID_WIDTH    (64'H0000000500000005),
  .C_AXI_SUPPORTS_USER_SIGNALS(0                   ),
  .C_AXI_AWUSER_WIDTH         (1                   ),
  .C_AXI_ARUSER_WIDTH         (1                   ),
  .C_AXI_WUSER_WIDTH          (1                   ),
  .C_AXI_RUSER_WIDTH          (1                   ),
  .C_AXI_BUSER_WIDTH          (1                   ),
  .C_M_AXI_WRITE_CONNECTIVITY (32'H00000003        ),
  .C_M_AXI_READ_CONNECTIVITY  (32'H00000003        ),
  .C_R_REGISTER               (0                   ),
  .C_S_AXI_SINGLE_THREAD      (64'H0000000000000000),
  .C_S_AXI_WRITE_ACCEPTANCE   (64'H0000000200000002),
  .C_S_AXI_READ_ACCEPTANCE    (64'H0000000200000002),
  .C_M_AXI_WRITE_ISSUING      (32'H00000004        ),
  .C_M_AXI_READ_ISSUING       (32'H00000004        ),
  .C_S_AXI_ARB_PRIORITY       (64'H0000000000000000),
  .C_M_AXI_SECURE             (32'H00000000        ),
  .C_CONNECTIVITY_MODE        (1                   )
) axi_crossbar_demux (
  .aclk          (clk                   ),
  .aresetn       (crsbar_rstn                ),
  .s_axi_awid    (axi_mosi_busX2.awid     ),
  .s_axi_awaddr  (axi_mosi_busX2.awaddr   ),
  .s_axi_awlen   (axi_mosi_busX2.awlen    ),
  .s_axi_awsize  (axi_mosi_busX2.awsize   ),
  .s_axi_awburst (4'H0                    ),
  .s_axi_awlock  (2'H0                    ),
  .s_axi_awcache (8'H0                    ),
  .s_axi_awprot  (6'H0                    ),
  .s_axi_awqos   (8'H0                    ),
  .s_axi_awuser  (2'H0                    ),
  .s_axi_awvalid (axi_mosi_busX2.awvalid  ),
  .s_axi_awready (axi_miso_busX2.awready  ),
  .s_axi_wid     (12'H000                 ),
  .s_axi_wdata   (axi_mosi_busX2.wdata    ),
  .s_axi_wstrb   (axi_mosi_busX2.wstrb    ),
  .s_axi_wlast   (axi_mosi_busX2.wlast    ),
  .s_axi_wuser   (2'H0                    ),
  .s_axi_wvalid  (axi_mosi_busX2.wvalid   ),
  .s_axi_wready  (axi_miso_busX2.wready   ),
  .s_axi_bid     (axi_miso_busX2.bid      ),
  .s_axi_bresp   (axi_miso_busX2.bresp    ),
  .s_axi_buser   (                        ),
  .s_axi_bvalid  (axi_miso_busX2.bvalid   ),
  .s_axi_bready  (axi_mosi_busX2.bready   ),
  .s_axi_arid    (axi_mosi_busX2.arid     ),
  .s_axi_araddr  (axi_mosi_busX2.araddr   ),
  .s_axi_arlen   (axi_mosi_busX2.arlen    ),
  .s_axi_arsize  (axi_mosi_busX2.arsize   ),
  .s_axi_arburst (4'H0                    ),
  .s_axi_arlock  (2'H0                    ),
  .s_axi_arcache (8'H0                    ),
  .s_axi_arprot  (6'H0                    ),
  .s_axi_arqos   (8'H0                    ),
  .s_axi_aruser  (2'H0                    ),
  .s_axi_arvalid (axi_mosi_busX2.arvalid  ),
  .s_axi_arready (axi_miso_busX2.arready  ),
  .s_axi_rid     (axi_miso_busX2.rid      ),
  .s_axi_rdata   (axi_miso_busX2.rdata    ),
  .s_axi_rresp   (axi_miso_busX2.rresp    ),
  .s_axi_rlast   (axi_miso_busX2.rlast    ),
  .s_axi_ruser   (                        ),
  .s_axi_rvalid  (axi_miso_busX2.rvalid   ),
  .s_axi_rready  (axi_mosi_busX2.rready   ),
  .m_axi_awid    (sh_pcim_mosi_bus.awid   ),
  .m_axi_awaddr  (sh_pcim_mosi_bus.awaddr ),
  .m_axi_awlen   (sh_pcim_mosi_bus.awlen  ),
  .m_axi_awsize  (sh_pcim_mosi_bus.awsize ),
  .m_axi_awburst (                        ),
  .m_axi_awlock  (                        ),
  .m_axi_awcache (                        ),
  .m_axi_awprot  (                        ),
  .m_axi_awregion(                        ),
  .m_axi_awqos   (                        ),
  .m_axi_awuser  (                        ),
  .m_axi_awvalid (sh_pcim_mosi_bus.awvalid),
  .m_axi_awready (sh_pcim_miso_bus.awready),
  .m_axi_wid     (                        ),
  .m_axi_wdata   (sh_pcim_mosi_bus.wdata  ),
  .m_axi_wstrb   (sh_pcim_mosi_bus.wstrb  ),
  .m_axi_wlast   (sh_pcim_mosi_bus.wlast  ),
  .m_axi_wuser   (                        ),
  .m_axi_wvalid  (sh_pcim_mosi_bus.wvalid ),
  .m_axi_wready  (sh_pcim_miso_bus.wready ),
  .m_axi_bid     (sh_pcim_miso_bus.bid    ),
  .m_axi_bresp   (sh_pcim_miso_bus.bresp  ),
  .m_axi_buser   (1'H0                    ),
  .m_axi_bvalid  (sh_pcim_miso_bus.bvalid ),
  .m_axi_bready  (sh_pcim_mosi_bus.bready ),
  .m_axi_arid    (sh_pcim_mosi_bus.arid   ),
  .m_axi_araddr  (sh_pcim_mosi_bus.araddr ),
  .m_axi_arlen   (sh_pcim_mosi_bus.arlen  ),
  .m_axi_arsize  (sh_pcim_mosi_bus.arsize ),
  .m_axi_arburst (                        ),
  .m_axi_arlock  (                        ),
  .m_axi_arcache (                        ),
  .m_axi_arprot  (                        ),
  .m_axi_arregion(                        ),
  .m_axi_arqos   (                        ),
  .m_axi_aruser  (                        ),
  .m_axi_arvalid (sh_pcim_mosi_bus.arvalid),
  .m_axi_arready (sh_pcim_miso_bus.arready),
  .m_axi_rid     (sh_pcim_miso_bus.rid    ),
  .m_axi_rdata   (sh_pcim_miso_bus.rdata  ),
  .m_axi_rresp   (sh_pcim_miso_bus.rresp  ),
  .m_axi_rlast   (sh_pcim_miso_bus.rlast  ),
  .m_axi_ruser   (1'H0                    ),
  .m_axi_rvalid  (sh_pcim_miso_bus.rvalid ),
  .m_axi_rready  (sh_pcim_mosi_bus.rready )
);


//---------------------------------------------------------------
//                    axi - fsb adapters                        |
//                                                              |
//---------------------------------------------------------------
localparam axil_slv_num_lp = 10;
localparam fsb_width_lp = `FSB_WIDTH;
(* dont_touch = "true" *) logic fsb_node_rstn;
lib_pipe #(.WIDTH(1), .STAGES(4)) FSB_RST_N (.clk(clk), .rst_n(1'b1), .in_bus(sync_rst_n), .out_bus(fsb_node_rstn));

// fsb slave
//-------------------------------------------------
logic [axil_slv_num_lp-1:0] m0_fsb_v_i, m0_fsb_v_o;
logic [(fsb_width_lp*axil_slv_num_lp)-1:0] m0_fsb_data_i, m0_fsb_data_o;
logic [axil_slv_num_lp-1:0] m0_fsb_yumi_o, m0_fsb_ready_i;

genvar i;
for (i=0;i<axil_slv_num_lp;i=i+1)
begin: bsg_test_node_slv
  cl_simple_loopback #(
    .data_width_p(80)
    ,.mask_p      ({80{1'b1}})
  ) fsb_loopback_node (
    .clk_i  (clk                                        ),
    .reset_i(~fsb_node_rstn                             ),
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

// fsb master
//-------------------------------------------------
logic                  s_fsb_v_i   ;
logic [fsb_width_lp-1:0] s_fsb_data_i;
logic                  s_fsb_yumi_o;

// bsg_test_node_master #(
//   .ring_width_p(fsb_width_lp),
//   .master_id_p (4'hF        ),
//   .client_id_p (4'hF        )
// ) fsb_node_master (
//   .clk_i  (clk                 ),
//   .reset_i(~fsb_node_rstn      ),
//   .en_i   (1'b1                ),
//   .v_i    (1'b0                ),
//   .data_i ({fsb_width_lp{1'b0}}),
//   .ready_o(                    ),
//   .v_o    (s_fsb_v_i           ),
//   .data_o (s_fsb_data_i        ),
//   .yumi_i (s_fsb_yumi_o        )
// );
`declare_bsg_axis_bus_s(128, bsg_axis_128_mosi_bus_s, bsg_axis_128_miso_bus_s);
bsg_axis_128_mosi_bus_s fsb_gen_mosi_bus;
bsg_axis_128_miso_bus_s fsb_gen_miso_bus;

assign fsb_gen_miso_bus.txd_tready = s_fsb_yumi_o;
assign s_fsb_v_i = fsb_gen_mosi_bus.txd_tvalid;
assign s_fsb_data_i = fsb_gen_mosi_bus.txd_tdata[79:0];

(* dont_touch = "true" *) logic axi4_fsb_rstn;
lib_pipe #(.WIDTH(1), .STAGES(4)) AXI_fsb_RST_N (.clk(clk), .rst_n(1'b1), .in_bus(sync_rst_n), .out_bus(axi4_fsb_rstn));
bsg_axis_gen_master #(.data_width_p(128)) fsb_master (
  .clk_i       (clk              ),
  .reset_i     (~axi4_fsb_rstn   ),
  .en_i        (1'b1             ),
  .m_axis_bus_i(fsb_gen_miso_bus),
  .m_axis_bus_o(fsb_gen_mosi_bus),
  .loop_done   (                 )
);



// axi to fsb adapters
// 1. axil -> fsb
// 2. axi4 -> fsb
// 3. fsb  -> axi4
//-------------------------------------------------
(* dont_touch = "true" *) logic axi_fsb_rstn;
lib_pipe #(.WIDTH(1), .STAGES(4)) AXI_FSB_RST_N (.clk(clk), .rst_n(1'b1), .in_bus(sync_rst_n), .out_bus(axi_fsb_rstn));
axi_fsb_adapters #(
  .fsb_width_p     (fsb_width_lp         ),
  .axil_slv_num_p  (axil_slv_num_lp      ),
  .axi_id_width_p  (sh_pcis_id_width_lp  ), // assert = sh_pcim_id_width_lp
  .axi_addr_width_p(sh_pcis_addr_width_lp), // assert = sh_pcim_addr_width_lp
  .axi_data_width_p(sh_pcis_data_width_lp)  // assert = sh_pcim_data_width_lp
) axi_fsb_adapters (
  .clk_i         (clk             ),
  .reset_i       (~axi_fsb_rstn   ),
  .s0_axil_bus_i (sh_ocl_0_i      ),
  .s0_axil_bus_o (sh_ocl_0_o      ),
  .s1_axil_bus_i (sh_ocl_2_i      ),
  .s1_axil_bus_o (sh_ocl_2_o      ),
  .s2_axil_bus_i (sh_ocl_1_i      ),
  .s2_axil_bus_o (sh_ocl_1_o      ),
  .s1_axi_bus_i  (sh_pcis_mosi_bus),
  .s1_axi_bus_o  (sh_pcis_miso_bus),
  .m0_fsb_v_i    (m0_fsb_v_i      ),
  .m0_fsb_data_i (m0_fsb_data_i   ),
  .m0_fsb_yumi_o (m0_fsb_yumi_o   ),
  .m0_fsb_v_o    (m0_fsb_v_o      ),
  .m0_fsb_data_o (m0_fsb_data_o   ),
  .m0_fsb_ready_i(m0_fsb_ready_i  ),
  .m1_fsb_v_i    (                ),
  .m1_fsb_data_i (                ),
  .m1_fsb_yumi_o (                ),
  .m1_fsb_v_o    (                ),
  .m1_fsb_data_o (                ),
  .m1_fsb_ready_i(                ),
  .m_axi_bus_i   (pcim_0_i        ),
  .m_axi_bus_o   (pcim_0_o        ),
  .s_fsb_v_i     (s_fsb_v_i       ),
  .s_fsb_data_i  (s_fsb_data_i    ),
  .s_fsb_yumi_o  (s_fsb_yumi_o    )
);


// axis master
//-------------------------------------------------
`declare_bsg_axis_bus_s(512, bsg_axis_mosi_bus_s, bsg_axis_miso_bus_s);
bsg_axis_mosi_bus_s axis_gen_mosi_bus, m_fifo_s_axis_i;
bsg_axis_miso_bus_s axis_gen_miso_bus, m_fifo_s_axis_o;

(* dont_touch = "true" *) logic axi_axis_rstn;
lib_pipe #(.WIDTH(1), .STAGES(4)) AXI_AXIS_RST_N (.clk(clk), .rst_n(1'b1), .in_bus(sync_rst_n), .out_bus(axi_axis_rstn));
bsg_axis_gen_master #(.data_width_p(512)) axis_master (
  .clk_i       (clk              ),
  .reset_i     (~axi_axis_rstn   ),
  .en_i        (1'b1             ),
  .m_axis_bus_i(axis_gen_miso_bus),
  .m_axis_bus_o(axis_gen_mosi_bus),
  .loop_done   (                 )
);

bsg_fifo_1r1w_small #(
  .width_p           (512),
  .els_p             (8  ),
  .ready_THEN_valid_p(0  )
) axis_fifo_512 (
  .clk_i  (clk                         ),
  .reset_i(~axi_axis_rstn              ),
  .v_i    (axis_gen_mosi_bus.txd_tvalid),
  .ready_o(axis_gen_miso_bus.txd_tready),
  .data_i (axis_gen_mosi_bus.txd_tdata ),
  .v_o    (m_fifo_s_axis_i.txd_tvalid  ),
  .data_o (m_fifo_s_axis_i.txd_tdata   ),
  .yumi_i (m_fifo_s_axis_o.txd_tready  )
);

m_axi4_s_axis_adapter axi4_adapter (
  .clk_i       (clk            ),
  .reset_i     (~axi_axis_rstn ),
  .s_axil_bus_i(sh_ocl_3_i     ),
  .s_axil_bus_o(sh_ocl_3_o     ),
  .m_axi_bus_i (pcim_1_i       ),
  .m_axi_bus_o (pcim_1_o       ),
  .s_axis_bus_i(m_fifo_s_axis_i),
  .s_axis_bus_o(m_fifo_s_axis_o),
  .atg_dst_sel (               )
);



//---------------------------------------------------------------
//                    axi - fsb adapters                        |
//                                                              |
//---------------------------------------------------------------
`ifndef DISABLE_VJTAG_DEBUG

// Flop for timing global clock counter
logic[63:0] sh_cl_glcount0_q;

always_ff @(posedge clk_main_a0)
   if (!sync_rst_n)
      sh_cl_glcount0_q <= 0;
   else
      sh_cl_glcount0_q <= sh_cl_glcount0;


   ila_0 CL_ILA_0 (
                   .clk    (clk_main_a0),
                   .probe0 (sh_ocl_mosi_bus.awvalid),
                   .probe1 ({sh_ocl_mosi_bus.wdata, sh_ocl_mosi_bus.awaddr}),
                   .probe2 (sh_ocl_miso_bus.awready),
                   .probe3 (sh_ocl_mosi_bus.wvalid),
                   .probe4 ({sh_ocl_miso_bus.rdata, sh_ocl_mosi_bus.araddr}),
                   .probe5 (sh_ocl_miso_bus.wready)
                   );


// // Integrated Logic Analyzers (ILA)
//   cl_ila_axil axil_analyser (
//     .clk    (clk_main_a0            ),
//     .probe0 (sh_ocl_mosi_bus.awvalid),
//     .probe1 (sh_ocl_mosi_bus.awaddr ),
//     .probe2 (sh_ocl_miso_bus.awready),
//     .probe3 (sh_ocl_mosi_bus.wvalid ),
//     .probe4 (sh_ocl_mosi_bus.wdata  ),
//     .probe5 (sh_ocl_mosi_bus
//       .wstrb  ),
//     .probe6 (sh_ocl_miso_bus.wready ),
//     .probe7 (sh_ocl_miso_bus.bresp  ),
//     .probe8 (sh_ocl_miso_bus.bvalid ),
//     .probe9 (sh_ocl_mosi_bus.bready ),
//     .probe10(sh_ocl_mosi_bus.araddr ),
//     .probe11(sh_ocl_mosi_bus.arvalid),
//     .probe12(sh_ocl_miso_bus.arready),
//     .probe13(sh_ocl_miso_bus.rdata  ),
//     .probe14(sh_ocl_miso_bus.rresp  ),
//     .probe15(sh_ocl_miso_bus.rvalid ),
//     .probe16(sh_ocl_rready          ),
//     .probe17(0                      ),
//     .probe18(0                      )
//   );

// Debug Bridge 
 cl_debug_bridge CL_DEBUG_BRIDGE (
      .clk(clk_main_a0),
      .S_BSCAN_drck(drck),
      .S_BSCAN_shift(shift),
      .S_BSCAN_tdi(tdi),
      .S_BSCAN_update(update),
      .S_BSCAN_sel(sel),
      .S_BSCAN_tdo(tdo),
      .S_BSCAN_tms(tms),
      .S_BSCAN_tck(tck),
      .S_BSCAN_runtest(runtest),
      .S_BSCAN_reset(reset),
      .S_BSCAN_capture(capture),
      .S_BSCAN_bscanid_en(bscanid_en)
   );

`endif //  `ifndef DISABLE_VJTAG_DEBUG

endmodule
