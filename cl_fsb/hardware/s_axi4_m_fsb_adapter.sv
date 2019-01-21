/**
 *  s_axi4_m_fsb_adapter.sv
 *
 *  axi4 (SH) <-> cl_bsg (CL)
 */

`include "bsg_axi_bus_pkg.vh"

module s_axi4_m_fsb_adapter #(
  sh_pcis_id_width_p = 6
  ,sh_pcis_addr_width_p = 64
  ,sh_pcis_data_width_p = 512
  ,axil_mosi_bus_width_lp = `bsg_axil_mosi_bus_width(1)
  ,axil_miso_bus_width_lp = `bsg_axil_miso_bus_width(1)
  ,axi_mosi_bus_width_lp = `bsg_axi_mosi_bus_width(1, sh_pcis_id_width_p, sh_pcis_addr_width_p, sh_pcis_data_width_p)
  ,axi_miso_bus_width_lp = `bsg_axi_miso_bus_width(1, sh_pcis_id_width_p, sh_pcis_addr_width_p, sh_pcis_data_width_p)
) (
  input                               clk_i
  ,input                               reset_i
  ,input  [axil_mosi_bus_width_lp-1:0] sh_ocl_bus_i
  ,output [axil_miso_bus_width_lp-1:0] sh_ocl_bus_o
  ,input  [ axi_mosi_bus_width_lp-1:0] sh_pcis_bus_i
  ,output [ axi_miso_bus_width_lp-1:0] sh_pcis_bus_o
  ,input m_fsb_v_i
  ,input [79:0] m_fsb_data_i
  ,output m_fsb_r_o
  ,output s_fsb_v_o
  ,output [79:0] s_fsb_data_o
  ,input s_fsb_r_i
);

parameter fpga_version_p = "virtexuplus";


`declare_bsg_axil_bus_s(1, bsg_axil_mosi_bus_s, bsg_axil_miso_bus_s);
bsg_axil_mosi_bus_s sh_ocl_bus_i_cast;
bsg_axil_miso_bus_s sh_ocl_bus_o_cast;
assign sh_ocl_bus_i_cast = sh_ocl_bus_i;
assign sh_ocl_bus_o = sh_ocl_bus_o_cast;

`declare_bsg_axi_bus_s(1, sh_pcis_id_width_p, sh_pcis_addr_width_p, sh_pcis_data_width_p,
  bsg_axi_mosi_bus_s, bsg_axi_miso_bus_s);

bsg_axi_mosi_bus_s sh_pcis_bus_i_cast, sh_pcis_bus_mosi_r;
bsg_axi_miso_bus_s sh_pcis_bus_o_cast, sh_pcis_bus_miso_r;
assign sh_pcis_bus_i_cast = sh_pcis_bus_i;
assign sh_pcis_bus_o = sh_pcis_bus_o_cast;


// flop the input PCIS_DMA bus
//---------------------------------
logic [15:0] sh_cl_dma_pcis_rid;
logic [15:0] sh_cl_dma_pcis_bid;
assign sh_pcis_bus_o_cast.rid = sh_cl_dma_pcis_rid[5:0];
assign sh_pcis_bus_o_cast.bid = sh_cl_dma_pcis_bid[5:0];
axi_register_slice AXI4_PCIS_REG_SLC (
  .aclk          (clk_i                   ),
  .aresetn       (~reset_i                ),
  .s_axi_awid    (16'h0),
  .s_axi_awaddr  (sh_pcis_bus_i_cast.awaddr   ),
  .s_axi_awlen   (sh_pcis_bus_i_cast.awlen    ),
  .s_axi_awsize  (sh_pcis_bus_i_cast.awsize   ),
  .s_axi_awburst (2'h0                    ),
  .s_axi_awlock  (1'h0                    ),
  .s_axi_awcache (4'h0                    ),
  .s_axi_awprot  (3'h0                    ),
  .s_axi_awregion(4'h0                    ),
  .s_axi_awqos   (4'h0                    ),
  .s_axi_awvalid (sh_pcis_bus_i_cast.awvalid  ),
  .s_axi_awready (sh_pcis_bus_o_cast.awready  ),
  .s_axi_wdata   (sh_pcis_bus_i_cast.wdata    ),
  .s_axi_wstrb   (sh_pcis_bus_i_cast.wstrb    ),
  .s_axi_wlast   (sh_pcis_bus_i_cast.wlast    ),
  .s_axi_wvalid  (sh_pcis_bus_i_cast.wvalid   ),
  .s_axi_wready  (sh_pcis_bus_o_cast.wready   ),
  .s_axi_bid     (sh_cl_dma_pcis_bid),
  .s_axi_bresp   (sh_pcis_bus_o_cast.bresp    ),
  .s_axi_bvalid  (sh_pcis_bus_o_cast.bvalid   ),
  .s_axi_bready  (sh_pcis_bus_i_cast.bready   ),
  .s_axi_arid    (16'h0),
  .s_axi_araddr  (sh_pcis_bus_i_cast.araddr   ),
  .s_axi_arlen   (sh_pcis_bus_i_cast.arlen    ),
  .s_axi_arsize  (sh_pcis_bus_i_cast.arsize   ),
  .s_axi_arburst (2'h0                    ),
  .s_axi_arlock  (1'h0                    ),
  .s_axi_arcache (4'h0                    ),
  .s_axi_arprot  (3'h0                    ),
  .s_axi_arregion(4'h0                    ),
  .s_axi_arqos   (4'h0                    ),
  .s_axi_arvalid (sh_pcis_bus_i_cast.arvalid  ),
  .s_axi_arready (sh_pcis_bus_o_cast.arready  ),
  .s_axi_rid     (sh_cl_dma_pcis_rid      ),
  .s_axi_rdata   (sh_pcis_bus_o_cast.rdata    ),
  .s_axi_rresp   (sh_pcis_bus_o_cast.rresp    ),
  .s_axi_rlast   (sh_pcis_bus_o_cast.rlast    ),
  .s_axi_rvalid  (sh_pcis_bus_o_cast.rvalid   ),
  .s_axi_rready  (sh_pcis_bus_i_cast.rready   ),
  .m_axi_awid    (),
  .m_axi_awaddr  (sh_pcis_bus_mosi_r.awaddr ),
  .m_axi_awlen   (sh_pcis_bus_mosi_r.awlen  ),
  .m_axi_awsize  (sh_pcis_bus_mosi_r.awsize ),
  .m_axi_awburst (                        ),
  .m_axi_awlock  (                        ),
  .m_axi_awcache (                        ),
  .m_axi_awprot  (                        ),
  .m_axi_awregion(                        ),
  .m_axi_awqos   (                        ),
  .m_axi_awvalid (sh_pcis_bus_mosi_r.awvalid),
  .m_axi_awready (sh_pcis_bus_miso_r.awready),
  .m_axi_wdata   (sh_pcis_bus_mosi_r.wdata  ),
  .m_axi_wstrb   (sh_pcis_bus_mosi_r.wstrb  ),
  .m_axi_wlast   (sh_pcis_bus_mosi_r.wlast  ),
  .m_axi_wvalid  (sh_pcis_bus_mosi_r.wvalid ),
  .m_axi_wready  (sh_pcis_bus_miso_r.wready ),
  .m_axi_bid     (16'h0),
  .m_axi_bresp   (sh_pcis_bus_miso_r.bresp  ),
  .m_axi_bvalid  (sh_pcis_bus_miso_r.bvalid ),
  .m_axi_bready  (sh_pcis_bus_mosi_r.bready ),
  .m_axi_arid    (),
  .m_axi_araddr  (sh_pcis_bus_mosi_r.araddr ),
  .m_axi_arlen   (sh_pcis_bus_mosi_r.arlen  ),
  .m_axi_arsize  (sh_pcis_bus_mosi_r.arsize ),
  .m_axi_arburst (                        ),
  .m_axi_arlock  (                        ),
  .m_axi_arcache (                        ),
  .m_axi_arprot  (                        ),
  .m_axi_arregion(                        ),
  .m_axi_arqos   (                        ),
  .m_axi_arvalid (sh_pcis_bus_mosi_r.arvalid),
  .m_axi_arready (sh_pcis_bus_miso_r.arready),
  .m_axi_rid     (16'h0                   ),
  .m_axi_rdata   (sh_pcis_bus_miso_r.rdata  ),
  .m_axi_rresp   (sh_pcis_bus_miso_r.rresp  ),
  .m_axi_rlast   (sh_pcis_bus_miso_r.rlast  ),
  .m_axi_rvalid  (sh_pcis_bus_miso_r.rvalid ),
  .m_axi_rready  (sh_pcis_bus_mosi_r.rready )
);


`declare_bsg_axis_bus_s(512, bsg_axisx512_mosi_bus_s, bsg_axisx512_miso_bus_s);
`declare_bsg_axis_bus_s(128, bsg_axisx128_mosi_bus_s, bsg_axisx128_miso_bus_s);

bsg_axisx512_mosi_bus_s mosi_axisx512_bus;
bsg_axisx512_miso_bus_s miso_axisx512_bus;
bsg_axisx128_mosi_bus_s mosi_axisx128_bus;
bsg_axisx128_miso_bus_s miso_axisx128_bus;

// convert axi4 to axis
//---------------------------------
axi_fifo_mm_s #(
  .C_FAMILY              (fpga_version_p),
  .C_S_AXI_ID_WIDTH      (4           ),
  .C_S_AXI_ADDR_WIDTH    (32          ),
  .C_S_AXI_DATA_WIDTH    (32          ),
  .C_S_AXI4_DATA_WIDTH   (512         ),
  .C_TX_FIFO_DEPTH       (512         ),
  .C_RX_FIFO_DEPTH       (512         ),
  .C_TX_FIFO_PF_THRESHOLD(507         ),
  .C_TX_FIFO_PE_THRESHOLD(2           ),
  .C_RX_FIFO_PF_THRESHOLD(507         ),
  .C_RX_FIFO_PE_THRESHOLD(2           ),
  .C_USE_TX_CUT_THROUGH  (0           ),
  .C_DATA_INTERFACE_TYPE (1           ),
  .C_BASEADDR            (32'h80000000),
  .C_HIGHADDR            (32'h80000FFF),
  .C_AXI4_BASEADDR       (32'h80001000),
  .C_AXI4_HIGHADDR       (32'h80002FFF),
  .C_HAS_AXIS_TID        (0           ),
  .C_HAS_AXIS_TDEST      (0           ),
  .C_HAS_AXIS_TUSER      (0           ),
  .C_HAS_AXIS_TSTRB      (1           ),
  .C_HAS_AXIS_TKEEP      (1           ),
  .C_AXIS_TID_WIDTH      (4           ),
  .C_AXIS_TDEST_WIDTH    (4           ),
  .C_AXIS_TUSER_WIDTH    (64          ),
  .C_USE_RX_CUT_THROUGH  (0           ),
  .C_USE_TX_DATA         (1           ),
  .C_USE_TX_CTRL         (1           ),
  .C_USE_RX_DATA         (1           )
) axi_fifo_mm_s_axi4 (
  .interrupt             (                             ), // output wire interrupt
  .s_axi_aclk            (clk_i                        ), // input wire s_axi_aclk
  .s_axi_aresetn         (~reset_i                     ), // input wire s_axi_aresetn
  .s_axi_awaddr          (sh_ocl_bus_i_cast.awaddr            ), // input wire [31 : 0] s_axi_awaddr
  .s_axi_awvalid         (sh_ocl_bus_i_cast.awvalid           ), // input wire s_axi_awvalid
  .s_axi_awready         (sh_ocl_bus_o_cast.awready           ), // output wire s_axi_awready
  .s_axi_wdata           (sh_ocl_bus_i_cast.wdata             ), // input wire [31 : 0] s_axi_wdata
  .s_axi_wstrb           (sh_ocl_bus_i_cast.wstrb             ), // input wire [3 : 0] s_axi_wstrb
  .s_axi_wvalid          (sh_ocl_bus_i_cast.wvalid            ), // input wire s_axi_wvalid
  .s_axi_wready          (sh_ocl_bus_o_cast.wready            ), // output wire s_axi_wready
  .s_axi_bresp           (sh_ocl_bus_o_cast.bresp             ), // output wire [1 : 0] s_axi_bresp
  .s_axi_bvalid          (sh_ocl_bus_o_cast.bvalid            ), // output wire s_axi_bvalid
  .s_axi_bready          (sh_ocl_bus_i_cast.bready            ), // input wire s_axi_bready
  .s_axi_araddr          (sh_ocl_bus_i_cast.araddr            ), // input wire [31 : 0] s_axi_araddr
  .s_axi_arvalid         (sh_ocl_bus_i_cast.arvalid           ), // input wire s_axi_arvalid
  .s_axi_arready         (sh_ocl_bus_o_cast.arready           ), // output wire s_axi_arready
  .s_axi_rdata           (sh_ocl_bus_o_cast.rdata             ), // output wire [31 : 0] s_axi_rdata
  .s_axi_rresp           (sh_ocl_bus_o_cast.rresp             ), // output wire [1 : 0] s_axi_rresp
  .s_axi_rvalid          (sh_ocl_bus_o_cast.rvalid            ), // output wire s_axi_rvalid
  .s_axi_rready          (sh_ocl_bus_i_cast.rready            ), // input wire s_axi_rready
  .s_axi4_awid           (4'h0                         ), // input wire [3 : 0] s_axi4_awid
  .s_axi4_awaddr         (sh_pcis_bus_mosi_r.awaddr[31:0]), // input wire [31 : 0] s_axi4_awaddr
  .s_axi4_awlen          (sh_pcis_bus_mosi_r.awlen       ), // input wire [7 : 0] s_axi4_awlen
  .s_axi4_awsize         (sh_pcis_bus_mosi_r.awsize      ), // input wire [2 : 0] s_axi4_awsize
  .s_axi4_awburst        (2'h0                         ), // input wire [1 : 0] s_axi4_awburst
  .s_axi4_awlock         (1'h0                         ), // input wire s_axi4_awlock
  .s_axi4_awcache        (4'h0                         ), // input wire [3 : 0] s_axi4_awcache
  .s_axi4_awprot         (3'h0                         ), // input wire [2 : 0] s_axi4_awprot
  .s_axi4_awvalid        (sh_pcis_bus_mosi_r.awvalid     ), // input wire s_axi4_awvalid
  .s_axi4_awready        (sh_pcis_bus_miso_r.awready     ), // output wire s_axi4_awready
  .s_axi4_wdata          (sh_pcis_bus_mosi_r.wdata       ), // input wire [511 : 0] s_axi4_wdata
  .s_axi4_wstrb          (sh_pcis_bus_mosi_r.wstrb       ), // input wire [63 : 0] s_axi4_wstrb
  .s_axi4_wlast          (sh_pcis_bus_mosi_r.wlast       ), // input wire s_axi4_wlast
  .s_axi4_wvalid         (sh_pcis_bus_mosi_r.wvalid      ), // input wire s_axi4_wvalid
  .s_axi4_wready         (sh_pcis_bus_miso_r.wready      ), // output wire s_axi4_wready
  .s_axi4_bid            (                             ), // output wire [3 : 0] s_axi4_bid
  .s_axi4_bresp          (sh_pcis_bus_miso_r.bresp       ), // output wire [1 : 0] s_axi4_bresp
  .s_axi4_bvalid         (sh_pcis_bus_miso_r.bvalid      ), // output wire s_axi4_bvalid
  .s_axi4_bready         (sh_pcis_bus_mosi_r.bready      ), // input wire s_axi4_bready
  .s_axi4_arid           (4'h0                         ), // input wire [3 : 0] s_axi4_arid
  .s_axi4_araddr         (sh_pcis_bus_mosi_r.araddr[31:0]), // input wire [31 : 0] s_axi4_araddr
  .s_axi4_arlen          (sh_pcis_bus_mosi_r.arlen       ), // input wire [7 : 0] s_axi4_arlen
  .s_axi4_arsize         (sh_pcis_bus_mosi_r.arsize      ), // input wire [2 : 0] s_axi4_arsize
  .s_axi4_arburst        (2'h0                         ), // input wire [1 : 0] s_axi4_arburst
  .s_axi4_arlock         (1'h0                         ), // input wire s_axi4_arlock
  .s_axi4_arcache        (4'h0                         ), // input wire [3 : 0] s_axi4_arcache
  .s_axi4_arprot         (3'h0                         ), // input wire [2 : 0] s_axi4_arprot
  .s_axi4_arvalid        (sh_pcis_bus_mosi_r.arvalid     ), // input wire s_axi4_arvalid
  .s_axi4_arready        (sh_pcis_bus_miso_r.arready     ), // output wire s_axi4_arready
  .s_axi4_rid            (                             ), // output wire [3 : 0] s_axi4_rid
  .s_axi4_rdata          (sh_pcis_bus_miso_r.rdata       ), // output wire [511 : 0] s_axi4_rdata
  .s_axi4_rresp          (sh_pcis_bus_miso_r.rresp       ), // output wire [1 : 0] s_axi4_rresp
  .s_axi4_rlast          (sh_pcis_bus_miso_r.rlast       ), // output wire s_axi4_rlast
  .s_axi4_rvalid         (sh_pcis_bus_miso_r.rvalid      ), // output wire s_axi4_rvalid
  .s_axi4_rready         (sh_pcis_bus_mosi_r.rready      ), // input wire s_axi4_rready
  .mm2s_prmry_reset_out_n(                             ), // output wire mm2s_prmry_reset_out_n
  .axi_str_txd_tvalid    (mosi_axisx512_bus.txd_tvalid     ), // output wire axi_str_txd_tvalid
  .axi_str_txd_tready    (miso_axisx512_bus.txd_tready     ), // input wire axi_str_txd_tready
  .axi_str_txd_tlast     (mosi_axisx512_bus.txd_tlast      ), // output wire axi_str_txd_tlast
  .axi_str_txd_tkeep     (mosi_axisx512_bus.txd_tkeep      ), // output wire [63 : 0] axi_str_txd_tkeep
  .axi_str_txd_tdata     (mosi_axisx512_bus.txd_tdata      ), // output wire [511 : 0] axi_str_txd_tdata
  .axi_str_txd_tstrb     (                             ), // output wire [63 : 0] axi_str_txd_tstrb
  .mm2s_cntrl_reset_out_n(                             ), // output wire mm2s_cntrl_reset_out_n
  .axi_str_txc_tvalid    (                             ), // output wire axi_str_txc_tvalid
  .axi_str_txc_tready    (1'h0                         ), // input wire axi_str_txc_tready
  .axi_str_txc_tlast     (                             ), // output wire axi_str_txc_tlast
  .axi_str_txc_tkeep     (                             ), // output wire [63 : 0] axi_str_txc_tkeep
  .axi_str_txc_tdata     (                             ), // output wire [511 : 0] axi_str_txc_tdata
  .axi_str_txc_tstrb     (                             ), // output wire [63 : 0] axi_str_txc_tstrb
  .s2mm_prmry_reset_out_n(                             ), // output wire s2mm_prmry_reset_out_n
  .axi_str_rxd_tvalid    (miso_axisx512_bus.rxd_tvalid     ), // input wire axi_str_rxd_tvalid
  .axi_str_rxd_tready    (mosi_axisx512_bus.rxd_tready     ), // output wire axi_str_rxd_tready
  .axi_str_rxd_tlast     (miso_axisx512_bus.rxd_tlast      ), // input wire axi_str_rxd_tlast
  .axi_str_rxd_tkeep     (miso_axisx512_bus.rxd_tkeep      ), // input wire [63 : 0] axi_str_rxd_tkeep
  .axi_str_rxd_tdata     (miso_axisx512_bus.rxd_tdata      ), // input wire [511 : 0] axi_str_rxd_tdata
  .axi_str_rxd_tstrb     (64'h0                        )  // input wire [63 : 0] axi_str_rxd_tstrb
);




axis_dwidth_converter_v1_1_16_axis_dwidth_converter #(
  .C_FAMILY(fpga_version_p),
  .C_S_AXIS_TDATA_WIDTH(512),
  .C_M_AXIS_TDATA_WIDTH(128),
  .C_AXIS_TID_WIDTH(1),
  .C_AXIS_TDEST_WIDTH(1),
  .C_S_AXIS_TUSER_WIDTH(1),
  .C_M_AXIS_TUSER_WIDTH(1),
  .C_AXIS_SIGNAL_SET('B00000000000000000000000000010011)
) axis_32_128 (
  .aclk(clk_i),
  .aresetn(~reset_i),
  .aclken(1'H1),
  .s_axis_tvalid(mosi_axisx512_bus.txd_tvalid),
  .s_axis_tready(miso_axisx512_bus.txd_tready),
  .s_axis_tdata(mosi_axisx512_bus.txd_tdata),
  .s_axis_tstrb(64'H0),
  .s_axis_tkeep(mosi_axisx512_bus.txd_tkeep),
  .s_axis_tlast(mosi_axisx512_bus.txd_tlast),
  .s_axis_tid(1'H0),
  .s_axis_tdest(1'H0),
  .s_axis_tuser(1'H0),
  .m_axis_tvalid(mosi_axisx128_bus.txd_tvalid),  // ->
  .m_axis_tready(miso_axisx128_bus.txd_tready),  // <-
  .m_axis_tdata(mosi_axisx128_bus.txd_tdata),    // ->
  .m_axis_tstrb(),
  .m_axis_tkeep(mosi_axisx128_bus.txd_tkeep),    // ->
  .m_axis_tlast(mosi_axisx128_bus.txd_tlast),    // ->
  .m_axis_tid(),
  .m_axis_tdest(),
  .m_axis_tuser()
);


assign miso_axisx128_bus.rxd_tvalid = mosi_axisx128_bus.txd_tvalid;
assign miso_axisx128_bus.rxd_tdata  = mosi_axisx128_bus.txd_tdata;
assign miso_axisx128_bus.rxd_tkeep  = mosi_axisx128_bus.txd_tkeep;
assign miso_axisx128_bus.rxd_tlast  = mosi_axisx128_bus.txd_tlast;

assign miso_axisx128_bus.txd_tready = mosi_axisx128_bus.rxd_tready;

// assign s_fsb_v_o = mosi_axisx128_bus.txd_tvalid;
// assign s_fsb_data_o = mosi_axisx128_bus.txd_tdata[79:0];
// assign miso_axisx128_bus.txd_tready = s_fsb_r_i;

// //  ||
// //  \/
// // FSB MODULE
// //  ||
// //  \/

// assign miso_axisx128_bus.rxd_tvalid = m_fsb_v_i;
// assign miso_axisx128_bus.rxd_tdata = {48'h0000_0000_0000, m_fsb_data_i};
// assign miso_axisx128_bus.rxd_tlast = m_fsb_v_i & mosi_axisx128_bus.rxd_tready;

// assign m_fsb_r_o = mosi_axisx128_bus.rxd_tready;


axis_dwidth_converter_v1_1_16_axis_dwidth_converter #(
  .C_FAMILY            (fpga_version_p                      ),
  .C_S_AXIS_TDATA_WIDTH(128                               ),
  .C_M_AXIS_TDATA_WIDTH(512                               ),
  .C_AXIS_TID_WIDTH    (1                                 ),
  .C_AXIS_TDEST_WIDTH  (1                                 ),
  .C_S_AXIS_TUSER_WIDTH(1                                 ),
  .C_M_AXIS_TUSER_WIDTH(1                                 ),
  .C_AXIS_SIGNAL_SET   ('B00000000000000000000000000010011)
) axis_128_32 (
  .aclk         (clk_i                   ),
  .aresetn      (~reset_i                ),
  .aclken       (1'H1                    ),
  .s_axis_tvalid(miso_axisx128_bus.rxd_tvalid ),
  .s_axis_tready(mosi_axisx128_bus.rxd_tready ),
  .s_axis_tdata (miso_axisx128_bus.rxd_tdata  ),
  .s_axis_tstrb (16'H0                ),
  .s_axis_tkeep (miso_axisx128_bus.rxd_tkeep  ),
  .s_axis_tlast (miso_axisx128_bus.rxd_tlast  ),
  .s_axis_tid   (1'H0                    ),
  .s_axis_tdest (1'H0                    ),
  .s_axis_tuser (1'H0                    ),
  .m_axis_tvalid(miso_axisx512_bus.rxd_tvalid),
  .m_axis_tready(mosi_axisx512_bus.rxd_tready),
  .m_axis_tdata (miso_axisx512_bus.rxd_tdata ),
  .m_axis_tstrb (                        ),
  .m_axis_tkeep (miso_axisx512_bus.rxd_tkeep ),
  .m_axis_tlast (miso_axisx512_bus.rxd_tlast ),
  .m_axis_tid   (                        ),
  .m_axis_tdest (                        ),
  .m_axis_tuser (                        )
);

endmodule
