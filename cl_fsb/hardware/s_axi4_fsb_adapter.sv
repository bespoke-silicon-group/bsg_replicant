/**
 *  s_axi4_fsb_adapter.sv
 *
 *  axi4 (SH) <-> cl_bsg (CL)
 */

module s_axi4_fsb_adapter (
  input clk_i
  ,input resetn_i
  ,axil_bus_t.master sh_ocl_bus
  ,axi_bus_t.master sh_cl_dma_pcis
  ,input adpt_slave_v
  ,input [79:0] adpt_slave_data
  ,output adpt_slave_r
  ,output adpt_master_v
  ,output [79:0] adpt_master_data
  ,input adpt_master_r
);

parameter FPGA_VERSION = "virtexuplus";

// bus for axi4 flop output
axi_bus_t sh_cl_pcis_bus_q();

// bus for axi_fifo_mm_s axis output
axis_bus_t #(.TDATA_WIDTH(512)) fifo_axis_bus();

// bus for datawidth coverter X4 output and datawidth coverter /4 input
axis_bus_t #(.TDATA_WIDTH(128)) axis_fsb_bus();


// flop the input PCIS_DMA bus
//---------------------------------
axi_register_slice_axi4 AXI4_PCIS_REG_SLC (
  .aclk          (clk_i                   ),
  .aresetn       (resetn_i                ),
  .s_axi_awaddr  (sh_cl_dma_pcis.awaddr   ),
  .s_axi_awlen   (sh_cl_dma_pcis.awlen    ),
  .s_axi_awsize  (sh_cl_dma_pcis.awsize   ),
  .s_axi_awburst (2'h0                    ),
  .s_axi_awlock  (1'h0                    ),
  .s_axi_awcache (4'h0                    ),
  .s_axi_awprot  (3'h0                    ),
  .s_axi_awregion(4'h0                    ),
  .s_axi_awqos   (4'h0                    ),
  .s_axi_awvalid (sh_cl_dma_pcis.awvalid  ),
  .s_axi_awready (sh_cl_dma_pcis.awready  ),
  .s_axi_wdata   (sh_cl_dma_pcis.wdata    ),
  .s_axi_wstrb   (sh_cl_dma_pcis.wstrb    ),
  .s_axi_wlast   (sh_cl_dma_pcis.wlast    ),
  .s_axi_wvalid  (sh_cl_dma_pcis.wvalid   ),
  .s_axi_wready  (sh_cl_dma_pcis.wready   ),
  .s_axi_bresp   (sh_cl_dma_pcis.bresp    ),
  .s_axi_bvalid  (sh_cl_dma_pcis.bvalid   ),
  .s_axi_bready  (sh_cl_dma_pcis.bready   ),
  .s_axi_araddr  (sh_cl_dma_pcis.araddr   ),
  .s_axi_arlen   (sh_cl_dma_pcis.arlen    ),
  .s_axi_arsize  (sh_cl_dma_pcis.arsize   ),
  .s_axi_arburst (2'h0                    ),
  .s_axi_arlock  (1'h0                    ),
  .s_axi_arcache (4'h0                    ),
  .s_axi_arprot  (3'h0                    ),
  .s_axi_arregion(4'h0                    ),
  .s_axi_arqos   (4'h0                    ),
  .s_axi_arvalid (sh_cl_dma_pcis.arvalid  ),
  .s_axi_arready (sh_cl_dma_pcis.arready  ),
  .s_axi_rdata   (sh_cl_dma_pcis.rdata    ),
  .s_axi_rresp   (sh_cl_dma_pcis.rresp    ),
  .s_axi_rlast   (sh_cl_dma_pcis.rlast    ),
  .s_axi_rvalid  (sh_cl_dma_pcis.rvalid   ),
  .s_axi_rready  (sh_cl_dma_pcis.rready   ),
  .m_axi_awaddr  (sh_cl_pcis_bus_q.awaddr ),
  .m_axi_awlen   (sh_cl_pcis_bus_q.awlen  ),
  .m_axi_awsize  (sh_cl_pcis_bus_q.awsize ),
  .m_axi_awburst (                        ),
  .m_axi_awlock  (                        ),
  .m_axi_awcache (                        ),
  .m_axi_awprot  (                        ),
  .m_axi_awregion(                        ),
  .m_axi_awqos   (                        ),
  .m_axi_awvalid (sh_cl_pcis_bus_q.awvalid),
  .m_axi_awready (sh_cl_pcis_bus_q.awready),
  .m_axi_wdata   (sh_cl_pcis_bus_q.wdata  ),
  .m_axi_wstrb   (sh_cl_pcis_bus_q.wstrb  ),
  .m_axi_wlast   (sh_cl_pcis_bus_q.wlast  ),
  .m_axi_wvalid  (sh_cl_pcis_bus_q.wvalid ),
  .m_axi_wready  (sh_cl_pcis_bus_q.wready ),
  .m_axi_bresp   (sh_cl_pcis_bus_q.bresp  ),
  .m_axi_bvalid  (sh_cl_pcis_bus_q.bvalid ),
  .m_axi_bready  (sh_cl_pcis_bus_q.bready ),
  .m_axi_araddr  (sh_cl_pcis_bus_q.araddr ),
  .m_axi_arlen   (sh_cl_pcis_bus_q.arlen  ),
  .m_axi_arsize  (sh_cl_pcis_bus_q.arsize ),
  .m_axi_arburst (                        ),
  .m_axi_arlock  (                        ),
  .m_axi_arcache (                        ),
  .m_axi_arprot  (                        ),
  .m_axi_arregion(                        ),
  .m_axi_arqos   (                        ),
  .m_axi_arvalid (sh_cl_pcis_bus_q.arvalid),
  .m_axi_arready (sh_cl_pcis_bus_q.arready),
  .m_axi_rdata   (sh_cl_pcis_bus_q.rdata  ),
  .m_axi_rresp   (sh_cl_pcis_bus_q.rresp  ),
  .m_axi_rlast   (sh_cl_pcis_bus_q.rlast  ),
  .m_axi_rvalid  (sh_cl_pcis_bus_q.rvalid ),
  .m_axi_rready  (sh_cl_pcis_bus_q.rready )
);

assign sh_cl_pcis_bus_q.awid = 0;
assign sh_cl_pcis_bus_q.bid  = 0;


// convert axi4 to axis
//---------------------------------
axi_fifo_mm_s #(
  .C_FAMILY              (FPGA_VERSION),
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
  .s_axi_aresetn         (resetn_i                     ), // input wire s_axi_aresetn
  .s_axi_awaddr          (sh_ocl_bus.awaddr            ), // input wire [31 : 0] s_axi_awaddr
  .s_axi_awvalid         (sh_ocl_bus.awvalid           ), // input wire s_axi_awvalid
  .s_axi_awready         (sh_ocl_bus.awready           ), // output wire s_axi_awready
  .s_axi_wdata           (sh_ocl_bus.wdata             ), // input wire [31 : 0] s_axi_wdata
  .s_axi_wstrb           (sh_ocl_bus.wstrb             ), // input wire [3 : 0] s_axi_wstrb
  .s_axi_wvalid          (sh_ocl_bus.wvalid            ), // input wire s_axi_wvalid
  .s_axi_wready          (sh_ocl_bus.wready            ), // output wire s_axi_wready
  .s_axi_bresp           (sh_ocl_bus.bresp             ), // output wire [1 : 0] s_axi_bresp
  .s_axi_bvalid          (sh_ocl_bus.bvalid            ), // output wire s_axi_bvalid
  .s_axi_bready          (sh_ocl_bus.bready            ), // input wire s_axi_bready
  .s_axi_araddr          (sh_ocl_bus.araddr            ), // input wire [31 : 0] s_axi_araddr
  .s_axi_arvalid         (sh_ocl_bus.arvalid           ), // input wire s_axi_arvalid
  .s_axi_arready         (sh_ocl_bus.arready           ), // output wire s_axi_arready
  .s_axi_rdata           (sh_ocl_bus.rdata             ), // output wire [31 : 0] s_axi_rdata
  .s_axi_rresp           (sh_ocl_bus.rresp             ), // output wire [1 : 0] s_axi_rresp
  .s_axi_rvalid          (sh_ocl_bus.rvalid            ), // output wire s_axi_rvalid
  .s_axi_rready          (sh_ocl_bus.rready            ), // input wire s_axi_rready
  .s_axi4_awid           (sh_cl_pcis_bus_q.awid[3:0]   ), // input wire [3 : 0] s_axi4_awid
  .s_axi4_awaddr         (sh_cl_pcis_bus_q.awaddr[31:0]), // input wire [31 : 0] s_axi4_awaddr
  .s_axi4_awlen          (sh_cl_pcis_bus_q.awlen       ), // input wire [7 : 0] s_axi4_awlen
  .s_axi4_awsize         (sh_cl_pcis_bus_q.awsize      ), // input wire [2 : 0] s_axi4_awsize
  .s_axi4_awburst        (2'h0                         ), // input wire [1 : 0] s_axi4_awburst
  .s_axi4_awlock         (1'h0                         ), // input wire s_axi4_awlock
  .s_axi4_awcache        (4'h0                         ), // input wire [3 : 0] s_axi4_awcache
  .s_axi4_awprot         (3'h0                         ), // input wire [2 : 0] s_axi4_awprot
  .s_axi4_awvalid        (sh_cl_pcis_bus_q.awvalid     ), // input wire s_axi4_awvalid
  .s_axi4_awready        (sh_cl_pcis_bus_q.awready     ), // output wire s_axi4_awready
  .s_axi4_wdata          (sh_cl_pcis_bus_q.wdata       ), // input wire [511 : 0] s_axi4_wdata
  .s_axi4_wstrb          (sh_cl_pcis_bus_q.wstrb       ), // input wire [63 : 0] s_axi4_wstrb
  .s_axi4_wlast          (sh_cl_pcis_bus_q.wlast       ), // input wire s_axi4_wlast
  .s_axi4_wvalid         (sh_cl_pcis_bus_q.wvalid      ), // input wire s_axi4_wvalid
  .s_axi4_wready         (sh_cl_pcis_bus_q.wready      ), // output wire s_axi4_wready
  .s_axi4_bid            (sh_cl_pcis_bus_q.bid[3:0]    ), // output wire [3 : 0] s_axi4_bid
  .s_axi4_bresp          (sh_cl_pcis_bus_q.bresp       ), // output wire [1 : 0] s_axi4_bresp
  .s_axi4_bvalid         (sh_cl_pcis_bus_q.bvalid      ), // output wire s_axi4_bvalid
  .s_axi4_bready         (sh_cl_pcis_bus_q.bready      ), // input wire s_axi4_bready
  .s_axi4_arid           (sh_cl_pcis_bus_q.arid[3:0]   ), // input wire [3 : 0] s_axi4_arid
  .s_axi4_araddr         (sh_cl_pcis_bus_q.araddr[31:0]), // input wire [31 : 0] s_axi4_araddr
  .s_axi4_arlen          (sh_cl_pcis_bus_q.arlen       ), // input wire [7 : 0] s_axi4_arlen
  .s_axi4_arsize         (sh_cl_pcis_bus_q.arsize      ), // input wire [2 : 0] s_axi4_arsize
  .s_axi4_arburst        (2'h0                         ), // input wire [1 : 0] s_axi4_arburst
  .s_axi4_arlock         (1'h0                         ), // input wire s_axi4_arlock
  .s_axi4_arcache        (4'h0                         ), // input wire [3 : 0] s_axi4_arcache
  .s_axi4_arprot         (3'h0                         ), // input wire [2 : 0] s_axi4_arprot
  .s_axi4_arvalid        (sh_cl_pcis_bus_q.arvalid     ), // input wire s_axi4_arvalid
  .s_axi4_arready        (sh_cl_pcis_bus_q.arready     ), // output wire s_axi4_arready
  .s_axi4_rid            (sh_cl_pcis_bus_q.rid[3:0]    ), // output wire [3 : 0] s_axi4_rid
  .s_axi4_rdata          (sh_cl_pcis_bus_q.rdata       ), // output wire [511 : 0] s_axi4_rdata
  .s_axi4_rresp          (sh_cl_pcis_bus_q.rresp       ), // output wire [1 : 0] s_axi4_rresp
  .s_axi4_rlast          (sh_cl_pcis_bus_q.rlast       ), // output wire s_axi4_rlast
  .s_axi4_rvalid         (sh_cl_pcis_bus_q.rvalid      ), // output wire s_axi4_rvalid
  .s_axi4_rready         (sh_cl_pcis_bus_q.rready      ), // input wire s_axi4_rready
  .mm2s_prmry_reset_out_n(                             ), // output wire mm2s_prmry_reset_out_n
  .axi_str_txd_tvalid    (fifo_axis_bus.txd_tvalid     ), // output wire axi_str_txd_tvalid
  .axi_str_txd_tready    (fifo_axis_bus.txd_tready     ), // input wire axi_str_txd_tready
  .axi_str_txd_tlast     (fifo_axis_bus.txd_tlast      ), // output wire axi_str_txd_tlast
  .axi_str_txd_tkeep     (fifo_axis_bus.txd_tkeep      ), // output wire [63 : 0] axi_str_txd_tkeep
  .axi_str_txd_tdata     (fifo_axis_bus.txd_tdata      ), // output wire [511 : 0] axi_str_txd_tdata
  .axi_str_txd_tstrb     (                             ), // output wire [63 : 0] axi_str_txd_tstrb
  .mm2s_cntrl_reset_out_n(                             ), // output wire mm2s_cntrl_reset_out_n
  .axi_str_txc_tvalid    (                             ), // output wire axi_str_txc_tvalid
  .axi_str_txc_tready    (1'h0                         ), // input wire axi_str_txc_tready
  .axi_str_txc_tlast     (                             ), // output wire axi_str_txc_tlast
  .axi_str_txc_tkeep     (                             ), // output wire [63 : 0] axi_str_txc_tkeep
  .axi_str_txc_tdata     (                             ), // output wire [511 : 0] axi_str_txc_tdata
  .axi_str_txc_tstrb     (                             ), // output wire [63 : 0] axi_str_txc_tstrb
  .s2mm_prmry_reset_out_n(                             ), // output wire s2mm_prmry_reset_out_n
  .axi_str_rxd_tvalid    (fifo_axis_bus.rxd_tvalid     ), // input wire axi_str_rxd_tvalid
  .axi_str_rxd_tready    (fifo_axis_bus.rxd_tready     ), // output wire axi_str_rxd_tready
  .axi_str_rxd_tlast     (fifo_axis_bus.rxd_tlast      ), // input wire axi_str_rxd_tlast
  .axi_str_rxd_tkeep     (fifo_axis_bus.rxd_tkeep      ), // input wire [63 : 0] axi_str_rxd_tkeep
  .axi_str_rxd_tdata     (fifo_axis_bus.rxd_tdata      ), // input wire [511 : 0] axi_str_rxd_tdata
  .axi_str_rxd_tstrb     (64'h0                        )  // input wire [63 : 0] axi_str_rxd_tstrb
);



axis_dwidth_converter_v1_1_16_axis_dwidth_converter #(
  .C_FAMILY(FPGA_VERSION),
  .C_S_AXIS_TDATA_WIDTH(512),
  .C_M_AXIS_TDATA_WIDTH(128),
  .C_AXIS_TID_WIDTH(1),
  .C_AXIS_TDEST_WIDTH(1),
  .C_S_AXIS_TUSER_WIDTH(1),
  .C_M_AXIS_TUSER_WIDTH(1),
  .C_AXIS_SIGNAL_SET('B00000000000000000000000000010011)
) axis_32_128 (
  .aclk(clk_i),
  .aresetn(resetn_i),
  .aclken(1'H1),
  .s_axis_tvalid(fifo_axis_bus.txd_tvalid),
  .s_axis_tready(fifo_axis_bus.txd_tready),
  .s_axis_tdata(fifo_axis_bus.txd_tdata),
  .s_axis_tstrb(16'H0),
  .s_axis_tkeep(fifo_axis_bus.txd_tkeep),
  .s_axis_tlast(fifo_axis_bus.txd_tlast),
  .s_axis_tid(1'H0),
  .s_axis_tdest(1'H0),
  .s_axis_tuser(1'H0),
  .m_axis_tvalid(axis_fsb_bus.rxd_tvalid),  // ->
  .m_axis_tready(axis_fsb_bus.rxd_tready),  // <-
  .m_axis_tdata(axis_fsb_bus.rxd_tdata),    // ->
  .m_axis_tstrb(),
  .m_axis_tkeep(axis_fsb_bus.rxd_tkeep),    // ->
  .m_axis_tlast(axis_fsb_bus.rxd_tlast),    // ->
  .m_axis_tid(),
  .m_axis_tdest(),
  .m_axis_tuser()
);


assign axis_fsb_bus.txd_tvalid = axis_fsb_bus.rxd_tvalid;
assign axis_fsb_bus.txd_tdata  = axis_fsb_bus.rxd_tdata;
assign axis_fsb_bus.rxd_tready = axis_fsb_bus.txd_tready;
assign axis_fsb_bus.txd_tkeep  = axis_fsb_bus.rxd_tkeep;
assign axis_fsb_bus.txd_tlast  = axis_fsb_bus.rxd_tlast;

// assign adpt_master_v = axis_fsb_bus.rxd_tvalid;
// assign adpt_master_data = axis_fsb_bus.rxd_tdata[79:0];
// // assign axis_fsb_bus.rxd_tlast;
// assign axis_fsb_bus.rxd_tready = adpt_master_r;

// //  ||
// //  \/
// // FSB MODULE
// //  ||
// //  \/

// assign axis_fsb_bus.txd_tvalid = adpt_slave_v;
// assign axis_fsb_bus.txd_tdata = {48'h0000_0000_0000, adpt_slave_data};
// assign axis_fsb_bus.txd_tlast = adpt_slave_v & axis_fsb_bus.txd_tready;

// assign adpt_slave_r = axis_fsb_bus.txd_tready;


axis_dwidth_converter_v1_1_16_axis_dwidth_converter #(
  .C_FAMILY            (FPGA_VERSION                      ),
  .C_S_AXIS_TDATA_WIDTH(128                               ),
  .C_M_AXIS_TDATA_WIDTH(512                               ),
  .C_AXIS_TID_WIDTH    (1                                 ),
  .C_AXIS_TDEST_WIDTH  (1                                 ),
  .C_S_AXIS_TUSER_WIDTH(1                                 ),
  .C_M_AXIS_TUSER_WIDTH(1                                 ),
  .C_AXIS_SIGNAL_SET   ('B00000000000000000000000000010011)
) axis_128_32 (
  .aclk         (clk_i                   ),
  .aresetn      (resetn_i                ),
  .aclken       (1'H1                    ),
  .s_axis_tvalid(axis_fsb_bus.txd_tvalid ),
  .s_axis_tready(axis_fsb_bus.txd_tready ),
  .s_axis_tdata (axis_fsb_bus.txd_tdata  ),
  .s_axis_tstrb (16'H0                ),
  .s_axis_tkeep (axis_fsb_bus.txd_tkeep  ),
  .s_axis_tlast (axis_fsb_bus.txd_tlast  ),
  .s_axis_tid   (1'H0                    ),
  .s_axis_tdest (1'H0                    ),
  .s_axis_tuser (1'H0                    ),
  .m_axis_tvalid(fifo_axis_bus.rxd_tvalid),
  .m_axis_tready(fifo_axis_bus.rxd_tready),
  .m_axis_tdata (fifo_axis_bus.rxd_tdata ),
  .m_axis_tstrb (                        ),
  .m_axis_tkeep (fifo_axis_bus.rxd_tkeep ),
  .m_axis_tlast (fifo_axis_bus.rxd_tlast ),
  .m_axis_tid   (                        ),
  .m_axis_tdest (                        ),
  .m_axis_tuser (                        )
);

endmodule
