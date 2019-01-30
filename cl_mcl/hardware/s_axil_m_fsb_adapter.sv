/**
 *  s_axil_m_fsb_adapter.sv
 *
 *  axi-lite (SH) <-> cl_bsg (CL)
 */

`include "bsg_axi_bus_pkg.vh"

module s_axil_m_fsb_adapter #(
   fsb_width_p = "inv"
  ,axil_mosi_bus_width_lp = `bsg_axil_mosi_bus_width(1)
  ,axil_miso_bus_width_lp = `bsg_axil_miso_bus_width(1)
)(
  input clk_i
  ,input reset_i
  ,input [axil_mosi_bus_width_lp-1:0] sh_ocl_bus_i
  ,output [axil_miso_bus_width_lp-1:0] sh_ocl_bus_o
  ,input m_fsb_v_i
  ,input [fsb_width_p-1:0] m_fsb_data_i
  ,output m_fsb_r_o
  ,output m_fsb_v_o
  ,output [fsb_width_p-1:0] m_fsb_data_o
  ,input m_fsb_r_i
);

parameter fpga_version_p = "virtexuplus";

`declare_bsg_axil_bus_s(1, bsg_axil_mosi_bus_s, bsg_axil_miso_bus_s);

bsg_axil_mosi_bus_s sh_ocl_bus_i_cast, sh_ocl_bus_mosi_r;
bsg_axil_miso_bus_s sh_ocl_bus_o_cast, sh_ocl_bus_miso_r;
assign sh_ocl_bus_i_cast = sh_ocl_bus_i;
assign sh_ocl_bus_o = sh_ocl_bus_o_cast;

// flop the input OCL bus
//---------------------------------
axi_register_slice_light AXIL_OCL_REG_SLC (
  .aclk         (clk_i                    ),
  .aresetn      (~reset_i                 ),
  .s_axi_awaddr (sh_ocl_bus_i_cast.awaddr ),
  .s_axi_awprot (3'h0                     ),
  .s_axi_awvalid(sh_ocl_bus_i_cast.awvalid),
  .s_axi_awready(sh_ocl_bus_o_cast.awready),
  .s_axi_wdata  (sh_ocl_bus_i_cast.wdata  ),
  .s_axi_wstrb  (sh_ocl_bus_i_cast.wstrb  ),
  .s_axi_wvalid (sh_ocl_bus_i_cast.wvalid ),
  .s_axi_wready (sh_ocl_bus_o_cast.wready ),
  .s_axi_bresp  (sh_ocl_bus_o_cast.bresp  ),
  .s_axi_bvalid (sh_ocl_bus_o_cast.bvalid ),
  .s_axi_bready (sh_ocl_bus_i_cast.bready ),
  .s_axi_araddr (sh_ocl_bus_i_cast.araddr ),
  .s_axi_arvalid(sh_ocl_bus_i_cast.arvalid),
  .s_axi_arready(sh_ocl_bus_o_cast.arready),
  .s_axi_rdata  (sh_ocl_bus_o_cast.rdata  ),
  .s_axi_rresp  (sh_ocl_bus_o_cast.rresp  ),
  .s_axi_rvalid (sh_ocl_bus_o_cast.rvalid ),
  .s_axi_rready (sh_ocl_bus_i_cast.rready ),
  
  .m_axi_awaddr (sh_ocl_bus_mosi_r.awaddr    ),
  .m_axi_awprot (                         ),
  .m_axi_awvalid(sh_ocl_bus_mosi_r.awvalid   ),
  .m_axi_awready(sh_ocl_bus_miso_r.awready   ),
  .m_axi_wdata  (sh_ocl_bus_mosi_r.wdata     ),
  .m_axi_wstrb  (sh_ocl_bus_mosi_r.wstrb     ),
  .m_axi_wvalid (sh_ocl_bus_mosi_r.wvalid    ),
  .m_axi_wready (sh_ocl_bus_miso_r.wready    ),
  .m_axi_bresp  (sh_ocl_bus_miso_r.bresp     ),
  .m_axi_bvalid (sh_ocl_bus_miso_r.bvalid    ),
  .m_axi_bready (sh_ocl_bus_mosi_r.bready    ),
  .m_axi_araddr (sh_ocl_bus_mosi_r.araddr    ),
  .m_axi_arvalid(sh_ocl_bus_mosi_r.arvalid   ),
  .m_axi_arready(sh_ocl_bus_miso_r.arready   ),
  .m_axi_rdata  (sh_ocl_bus_miso_r.rdata     ),
  .m_axi_rresp  (sh_ocl_bus_miso_r.rresp     ),
  .m_axi_rvalid (sh_ocl_bus_miso_r.rvalid    ),
  .m_axi_rready (sh_ocl_bus_mosi_r.rready    )
);


`declare_bsg_axis_bus_s(32, bsg_axisx32_mosi_bus_s, bsg_axisx32_miso_bus_s);
`declare_bsg_axis_bus_s(128, bsg_axisx128_mosi_bus_s, bsg_axisx128_miso_bus_s);

bsg_axisx32_mosi_bus_s mosi_axisx32_bus;
bsg_axisx32_miso_bus_s miso_axisx32_bus;
bsg_axisx128_mosi_bus_s mosi_axisx128_bus;
bsg_axisx128_miso_bus_s miso_axisx128_bus;

// convert axil to axis
//---------------------------------
axi_fifo_mm_s #(
  .C_FAMILY              (fpga_version_p),
  .C_S_AXI_ID_WIDTH      (4           ),
  .C_S_AXI_ADDR_WIDTH    (32          ),
  .C_S_AXI_DATA_WIDTH    (32          ),
  .C_S_AXI4_DATA_WIDTH   (32          ),
  .C_TX_FIFO_DEPTH       (512         ),
  .C_RX_FIFO_DEPTH       (512         ),
  .C_TX_FIFO_PF_THRESHOLD(507         ),
  .C_TX_FIFO_PE_THRESHOLD(2           ),
  .C_RX_FIFO_PF_THRESHOLD(507         ),
  .C_RX_FIFO_PE_THRESHOLD(2           ),
  .C_USE_TX_CUT_THROUGH  (1           ),
  .C_DATA_INTERFACE_TYPE (0           ),
  .C_BASEADDR            (32'h80000000),
  .C_HIGHADDR            (32'h80000FFF),
  .C_AXI4_BASEADDR       (32'h80001000),
  .C_AXI4_HIGHADDR       (32'h80002FFF),
  .C_HAS_AXIS_TID        (0           ),
  .C_HAS_AXIS_TDEST      (0           ),
  .C_HAS_AXIS_TUSER      (0           ),
  .C_HAS_AXIS_TSTRB      (0           ),
  .C_HAS_AXIS_TKEEP      (0           ),
  .C_AXIS_TID_WIDTH      (4           ),
  .C_AXIS_TDEST_WIDTH    (4           ),
  .C_AXIS_TUSER_WIDTH    (4           ),
  .C_USE_RX_CUT_THROUGH  (1           ),
  .C_USE_TX_DATA         (1           ),
  .C_USE_TX_CTRL         (0           ),
  .C_USE_RX_DATA         (1           )
) axi_fifo_mm_s_axi_lite (
  .interrupt             (                         ), // output wire interrupt
  .s_axi_aclk            (clk_i                    ), // input wire s_axi_aclk
  .s_axi_aresetn         (~reset_i                 ), // input wire s_axi_aresetn
  .s_axi_awaddr          (sh_ocl_bus_mosi_r.awaddr ), // input wire [31 : 0] s_axi_awaddr
  .s_axi_awvalid         (sh_ocl_bus_mosi_r.awvalid), // input wire s_axi_awvalid
  .s_axi_awready         (sh_ocl_bus_miso_r.awready), // output wire s_axi_awready
  .s_axi_wdata           (sh_ocl_bus_mosi_r.wdata  ), // input wire [31 : 0] s_axi_wdata
  .s_axi_wstrb           (sh_ocl_bus_mosi_r.wstrb  ), // input wire [3 : 0] s_axi_wstrb
  .s_axi_wvalid          (sh_ocl_bus_mosi_r.wvalid ), // input wire s_axi_wvalid
  .s_axi_wready          (sh_ocl_bus_miso_r.wready ), // output wire s_axi_wready
  .s_axi_bresp           (sh_ocl_bus_miso_r.bresp  ), // output wire [1 : 0] s_axi_bresp
  .s_axi_bvalid          (sh_ocl_bus_miso_r.bvalid ), // output wire s_axi_bvalid
  .s_axi_bready          (sh_ocl_bus_mosi_r.bready ), // input wire s_axi_bready
  .s_axi_araddr          (sh_ocl_bus_mosi_r.araddr ), // input wire [31 : 0] s_axi_araddr
  .s_axi_arvalid         (sh_ocl_bus_mosi_r.arvalid), // input wire s_axi_arvalid
  .s_axi_arready         (sh_ocl_bus_miso_r.arready), // output wire s_axi_arready
  .s_axi_rdata           (sh_ocl_bus_miso_r.rdata  ), // output wire [31 : 0] s_axi_rdata
  .s_axi_rresp           (sh_ocl_bus_miso_r.rresp  ), // output wire [1 : 0] s_axi_rresp
  .s_axi_rvalid          (sh_ocl_bus_miso_r.rvalid ), // output wire s_axi_rvalid
  .s_axi_rready          (sh_ocl_bus_mosi_r.rready ), // input wire s_axi_rready
  .s_axi4_awid           (4'h0                     ),
  .s_axi4_awaddr         (32'h0                    ),
  .s_axi4_awlen          (8'h0                     ),
  .s_axi4_awsize         (3'h0                     ),
  .s_axi4_awburst        (2'h0                     ),
  .s_axi4_awlock         (1'h0                     ),
  .s_axi4_awcache        (4'h0                     ),
  .s_axi4_awprot         (3'h0                     ),
  .s_axi4_awvalid        (1'h0                     ),
  .s_axi4_wdata          (32'h0                    ),
  .s_axi4_wstrb          (4'h0                     ),
  .s_axi4_wlast          (1'h0                     ),
  .s_axi4_wvalid         (1'h0                     ),
  .s_axi4_bready         (1'h0                     ),
  .s_axi4_arid           (4'h0                     ),
  .s_axi4_araddr         (32'h0                    ),
  .s_axi4_arlen          (8'h0                     ),
  .s_axi4_arsize         (3'h0                     ),
  .s_axi4_arburst        (2'h0                     ),
  .s_axi4_arlock         (1'h0                     ),
  .s_axi4_arcache        (4'h0                     ),
  .s_axi4_arprot         (3'h0                     ),
  .s_axi4_arvalid        (1'h0                     ),
  .s_axi4_rready         (1'h0                     ),
  .mm2s_prmry_reset_out_n(                         ), // output wire mm2s_prmry_reset_out_n
  .axi_str_txd_tvalid    (mosi_axisx32_bus.txd_tvalid ), // output wire axi_str_txd_tvalid
  .axi_str_txd_tready    (miso_axisx32_bus.txd_tready ), // input wire axi_str_txd_tready
  .axi_str_txd_tlast     (mosi_axisx32_bus.txd_tlast  ), // output wire axi_str_txd_tlast
  .axi_str_txd_tdata     (mosi_axisx32_bus.txd_tdata  ), // output wire [31 : 0] axi_str_txd_tdata
  .axi_str_txc_tready    (1'h0                     ),
  .s2mm_prmry_reset_out_n(                         ), // output wire s2mm_prmry_reset_out_n
  .axi_str_rxd_tvalid    (miso_axisx32_bus.rxd_tvalid ), // input wire axi_str_rxd_tvalid
  .axi_str_rxd_tready    (mosi_axisx32_bus.rxd_tready ), // output wire axi_str_rxd_tready
  .axi_str_rxd_tlast     (miso_axisx32_bus.rxd_tlast  ), // input wire axi_str_rxd_tlast
  .axi_str_rxd_tkeep     (4'h0                     ),
  .axi_str_rxd_tdata     (miso_axisx32_bus.rxd_tdata  ), // input wire [31 : 0] axi_str_rxd_tdata
  .axi_str_rxd_tstrb     (4'h0                     ),
  .axi_str_rxd_tdest     (4'h0                     ),
  .axi_str_rxd_tid       (4'h0                     ),
  .axi_str_rxd_tuser     (4'h0                     )
);

   ila_0 CL_ILA_0 (
                   .clk    (clk_i),
                   .probe0 (mosi_axisx32_bus.txd_tvalid),
                   .probe1 (0),
                   .probe2 (miso_axisx32_bus.txd_tready),
                   .probe3 (mosi_axisx32_bus.txd_tlast),
                   .probe4 (0),
                   .probe5 (miso_axisx128_bus.txd_tready)
                   );


axis_dwidth_converter_v1_1_16_axis_dwidth_converter #(
  .C_FAMILY(fpga_version_p),
  .C_S_AXIS_TDATA_WIDTH(32),
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
  .s_axis_tvalid(mosi_axisx32_bus.txd_tvalid),
  .s_axis_tready(miso_axisx32_bus.txd_tready),
  .s_axis_tdata(mosi_axisx32_bus.txd_tdata),
  .s_axis_tstrb(4'HF),
  .s_axis_tkeep(4'HF),
  .s_axis_tlast(mosi_axisx32_bus.txd_tlast),
  .s_axis_tid(1'H0),
  .s_axis_tdest(1'H0),
  .s_axis_tuser(1'H0),
  .m_axis_tvalid(mosi_axisx128_bus.txd_tvalid),  // ->
  .m_axis_tready(miso_axisx128_bus.txd_tready),  // <-
  .m_axis_tdata(mosi_axisx128_bus.txd_tdata),    // ->
  .m_axis_tstrb(),
  .m_axis_tkeep(mosi_axisx128_bus.txd_tkeep),    // ->
  .m_axis_tlast(mosi_axisx128_bus.txd_tlast),    // -> not used
  .m_axis_tid(),
  .m_axis_tdest(),
  .m_axis_tuser()
);

assign m_fsb_v_o = mosi_axisx128_bus.txd_tvalid;
assign m_fsb_data_o = mosi_axisx128_bus.txd_tdata[fsb_width_p-1:0];
assign miso_axisx128_bus.txd_tready = m_fsb_r_i;

//  ||
//  \/
// FSB MODULE
//  ||
//  \/

assign miso_axisx128_bus.rxd_tvalid = m_fsb_v_i;
assign miso_axisx128_bus.rxd_tdata = {48'h0000_0000_0000, m_fsb_data_i};
assign miso_axisx128_bus.rxd_tlast = m_fsb_v_i & mosi_axisx128_bus.rxd_tready;

assign m_fsb_r_o = mosi_axisx128_bus.rxd_tready & m_fsb_v_i;


axis_dwidth_converter_v1_1_16_axis_dwidth_converter #(
  .C_FAMILY(fpga_version_p),
  .C_S_AXIS_TDATA_WIDTH(128),
  .C_M_AXIS_TDATA_WIDTH(32),
  .C_AXIS_TID_WIDTH(1),
  .C_AXIS_TDEST_WIDTH(1),
  .C_S_AXIS_TUSER_WIDTH(1),
  .C_M_AXIS_TUSER_WIDTH(1),
  .C_AXIS_SIGNAL_SET('B00000000000000000000000000010011)
) axis_128_32 (
  .aclk(clk_i),
  .aresetn(~reset_i),
  .aclken(1'H1),
  .s_axis_tvalid(miso_axisx128_bus.rxd_tvalid),
  .s_axis_tready(mosi_axisx128_bus.rxd_tready),
  .s_axis_tdata(miso_axisx128_bus.rxd_tdata),
  .s_axis_tstrb(16'HFFFF),
  .s_axis_tkeep(16'HFFFF),
  .s_axis_tlast(miso_axisx128_bus.rxd_tlast),
  .s_axis_tid(1'H0),
  .s_axis_tdest(1'H0),
  .s_axis_tuser(1'H0),
  .m_axis_tvalid(miso_axisx32_bus.rxd_tvalid),
  .m_axis_tready(mosi_axisx32_bus.rxd_tready),
  .m_axis_tdata(miso_axisx32_bus.rxd_tdata),
  .m_axis_tstrb(),
  .m_axis_tkeep(),
  .m_axis_tlast(miso_axisx32_bus.rxd_tlast),
  .m_axis_tid(),
  .m_axis_tdest(),
  .m_axis_tuser()
);

endmodule
