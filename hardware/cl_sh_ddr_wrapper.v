/**
*  cl_sh_ddr_wrapper.v
*
*/

`include "bsg_axi_bus_pkg.vh"
`include "cl_manycore_defines.vh"

module cl_sh_ddr_wrapper #(
  parameter num_axi4_p = 3
  , parameter axi4_id_width_p = "inv"
  , parameter axi4_addr_width_p = "inv"
  , parameter axi4_data_width_p = "inv"
  , localparam axi4_mosi_bus_width_lp = `bsg_axi4_mosi_bus_width(num_axi4_p, axi4_id_width_p, axi4_addr_width_p, axi4_data_width_p)
  , localparam axi4_miso_bus_width_lp = `bsg_axi4_miso_bus_width(num_axi4_p, axi4_id_width_p, axi4_addr_width_p, axi4_data_width_p)
) (
  input                                     clk_i
  ,input                                     reset_i
  // ------------------- STATS -----------------------------------------------------------
  ,output                                    cl_RST_DIMM_D_N
  ,input  [            num_axi4_p-1:0][ 7:0] sh_ddr_stat_addr_i
  ,input  [            num_axi4_p-1:0]       sh_ddr_stat_wr_i
  ,input  [            num_axi4_p-1:0]       sh_ddr_stat_rd_i
  ,input  [            num_axi4_p-1:0][31:0] sh_ddr_stat_wdata_i
  ,output [            num_axi4_p-1:0]       ddr_sh_stat_ack_o
  ,output [            num_axi4_p-1:0][31:0] ddr_sh_stat_rdata_o
  ,output [            num_axi4_p-1:0][ 7:0] ddr_sh_stat_int_o
  ,input  [axi4_mosi_bus_width_lp-1:0]       s_axi4_i
  ,output [axi4_miso_bus_width_lp-1:0]       s_axi4_o
  // ------------------- DDR4 x72 RDIMM 2100 Interface A ----------------------------------
  ,input                                     CLK_300M_DIMM0_DP
  ,input                                     CLK_300M_DIMM0_DN
  ,output                                    M_A_ACT_N
  ,output [                      16:0]       M_A_MA
  ,output [                       1:0]       M_A_BA
  ,output [                       1:0]       M_A_BG
  ,output [                       0:0]       M_A_CKE
  ,output [                       0:0]       M_A_ODT
  ,output [                       0:0]       M_A_CS_N
  ,output [                       0:0]       M_A_CLK_DN
  ,output [                       0:0]       M_A_CLK_DP
  ,output                                    M_A_PAR
  ,inout  [                      63:0]       M_A_DQ
  ,inout  [                       7:0]       M_A_ECC
  ,inout  [                      17:0]       M_A_DQS_DP
  ,inout  [                      17:0]       M_A_DQS_DN
  ,output                                    cl_RST_DIMM_A_N
  // ------------------- DDR4 x72 RDIMM 2100 Interface B ----------------------------------
  ,input                                     CLK_300M_DIMM1_DP
  ,input                                     CLK_300M_DIMM1_DN
  ,output                                    M_B_ACT_N
  ,output [                      16:0]       M_B_MA
  ,output [                       1:0]       M_B_BA
  ,output [                       1:0]       M_B_BG
  ,output [                       0:0]       M_B_CKE
  ,output [                       0:0]       M_B_ODT
  ,output [                       0:0]       M_B_CS_N
  ,output [                       0:0]       M_B_CLK_DN
  ,output [                       0:0]       M_B_CLK_DP
  ,output                                    M_B_PAR
  ,inout  [                      63:0]       M_B_DQ
  ,inout  [                       7:0]       M_B_ECC
  ,inout  [                      17:0]       M_B_DQS_DP
  ,inout  [                      17:0]       M_B_DQS_DN
  ,output                                    cl_RST_DIMM_B_N
  // ------------------- DDR4 x72 RDIMM 2100 Interface D ----------------------------------
  ,input                                     CLK_300M_DIMM3_DP
  ,input                                     CLK_300M_DIMM3_DN
  ,output                                    M_D_ACT_N
  ,output [                      16:0]       M_D_MA
  ,output [                       1:0]       M_D_BA
  ,output [                       1:0]       M_D_BG
  ,output [                       0:0]       M_D_CKE
  ,output [                       0:0]       M_D_ODT
  ,output [                       0:0]       M_D_CS_N
  ,output [                       0:0]       M_D_CLK_DN
  ,output [                       0:0]       M_D_CLK_DP
  ,output                                    M_D_PAR
  ,inout  [                      63:0]       M_D_DQ
  ,inout  [                       7:0]       M_D_ECC
  ,inout  [                      17:0]       M_D_DQS_DP
  ,inout  [                      17:0]       M_D_DQS_DN
);

`declare_bsg_axi4_bus_s(num_axi4_p, axi4_id_width_p, axi4_addr_width_p, axi4_data_width_p, bsg_axi4_hs_mosi_s, bsg_axi4_hs_miso_s);

bsg_axi4_hs_mosi_s m_axi4_ddr_hs_lo;
bsg_axi4_hs_miso_s m_axi4_ddr_hs_li;

assign m_axi4_ddr_hs_lo = s_axi4_i;
assign s_axi4_o         = m_axi4_ddr_hs_li;

localparam axi4_ddr_id_width_lp   = 16 ;
localparam axi4_ddr_addr_width_lp = 64 ;
localparam axi4_ddr_data_width_lp = 512;

`declare_bsg_axi4_bus_s(num_axi4_p, axi4_ddr_id_width_lp, axi4_ddr_addr_width_lp, axi4_data_width_p, axi4_cl_ddr_mosi_s, axi4_cl_ddr_miso_s);

axi4_cl_ddr_mosi_s s_axi4_cl_ddr_li;
axi4_cl_ddr_miso_s s_axi4_cl_ddr_lo;

// signal cast to ddr
//
for (genvar i = 0; i < num_axi4_p; i++) begin : ddr_cast

  // mosi
  assign s_axi4_cl_ddr_li.awid[i] = m_axi4_ddr_hs_lo.awid[i];
  assign s_axi4_cl_ddr_li.awaddr[i]  = m_axi4_ddr_hs_lo.awaddr[i];
  assign s_axi4_cl_ddr_li.awlen[i]   = m_axi4_ddr_hs_lo.awlen[i];
  assign s_axi4_cl_ddr_li.awsize[i]  = m_axi4_ddr_hs_lo.awsize[i];
  assign s_axi4_cl_ddr_li.awburst[i] = m_axi4_ddr_hs_lo.awburst[i];
  // assign s_axi4_cl_ddr_li.awlock[i] = m_axi4_ddr_hs_lo.awlock[i];
  // assign s_axi4_cl_ddr_li.awcache[i] = m_axi4_ddr_hs_lo.awcache[i];
  // assign s_axi4_cl_ddr_li.awprot[i] = m_axi4_ddr_hs_lo.awprot[i];
  // assign s_axi4_cl_ddr_li.awqos[i] = m_axi4_ddr_hs_lo.awqos[i];
  // assign s_axi4_cl_ddr_li.awregion[i] = m_axi4_ddr_hs_lo.awregion[i];
  assign s_axi4_cl_ddr_li.awvalid[i] = m_axi4_ddr_hs_lo.awvalid[i];

  assign s_axi4_cl_ddr_li.wdata[i]  = m_axi4_ddr_hs_lo.wdata[i];
  assign s_axi4_cl_ddr_li.wstrb[i]  = m_axi4_ddr_hs_lo.wstrb[i];
  assign s_axi4_cl_ddr_li.wlast[i]  = m_axi4_ddr_hs_lo.wlast[i];
  assign s_axi4_cl_ddr_li.wvalid[i] = m_axi4_ddr_hs_lo.wvalid[i];

  assign s_axi4_cl_ddr_li.bready[i] = m_axi4_ddr_hs_lo.bready[i];

  assign s_axi4_cl_ddr_li.arid[i]    = m_axi4_ddr_hs_lo.arid[i];
  assign s_axi4_cl_ddr_li.araddr[i]  = m_axi4_ddr_hs_lo.araddr[i];
  assign s_axi4_cl_ddr_li.arlen[i]   = m_axi4_ddr_hs_lo.arlen[i];
  assign s_axi4_cl_ddr_li.arsize[i]  = m_axi4_ddr_hs_lo.arsize[i];
  assign s_axi4_cl_ddr_li.arburst[i] = m_axi4_ddr_hs_lo.arburst[i];
  // assign s_axi4_cl_ddr_li.arlock[i] = m_axi4_ddr_hs_lo.arlock[i];
  // assign s_axi4_cl_ddr_li.arcache[i] = m_axi4_ddr_hs_lo.arcache[i];
  // assign s_axi4_cl_ddr_li.arprot[i] = m_axi4_ddr_hs_lo.arprot[i];
  // assign s_axi4_cl_ddr_li.arqos[i] = m_axi4_ddr_hs_lo.arqos[i];
  // assign s_axi4_cl_ddr_li.arregion[i] = m_axi4_ddr_hs_lo.arregion[i];
  assign s_axi4_cl_ddr_li.arvalid[i] = m_axi4_ddr_hs_lo.arvalid[i];

  assign s_axi4_cl_ddr_li.rready[i] = m_axi4_ddr_hs_lo.rready[i];

  // miso
  assign m_axi4_ddr_hs_li.awready[i] = s_axi4_cl_ddr_lo.awready[i];

  assign m_axi4_ddr_hs_li.wready[i] = s_axi4_cl_ddr_lo.wready[i];

  assign m_axi4_ddr_hs_li.bid[i]    = s_axi4_cl_ddr_lo.bid[i];
  assign m_axi4_ddr_hs_li.bresp[i]  = s_axi4_cl_ddr_lo.bresp[i];
  assign m_axi4_ddr_hs_li.bvalid[i] = s_axi4_cl_ddr_lo.bvalid[i];

  assign m_axi4_ddr_hs_li.arready[i] = s_axi4_cl_ddr_lo.arready[i];

  assign m_axi4_ddr_hs_li.rid[i]    = s_axi4_cl_ddr_lo.rid[i];
  assign m_axi4_ddr_hs_li.rdata[i]  = s_axi4_cl_ddr_lo.rdata[i];
  assign m_axi4_ddr_hs_li.rresp[i]  = s_axi4_cl_ddr_lo.rresp[i];
  assign m_axi4_ddr_hs_li.rlast[i]  = s_axi4_cl_ddr_lo.rlast[i];
  assign m_axi4_ddr_hs_li.rvalid[i] = s_axi4_cl_ddr_lo.rvalid[i];

end : ddr_cast

wire [15:0] cl_sh_ddr_wid_2d[2:0];

assign cl_sh_ddr_wid_2d = '{16'b0, 16'b0, 16'b0};

//convert to 2D
logic [15:0] cl_sh_ddr_awid_2d   [2:0];
logic [63:0] cl_sh_ddr_awaddr_2d [2:0];
logic [ 7:0] cl_sh_ddr_awlen_2d  [2:0];
logic [ 2:0] cl_sh_ddr_awsize_2d [2:0];
logic [ 1:0] cl_sh_ddr_awburst_2d[2:0];
logic        cl_sh_ddr_awvalid_2d[2:0];
logic [ 2:0] sh_cl_ddr_awready_2d     ;

logic [ 15:0] cl_sh_ddr_wid_2d   [2:0];
logic [511:0] cl_sh_ddr_wdata_2d [2:0];
logic [ 63:0] cl_sh_ddr_wstrb_2d [2:0];
logic [  2:0] cl_sh_ddr_wlast_2d      ;
logic [  2:0] cl_sh_ddr_wvalid_2d     ;
logic [  2:0] sh_cl_ddr_wready_2d     ;

logic [15:0] sh_cl_ddr_bid_2d   [2:0];
logic [ 1:0] sh_cl_ddr_bresp_2d [2:0];
logic [ 2:0] sh_cl_ddr_bvalid_2d     ;
logic [ 2:0] cl_sh_ddr_bready_2d     ;

logic [15:0] cl_sh_ddr_arid_2d   [2:0];
logic [63:0] cl_sh_ddr_araddr_2d [2:0];
logic [ 7:0] cl_sh_ddr_arlen_2d  [2:0];
logic [ 2:0] cl_sh_ddr_arsize_2d [2:0];
logic [ 1:0] cl_sh_ddr_arburst_2d[2:0];
logic [ 2:0] cl_sh_ddr_arvalid_2d     ;
logic [ 2:0] sh_cl_ddr_arready_2d     ;

logic [ 15:0] sh_cl_ddr_rid_2d   [2:0];
logic [511:0] sh_cl_ddr_rdata_2d [2:0];
logic [  1:0] sh_cl_ddr_rresp_2d [2:0];
logic [  2:0] sh_cl_ddr_rlast_2d      ;
logic [  2:0] sh_cl_ddr_rvalid_2d     ;
logic [  2:0] cl_sh_ddr_rready_2d     ;

assign cl_sh_ddr_awid_2d    = '{s_axi4_cl_ddr_li.awid[2], s_axi4_cl_ddr_li.awid[1], s_axi4_cl_ddr_li.awid[0]};
assign cl_sh_ddr_awaddr_2d  = '{s_axi4_cl_ddr_li.awaddr[2], s_axi4_cl_ddr_li.awaddr[1], s_axi4_cl_ddr_li.awaddr[0]};
assign cl_sh_ddr_awlen_2d   = '{s_axi4_cl_ddr_li.awlen[2], s_axi4_cl_ddr_li.awlen[1], s_axi4_cl_ddr_li.awlen[0]};
assign cl_sh_ddr_awsize_2d  = '{s_axi4_cl_ddr_li.awsize[2], s_axi4_cl_ddr_li.awsize[1], s_axi4_cl_ddr_li.awsize[0]};
assign cl_sh_ddr_awvalid_2d = '{s_axi4_cl_ddr_li.awvalid[2], s_axi4_cl_ddr_li.awvalid[1], s_axi4_cl_ddr_li.awvalid[0]};
assign cl_sh_ddr_awburst_2d = {s_axi4_cl_ddr_li.awburst[2], s_axi4_cl_ddr_li.awburst[1], s_axi4_cl_ddr_li.awburst[0]};
assign {s_axi4_cl_ddr_lo.awready[2], s_axi4_cl_ddr_lo.awready[1], s_axi4_cl_ddr_lo.awready[0]} = sh_cl_ddr_awready_2d;

// assign cl_sh_ddr_wid_2d = '{lcl_cl_sh_ddrd.wid, lcl_cl_sh_ddrb.wid, lcl_cl_sh_ddra.wid};
assign cl_sh_ddr_wdata_2d  = '{s_axi4_cl_ddr_li.wdata[2], s_axi4_cl_ddr_li.wdata[1], s_axi4_cl_ddr_li.wdata[0]};
assign cl_sh_ddr_wstrb_2d  = '{s_axi4_cl_ddr_li.wstrb[2], s_axi4_cl_ddr_li.wstrb[1], s_axi4_cl_ddr_li.wstrb[0]};
assign cl_sh_ddr_wlast_2d  = {s_axi4_cl_ddr_li.wlast[2], s_axi4_cl_ddr_li.wlast[1], s_axi4_cl_ddr_li.wlast[0]};
assign cl_sh_ddr_wvalid_2d = {s_axi4_cl_ddr_li.wvalid[2], s_axi4_cl_ddr_li.wvalid[1], s_axi4_cl_ddr_li.wvalid[0]};
assign {s_axi4_cl_ddr_lo.wready[2], s_axi4_cl_ddr_lo.wready[1], s_axi4_cl_ddr_lo.wready[0]} = sh_cl_ddr_wready_2d;

assign {s_axi4_cl_ddr_lo.bid[2], s_axi4_cl_ddr_lo.bid[1], s_axi4_cl_ddr_lo.bid[0]} = {sh_cl_ddr_bid_2d[2], sh_cl_ddr_bid_2d[1], sh_cl_ddr_bid_2d[0]};
assign {s_axi4_cl_ddr_lo.bresp[2], s_axi4_cl_ddr_lo.bresp[1], s_axi4_cl_ddr_lo.bresp[0]} = {sh_cl_ddr_bresp_2d[2], sh_cl_ddr_bresp_2d[1], sh_cl_ddr_bresp_2d[0]};
assign {s_axi4_cl_ddr_lo.bvalid[2], s_axi4_cl_ddr_lo.bvalid[1], s_axi4_cl_ddr_lo.bvalid[0]} = sh_cl_ddr_bvalid_2d;
assign cl_sh_ddr_bready_2d = {s_axi4_cl_ddr_li.bready[2], s_axi4_cl_ddr_li.bready[1], s_axi4_cl_ddr_li.bready[0]};

assign cl_sh_ddr_arid_2d    = '{s_axi4_cl_ddr_li.arid[2], s_axi4_cl_ddr_li.arid[1], s_axi4_cl_ddr_li.arid[0]};
assign cl_sh_ddr_araddr_2d  = '{s_axi4_cl_ddr_li.araddr[2], s_axi4_cl_ddr_li.araddr[1], s_axi4_cl_ddr_li.araddr[0]};
assign cl_sh_ddr_arlen_2d   = '{s_axi4_cl_ddr_li.arlen[2], s_axi4_cl_ddr_li.arlen[1], s_axi4_cl_ddr_li.arlen[0]};
assign cl_sh_ddr_arsize_2d  = '{s_axi4_cl_ddr_li.arsize[2], s_axi4_cl_ddr_li.arsize[1], s_axi4_cl_ddr_li.arsize[0]};
assign cl_sh_ddr_arvalid_2d = {s_axi4_cl_ddr_li.arvalid[2], s_axi4_cl_ddr_li.arvalid[1], s_axi4_cl_ddr_li.arvalid[0]};
assign cl_sh_ddr_arburst_2d = {s_axi4_cl_ddr_li.arburst[2], s_axi4_cl_ddr_li.arburst[1], s_axi4_cl_ddr_li.arburst[0]};
assign {s_axi4_cl_ddr_lo.arready[2], s_axi4_cl_ddr_lo.arready[1], s_axi4_cl_ddr_lo.arready[0]} = sh_cl_ddr_arready_2d;

assign {s_axi4_cl_ddr_lo.rid[2], s_axi4_cl_ddr_lo.rid[1], s_axi4_cl_ddr_lo.rid[0]} = {sh_cl_ddr_rid_2d[2], sh_cl_ddr_rid_2d[1], sh_cl_ddr_rid_2d[0]};
assign {s_axi4_cl_ddr_lo.rresp[2], s_axi4_cl_ddr_lo.rresp[1], s_axi4_cl_ddr_lo.rresp[0]} = {sh_cl_ddr_rresp_2d[2], sh_cl_ddr_rresp_2d[1], sh_cl_ddr_rresp_2d[0]};
assign {s_axi4_cl_ddr_lo.rdata[2], s_axi4_cl_ddr_lo.rdata[1], s_axi4_cl_ddr_lo.rdata[0]} = {sh_cl_ddr_rdata_2d[2], sh_cl_ddr_rdata_2d[1], sh_cl_ddr_rdata_2d[0]};
assign {s_axi4_cl_ddr_lo.rlast[2], s_axi4_cl_ddr_lo.rlast[1], s_axi4_cl_ddr_lo.rlast[0]} = sh_cl_ddr_rlast_2d;
assign {s_axi4_cl_ddr_lo.rvalid[2], s_axi4_cl_ddr_lo.rvalid[1], s_axi4_cl_ddr_lo.rvalid[0]} = sh_cl_ddr_rvalid_2d;
assign cl_sh_ddr_rready_2d = {s_axi4_cl_ddr_li.rready[2], s_axi4_cl_ddr_li.rready[1], s_axi4_cl_ddr_li.rready[0]};


`ifndef AXI_MEMORY_MODEL   

  //-----------------------------------------
  // DDR controller instantiation
  //-----------------------------------------
  localparam NUM_CFG_STGS_CL_DDR_LP = 8;

  logic [ 7:0] sh_ddr_stat_addr_q [2:0];
  logic [ 2:0] sh_ddr_stat_wr_q        ;
  logic [ 2:0] sh_ddr_stat_rd_q        ;
  logic [31:0] sh_ddr_stat_wdata_q[2:0];
  logic [ 2:0] ddr_sh_stat_ack_q       ;
  logic [31:0] ddr_sh_stat_rdata_q[2:0];
  logic [ 7:0] ddr_sh_stat_int_q  [2:0];

  for (genvar i = 0; i < num_axi4_p; i++) begin : stat_buf

    lib_pipe #(.WIDTH(1+1+8+32), .STAGES(NUM_CFG_STGS_CL_DDR_LP)) PIPE_DDR_STAT (
      .clk    (clk_i                                                                                    ),
      .rst_n  (~reset_i                                                                                 ),
      .in_bus ({sh_ddr_stat_wr_i[i], sh_ddr_stat_rd_i[i], sh_ddr_stat_addr_i[i], sh_ddr_stat_wdata_i[i]}),
      .out_bus({sh_ddr_stat_wr_q[i], sh_ddr_stat_rd_q[i], sh_ddr_stat_addr_q[i], sh_ddr_stat_wdata_q[i]})
    );


    lib_pipe #(.WIDTH(1+8+32), .STAGES(NUM_CFG_STGS_CL_DDR_LP)) PIPE_DDR_STAT_ACK (
      .clk    (clk_i                                                               ),
      .rst_n  (~reset_i                                                            ),
      .in_bus ({ddr_sh_stat_ack_q[i], ddr_sh_stat_int_q[i], ddr_sh_stat_rdata_q[i]}),
      .out_bus({ddr_sh_stat_ack_o[i], ddr_sh_stat_int_o[i], ddr_sh_stat_rdata_o[i]})
    );

  end

  (* dont_touch = "true" *) logic sh_ddr_sync_rst_n;
  lib_pipe #(.WIDTH(1), .STAGES(4)) SH_DDR_SLC_RST_N (.clk(clk_i), .rst_n(1'b1), .in_bus(~reset_i), .out_bus(sh_ddr_sync_rst_n));
  sh_ddr #(
    .DDR_A_PRESENT(`DDR_A_PRESENT),
    .DDR_B_PRESENT(`DDR_B_PRESENT),
    .DDR_D_PRESENT(`DDR_D_PRESENT)
  ) SH_DDR (
    .clk               (clk_i                 ),
    .rst_n             (sh_ddr_sync_rst_n     ),

    .stat_clk          (clk_i                 ),
    .stat_rst_n        (sh_ddr_sync_rst_n     ),


    .CLK_300M_DIMM0_DP (CLK_300M_DIMM0_DP     ),
    .CLK_300M_DIMM0_DN (CLK_300M_DIMM0_DN     ),
    .M_A_ACT_N         (M_A_ACT_N             ),
    .M_A_MA            (M_A_MA                ),
    .M_A_BA            (M_A_BA                ),
    .M_A_BG            (M_A_BG                ),
    .M_A_CKE           (M_A_CKE               ),
    .M_A_ODT           (M_A_ODT               ),
    .M_A_CS_N          (M_A_CS_N              ),
    .M_A_CLK_DN        (M_A_CLK_DN            ),
    .M_A_CLK_DP        (M_A_CLK_DP            ),
    .M_A_PAR           (M_A_PAR               ),
    .M_A_DQ            (M_A_DQ                ),
    .M_A_ECC           (M_A_ECC               ),
    .M_A_DQS_DP        (M_A_DQS_DP            ),
    .M_A_DQS_DN        (M_A_DQS_DN            ),
    .cl_RST_DIMM_A_N   (cl_RST_DIMM_A_N       ),


    .CLK_300M_DIMM1_DP (CLK_300M_DIMM1_DP     ),
    .CLK_300M_DIMM1_DN (CLK_300M_DIMM1_DN     ),
    .M_B_ACT_N         (M_B_ACT_N             ),
    .M_B_MA            (M_B_MA                ),
    .M_B_BA            (M_B_BA                ),
    .M_B_BG            (M_B_BG                ),
    .M_B_CKE           (M_B_CKE               ),
    .M_B_ODT           (M_B_ODT               ),
    .M_B_CS_N          (M_B_CS_N              ),
    .M_B_CLK_DN        (M_B_CLK_DN            ),
    .M_B_CLK_DP        (M_B_CLK_DP            ),
    .M_B_PAR           (M_B_PAR               ),
    .M_B_DQ            (M_B_DQ                ),
    .M_B_ECC           (M_B_ECC               ),
    .M_B_DQS_DP        (M_B_DQS_DP            ),
    .M_B_DQS_DN        (M_B_DQS_DN            ),
    .cl_RST_DIMM_B_N   (cl_RST_DIMM_B_N       ),

    .CLK_300M_DIMM3_DP (CLK_300M_DIMM3_DP     ),
    .CLK_300M_DIMM3_DN (CLK_300M_DIMM3_DN     ),
    .M_D_ACT_N         (M_D_ACT_N             ),
    .M_D_MA            (M_D_MA                ),
    .M_D_BA            (M_D_BA                ),
    .M_D_BG            (M_D_BG                ),
    .M_D_CKE           (M_D_CKE               ),
    .M_D_ODT           (M_D_ODT               ),
    .M_D_CS_N          (M_D_CS_N              ),
    .M_D_CLK_DN        (M_D_CLK_DN            ),
    .M_D_CLK_DP        (M_D_CLK_DP            ),
    .M_D_PAR           (M_D_PAR               ),
    .M_D_DQ            (M_D_DQ                ),
    .M_D_ECC           (M_D_ECC               ),
    .M_D_DQS_DP        (M_D_DQS_DP            ),
    .M_D_DQS_DN        (M_D_DQS_DN            ),
    .cl_RST_DIMM_D_N   (cl_RST_DIMM_D_N       ),

    //------------------------------------------------------
    // DDR-4 Interface from CL (AXI-4)
    //------------------------------------------------------
    .cl_sh_ddr_awid    (cl_sh_ddr_awid_2d     ),
    .cl_sh_ddr_awaddr  (cl_sh_ddr_awaddr_2d   ),
    .cl_sh_ddr_awlen   (cl_sh_ddr_awlen_2d    ),
    .cl_sh_ddr_awsize  (cl_sh_ddr_awsize_2d   ),
    .cl_sh_ddr_awvalid (cl_sh_ddr_awvalid_2d  ),
    .cl_sh_ddr_awburst (cl_sh_ddr_awburst_2d  ),
    .sh_cl_ddr_awready (sh_cl_ddr_awready_2d  ),

    .cl_sh_ddr_wid     (cl_sh_ddr_wid_2d      ),
    .cl_sh_ddr_wdata   (cl_sh_ddr_wdata_2d    ),
    .cl_sh_ddr_wstrb   (cl_sh_ddr_wstrb_2d    ),
    .cl_sh_ddr_wlast   (cl_sh_ddr_wlast_2d    ),
    .cl_sh_ddr_wvalid  (cl_sh_ddr_wvalid_2d   ),
    .sh_cl_ddr_wready  (sh_cl_ddr_wready_2d   ),

    .sh_cl_ddr_bid     (sh_cl_ddr_bid_2d      ),
    .sh_cl_ddr_bresp   (sh_cl_ddr_bresp_2d    ),
    .sh_cl_ddr_bvalid  (sh_cl_ddr_bvalid_2d   ),
    .cl_sh_ddr_bready  (cl_sh_ddr_bready_2d   ),

    .cl_sh_ddr_arid    (cl_sh_ddr_arid_2d     ),
    .cl_sh_ddr_araddr  (cl_sh_ddr_araddr_2d   ),
    .cl_sh_ddr_arlen   (cl_sh_ddr_arlen_2d    ),
    .cl_sh_ddr_arsize  (cl_sh_ddr_arsize_2d   ),
    .cl_sh_ddr_arvalid (cl_sh_ddr_arvalid_2d  ),
    .cl_sh_ddr_arburst (cl_sh_ddr_arburst_2d  ),
    .sh_cl_ddr_arready (sh_cl_ddr_arready_2d  ),

    .sh_cl_ddr_rid     (sh_cl_ddr_rid_2d      ),
    .sh_cl_ddr_rdata   (sh_cl_ddr_rdata_2d    ),
    .sh_cl_ddr_rresp   (sh_cl_ddr_rresp_2d    ),
    .sh_cl_ddr_rlast   (sh_cl_ddr_rlast_2d    ),
    .sh_cl_ddr_rvalid  (sh_cl_ddr_rvalid_2d   ),
    .cl_sh_ddr_rready  (cl_sh_ddr_rready_2d   ),

    .sh_cl_ddr_is_ready(lcl_sh_cl_ddr_is_ready),

    .sh_ddr_stat_addr0 (sh_ddr_stat_addr_q[0] ),
    .sh_ddr_stat_wr0   (sh_ddr_stat_wr_q[0]   ),
    .sh_ddr_stat_rd0   (sh_ddr_stat_rd_q[0]   ),
    .sh_ddr_stat_wdata0(sh_ddr_stat_wdata_q[0]),
    .ddr_sh_stat_ack0  (ddr_sh_stat_ack_q[0]  ),
    .ddr_sh_stat_rdata0(ddr_sh_stat_rdata_q[0]),
    .ddr_sh_stat_int0  (ddr_sh_stat_int_q[0]  ),

    .sh_ddr_stat_addr1 (sh_ddr_stat_addr_q[1] ),
    .sh_ddr_stat_wr1   (sh_ddr_stat_wr_q[1]   ),
    .sh_ddr_stat_rd1   (sh_ddr_stat_rd_q[1]   ),
    .sh_ddr_stat_wdata1(sh_ddr_stat_wdata_q[1]),
    .ddr_sh_stat_ack1  (ddr_sh_stat_ack_q[1]  ),
    .ddr_sh_stat_rdata1(ddr_sh_stat_rdata_q[1]),
    .ddr_sh_stat_int1  (ddr_sh_stat_int_q[1]  ),

    .sh_ddr_stat_addr2 (sh_ddr_stat_addr_q[2] ),
    .sh_ddr_stat_wr2   (sh_ddr_stat_wr_q[2]   ),
    .sh_ddr_stat_rd2   (sh_ddr_stat_rd_q[2]   ),
    .sh_ddr_stat_wdata2(sh_ddr_stat_wdata_q[2]),
    .ddr_sh_stat_ack2  (ddr_sh_stat_ack_q[2]  ),
    .ddr_sh_stat_rdata2(ddr_sh_stat_rdata_q[2]),
    .ddr_sh_stat_int2  (ddr_sh_stat_int_q[2]  )
  );

`else 

    axi_mem_model axi4_ddr_model (
      .clk_core         (clk_i               ),
      .rst_n            (~reset_i            ),
      .cl_sh_ddr_awid   (cl_sh_ddr_awid_2d   ),
      .cl_sh_ddr_awaddr (cl_sh_ddr_awaddr_2d ),
      .cl_sh_ddr_awlen  (cl_sh_ddr_awlen_2d  ),
      .cl_sh_ddr_awsize (cl_sh_ddr_awsize_2d ),
      .cl_sh_ddr_awvalid(cl_sh_ddr_awvalid_2d),
      .cl_sh_ddr_awburst(cl_sh_ddr_awburst_2d),
      .sh_cl_ddr_awready(sh_cl_ddr_awready_2d),
      
      .cl_sh_ddr_wid    (cl_sh_ddr_wid_2d    ),
      .cl_sh_ddr_wdata  (cl_sh_ddr_wdata_2d  ),
      .cl_sh_ddr_wstrb  (cl_sh_ddr_wstrb_2d  ),
      .cl_sh_ddr_wlast  (cl_sh_ddr_wlast_2d  ),
      .cl_sh_ddr_wvalid (cl_sh_ddr_wvalid_2d ),
      .sh_cl_ddr_wready (sh_cl_ddr_wready_2d ),
      
      .sh_cl_ddr_bid    (sh_cl_ddr_bid_2d    ),
      .sh_cl_ddr_bresp  (sh_cl_ddr_bresp_2d  ),
      .sh_cl_ddr_bvalid (sh_cl_ddr_bvalid_2d ),
      .cl_sh_ddr_bready (cl_sh_ddr_bready_2d ),
      
      .cl_sh_ddr_arid   (cl_sh_ddr_arid_2d   ),
      .cl_sh_ddr_araddr (cl_sh_ddr_araddr_2d ),
      .cl_sh_ddr_arlen  (cl_sh_ddr_arlen_2d  ),
      .cl_sh_ddr_arsize (cl_sh_ddr_arsize_2d ),
      .cl_sh_ddr_arvalid(cl_sh_ddr_arvalid_2d),
      .cl_sh_ddr_arburst(cl_sh_ddr_arburst_2d),
      .sh_cl_ddr_arready(sh_cl_ddr_arready_2d),
      
      .sh_cl_ddr_rid    (sh_cl_ddr_rid_2d    ),
      .sh_cl_ddr_rdata  (sh_cl_ddr_rdata_2d  ),
      .sh_cl_ddr_rresp  (sh_cl_ddr_rresp_2d  ),
      .sh_cl_ddr_rlast  (sh_cl_ddr_rlast_2d  ),
      .sh_cl_ddr_rvalid (sh_cl_ddr_rvalid_2d ),
      .cl_sh_ddr_rready (cl_sh_ddr_rready_2d )
    );

  // 
  // copy from unused_ddr_a_b_d_template.inc
  //-------------------------------------------------
  // Array Signals to Tie-off AXI interfaces to sh_ddr module
  //-------------------------------------------------
    logic         tie_zero[2:0];
    logic [  1:0] tie_zero_burst[2:0];
    logic [ 15:0] tie_zero_id[2:0];
    logic [ 63:0] tie_zero_addr[2:0];
    logic [  7:0] tie_zero_len[2:0];
    logic [511:0] tie_zero_data[2:0];
    logic [ 63:0] tie_zero_strb[2:0];

  sh_ddr #(.DDR_A_PRESENT(0),
           .DDR_B_PRESENT(0),
           .DDR_D_PRESENT(0)) SH_DDR
     (
     .clk(clk_i),
     .rst_n(~reset_i),
     .stat_clk(clk_i),
     .stat_rst_n(~reset_i),

     .CLK_300M_DIMM0_DP(CLK_300M_DIMM0_DP),
     .CLK_300M_DIMM0_DN(CLK_300M_DIMM0_DN),
     .M_A_ACT_N(M_A_ACT_N),
     .M_A_MA(M_A_MA),
     .M_A_BA(M_A_BA),
     .M_A_BG(M_A_BG),
     .M_A_CKE(M_A_CKE),
     .M_A_ODT(M_A_ODT),
     .M_A_CS_N(M_A_CS_N),
     .M_A_CLK_DN(M_A_CLK_DN),
     .M_A_CLK_DP(M_A_CLK_DP),
     .M_A_PAR(M_A_PAR),
     .M_A_DQ(M_A_DQ),
     .M_A_ECC(M_A_ECC),
     .M_A_DQS_DP(M_A_DQS_DP),
     .M_A_DQS_DN(M_A_DQS_DN),
     .cl_RST_DIMM_A_N(),
     
     .CLK_300M_DIMM1_DP(CLK_300M_DIMM1_DP),
     .CLK_300M_DIMM1_DN(CLK_300M_DIMM1_DN),
     .M_B_ACT_N(M_B_ACT_N),
     .M_B_MA(M_B_MA),
     .M_B_BA(M_B_BA),
     .M_B_BG(M_B_BG),
     .M_B_CKE(M_B_CKE),
     .M_B_ODT(M_B_ODT),
     .M_B_CS_N(M_B_CS_N),
     .M_B_CLK_DN(M_B_CLK_DN),
     .M_B_CLK_DP(M_B_CLK_DP),
     .M_B_PAR(M_B_PAR),
     .M_B_DQ(M_B_DQ),
     .M_B_ECC(M_B_ECC),
     .M_B_DQS_DP(M_B_DQS_DP),
     .M_B_DQS_DN(M_B_DQS_DN),
     .cl_RST_DIMM_B_N(),

     .CLK_300M_DIMM3_DP(CLK_300M_DIMM3_DP),
     .CLK_300M_DIMM3_DN(CLK_300M_DIMM3_DN),
     .M_D_ACT_N(M_D_ACT_N),
     .M_D_MA(M_D_MA),
     .M_D_BA(M_D_BA),
     .M_D_BG(M_D_BG),
     .M_D_CKE(M_D_CKE),
     .M_D_ODT(M_D_ODT),
     .M_D_CS_N(M_D_CS_N),
     .M_D_CLK_DN(M_D_CLK_DN),
     .M_D_CLK_DP(M_D_CLK_DP),
     .M_D_PAR(M_D_PAR),
     .M_D_DQ(M_D_DQ),
     .M_D_ECC(M_D_ECC),
     .M_D_DQS_DP(M_D_DQS_DP),
     .M_D_DQS_DN(M_D_DQS_DN),
     .cl_RST_DIMM_D_N(),

     //------------------------------------------------------
     // DDR-4 Interface from CL (AXI-4)
     //------------------------------------------------------
     .cl_sh_ddr_awid     (tie_zero_id),
     .cl_sh_ddr_awaddr   (tie_zero_addr),
     .cl_sh_ddr_awlen    (tie_zero_len),
     .cl_sh_ddr_awvalid  (tie_zero),
     .cl_sh_ddr_awburst  (tie_zero_burst),
     .sh_cl_ddr_awready  (),

     .cl_sh_ddr_wid      (tie_zero_id),
     .cl_sh_ddr_wdata    (tie_zero_data),
     .cl_sh_ddr_wstrb    (tie_zero_strb),
     .cl_sh_ddr_wlast    (3'b0),
     .cl_sh_ddr_wvalid   (3'b0),
     .sh_cl_ddr_wready   (),

     .sh_cl_ddr_bid      (),
     .sh_cl_ddr_bresp    (),
     .sh_cl_ddr_bvalid   (),
     .cl_sh_ddr_bready   (3'b0),

     .cl_sh_ddr_arid     (tie_zero_id),
     .cl_sh_ddr_araddr   (tie_zero_addr),
     .cl_sh_ddr_arlen    (tie_zero_len),
     .cl_sh_ddr_arvalid  (3'b0),
     .cl_sh_ddr_arburst  (tie_zero_burst),
     .sh_cl_ddr_arready  (),

     .sh_cl_ddr_rid      (),
     .sh_cl_ddr_rdata    (),
     .sh_cl_ddr_rresp    (),
     .sh_cl_ddr_rlast    (),
     .sh_cl_ddr_rvalid   (),
     .cl_sh_ddr_rready   (3'b0),

     .sh_cl_ddr_is_ready (),

     .sh_ddr_stat_addr0   (8'h00),
     .sh_ddr_stat_wr0     (1'b0), 
     .sh_ddr_stat_rd0     (1'b0), 
     .sh_ddr_stat_wdata0  (32'b0),
     .ddr_sh_stat_ack0   (),
     .ddr_sh_stat_rdata0 (),
     .ddr_sh_stat_int0   (),
     .sh_ddr_stat_addr1   (8'h00),
     .sh_ddr_stat_wr1     (1'b0), 
     .sh_ddr_stat_rd1     (1'b0), 
     .sh_ddr_stat_wdata1  (32'b0),
     .ddr_sh_stat_ack1   (),
     .ddr_sh_stat_rdata1 (),
     .ddr_sh_stat_int1   (),
     .sh_ddr_stat_addr2   (8'h00),
     .sh_ddr_stat_wr2     (1'b0), 
     .sh_ddr_stat_rd2     (1'b0), 
     .sh_ddr_stat_wdata2  (32'b0),
     .ddr_sh_stat_ack2   (),
     .ddr_sh_stat_rdata2 (),
     .ddr_sh_stat_int2   ()
     );

  // Tie-off AXI interfaces to sh_ddr module
    assign tie_zero[2]        =   1'b0;
    assign tie_zero[1]        =   1'b0;
    assign tie_zero[0]        =   1'b0;

    assign tie_zero_burst[2]  =   2'b01; // Only INCR is supported, must be tied to 2'b01
    assign tie_zero_burst[1]  =   2'b01;
    assign tie_zero_burst[0]  =   2'b01;

    assign tie_zero_id[2]     =  16'b0;
    assign tie_zero_id[1]     =  16'b0;
    assign tie_zero_id[0]     =  16'b0;

    assign tie_zero_addr[2]   =  64'b0;
    assign tie_zero_addr[1]   =  64'b0;
    assign tie_zero_addr[0]   =  64'b0;

    assign tie_zero_len[2]    =   8'b0;
    assign tie_zero_len[1]    =   8'b0;
    assign tie_zero_len[0]    =   8'b0;

    assign tie_zero_data[2]   = 512'b0;
    assign tie_zero_data[1]   = 512'b0;
    assign tie_zero_data[0]   = 512'b0;

    assign tie_zero_strb[2]   =  64'b0;
    assign tie_zero_strb[1]   =  64'b0;
    assign tie_zero_strb[0]   =  64'b0;

    assign ddr_sh_stat_ack_o[0]   =   1'b1; // Needed in order not to hang the interface
    assign ddr_sh_stat_rdata_o[0] =  32'b0;
    assign ddr_sh_stat_int_o[0]   =   8'b0;

    assign ddr_sh_stat_ack_o[1]   =   1'b1; // Needed in order not to hang the interface
    assign ddr_sh_stat_rdata_o[1] =  32'b0;
    assign ddr_sh_stat_int_o[1]   =   8'b0;

    assign ddr_sh_stat_ack_o[2]   =   1'b1; // Needed in order not to hang the interface
    assign ddr_sh_stat_rdata_o[2] =  32'b0;
    assign ddr_sh_stat_int_o[2]   =   8'b0;

  `endif

endmodule
