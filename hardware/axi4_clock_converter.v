/**
*  axi4_clock_converter.v
*
*/
`include "bsg_axi_bus_pkg.vh"

module axi4_clock_converter #(
  parameter device_family = "inv"
  , parameter s_axi_aclk_ratio_p = "inv"
  , parameter m_axi_aclk_ratio_p = "inv"
  , parameter is_aclk_async_p = "inv"
  , parameter axi4_id_width_p = "inv"
  , parameter axi4_addr_width_p = "inv"
  , parameter axi4_data_width_p = "inv"
  , localparam axi4_mosi_bus_width_lp = `bsg_axi4_mosi_bus_width(1, axi4_id_width_p, axi4_addr_width_p, axi4_data_width_p)
  , localparam axi4_miso_bus_width_lp = `bsg_axi4_miso_bus_width(1, axi4_id_width_p, axi4_addr_width_p, axi4_data_width_p)
) (
  input                               clk_src_i
  ,input                               reset_src_i
  ,input                               clk_dst_i
  ,input                               reset_dst_i
  ,input  [axi4_mosi_bus_width_lp-1:0] s_axi4_src_i
  ,output [axi4_miso_bus_width_lp-1:0] s_axi4_src_o
  ,output [axi4_mosi_bus_width_lp-1:0] m_axi4_dst_o
  ,input  [axi4_miso_bus_width_lp-1:0] m_axi4_dst_i
);

  `declare_bsg_axi4_bus_s(1, axi4_id_width_p, axi4_addr_width_p, axi4_data_width_p, bsg_axi4_mosi_bus_s, bsg_axi4_miso_bus_s);
  bsg_axi4_mosi_bus_s s_axi4_li_cast, m_axi4_lo_cast;
  bsg_axi4_miso_bus_s s_axi4_lo_cast, m_axi4_li_cast;

  assign s_axi4_li_cast = s_axi4_src_i;
  assign s_axi4_src_o = s_axi4_lo_cast;

  assign m_axi4_dst_o = m_axi4_lo_cast;
  assign m_axi4_li_cast = m_axi4_dst_i;


`ifndef USE_IP_GEN
axi_clock_converter_v2_1_18_axi_clock_converter #(
    .C_FAMILY                   (device_family     ),
    .C_AXI_ID_WIDTH             (axi4_id_width_p        ),
    .C_AXI_ADDR_WIDTH           (axi4_addr_width_p      ),
    .C_AXI_DATA_WIDTH           (axi4_data_width_p      ),
    .C_S_AXI_ACLK_RATIO         (s_axi_aclk_ratio_p),
    .C_M_AXI_ACLK_RATIO         (m_axi_aclk_ratio_p),
    .C_AXI_IS_ACLK_ASYNC        (is_aclk_async_p   ),
    .C_AXI_PROTOCOL             (0                 ),
    .C_AXI_SUPPORTS_USER_SIGNALS(0                 ),
    .C_AXI_AWUSER_WIDTH         (1                 ),
    .C_AXI_ARUSER_WIDTH         (1                 ),
    .C_AXI_WUSER_WIDTH          (1                 ),
    .C_AXI_RUSER_WIDTH          (1                 ),
    .C_AXI_BUSER_WIDTH          (1                 ),
    .C_AXI_SUPPORTS_WRITE       (1                 ),
    .C_AXI_SUPPORTS_READ        (1                 ),
    .C_SYNCHRONIZER_STAGE       (3                 )
) inst
`else
axi_clock_converter_0 inst_ip
`endif
    (
    .s_axi_aclk    (clk_src_i              ),
    .s_axi_aresetn (~reset_src_i           ),
`ifndef USE_IP_GEN
    .s_axi_awid    ('0                     ),
`endif
    .s_axi_awaddr  (s_axi4_li_cast.awaddr  ),
    .s_axi_awlen   (s_axi4_li_cast.awlen   ),
    .s_axi_awsize  (s_axi4_li_cast.awsize  ),
    .s_axi_awburst (s_axi4_li_cast.awburst ),
    .s_axi_awlock  (s_axi4_li_cast.awlock  ),
    .s_axi_awcache (s_axi4_li_cast.awcache ),
    .s_axi_awprot  (s_axi4_li_cast.awprot  ),
    .s_axi_awregion(s_axi4_li_cast.awregion),
    .s_axi_awqos   (s_axi4_li_cast.awqos   ),
`ifndef USE_IP_GEN
    .s_axi_awuser  ('0                     ),
`endif
    .s_axi_awvalid (s_axi4_li_cast.awvalid ),
    .s_axi_awready (s_axi4_lo_cast.awready ),

`ifndef USE_IP_GEN
    .s_axi_wid     ('0                     ),
`endif
    .s_axi_wdata   (s_axi4_li_cast.wdata   ),
    .s_axi_wstrb   (s_axi4_li_cast.wstrb   ),
    .s_axi_wlast   (s_axi4_li_cast.wlast   ),
`ifndef USE_IP_GEN
    .s_axi_wuser   ('0                     ),
`endif
    .s_axi_wvalid  (s_axi4_li_cast.wvalid  ),
    .s_axi_wready  (s_axi4_lo_cast.wready  ),

    .s_axi_bid     (s_axi4_lo_cast.bid     ),
    .s_axi_bresp   (s_axi4_lo_cast.bresp   ),
`ifndef USE_IP_GEN
    .s_axi_buser   (                       ),
`endif
    .s_axi_bvalid  (s_axi4_lo_cast.bvalid  ),
    .s_axi_bready  (s_axi4_li_cast.bready  ),

    .s_axi_arid    (s_axi4_li_cast.arid    ),
    .s_axi_araddr  (s_axi4_li_cast.araddr  ),
    .s_axi_arlen   (s_axi4_li_cast.arlen   ),
    .s_axi_arsize  (s_axi4_li_cast.arsize  ),
    .s_axi_arburst (s_axi4_li_cast.arburst ),
    .s_axi_arlock  (s_axi4_li_cast.arlock  ),
    .s_axi_arcache (s_axi4_li_cast.arcache ),
    .s_axi_arprot  (s_axi4_li_cast.arprot  ),
    .s_axi_arregion(s_axi4_li_cast.arregion),
    .s_axi_arqos   (s_axi4_li_cast.arqos   ),
`ifndef USE_IP_GEN
    .s_axi_aruser  ('0                     ),
`endif
    .s_axi_arvalid (s_axi4_li_cast.arvalid ),
    .s_axi_arready (s_axi4_lo_cast.arready ),

    .s_axi_rid     (s_axi4_lo_cast.rid     ),
    .s_axi_rdata   (s_axi4_lo_cast.rdata   ),
    .s_axi_rresp   (s_axi4_lo_cast.rresp   ),
    .s_axi_rlast   (s_axi4_lo_cast.rlast   ),
`ifndef USE_IP_GEN
    .s_axi_ruser   (                       ),
`endif
    .s_axi_rvalid  (s_axi4_lo_cast.rvalid  ),
    .s_axi_rready  (s_axi4_li_cast.rready  ),

    .m_axi_aclk    (clk_dst_i              ),
    .m_axi_aresetn (~reset_dst_i           ),

    .m_axi_awid    (m_axi4_lo_cast.awid    ), //
    .m_axi_awaddr  (m_axi4_lo_cast.awaddr  ),
    .m_axi_awlen   (m_axi4_lo_cast.awlen   ),
    .m_axi_awsize  (m_axi4_lo_cast.awsize  ),
    .m_axi_awburst (m_axi4_lo_cast.awburst ),
    .m_axi_awlock  (m_axi4_lo_cast.awlock  ),
    .m_axi_awcache (m_axi4_lo_cast.awcache ),
    .m_axi_awprot  (m_axi4_lo_cast.awprot  ),
    .m_axi_awregion(m_axi4_lo_cast.awregion),
    .m_axi_awqos   (m_axi4_lo_cast.awqos   ),
`ifndef USE_IP_GEN
    .m_axi_awuser  (                       ),
`endif
    .m_axi_awvalid (m_axi4_lo_cast.awvalid ),
    .m_axi_awready (m_axi4_li_cast.awready ),
`ifndef USE_IP_GEN
    .m_axi_wid     (                       ), // not defined
`endif
    .m_axi_wdata   (m_axi4_lo_cast.wdata   ),
    .m_axi_wstrb   (m_axi4_lo_cast.wstrb   ),
    .m_axi_wlast   (m_axi4_lo_cast.wlast   ),
`ifndef USE_IP_GEN
    .m_axi_wuser   (                       ),
`endif
    .m_axi_wvalid  (m_axi4_lo_cast.wvalid  ),
    .m_axi_wready  (m_axi4_li_cast.wready  ),

    .m_axi_bid     (m_axi4_li_cast.bid     ),
    .m_axi_bresp   (m_axi4_li_cast.bresp   ),
`ifndef USE_IP_GEN
    .m_axi_buser   (1'H0                   ),
`endif
    .m_axi_bvalid  (m_axi4_li_cast.bvalid  ),
    .m_axi_bready  (m_axi4_lo_cast.bready  ),

    .m_axi_arid    (m_axi4_lo_cast.arid    ), //
    .m_axi_araddr  (m_axi4_lo_cast.araddr  ),
    .m_axi_arlen   (m_axi4_lo_cast.arlen   ),
    .m_axi_arsize  (m_axi4_lo_cast.arsize  ),
    .m_axi_arburst (m_axi4_lo_cast.arburst ),
    .m_axi_arlock  (m_axi4_lo_cast.arlock  ),
    .m_axi_arcache (m_axi4_lo_cast.arcache ),
    .m_axi_arprot  (m_axi4_lo_cast.arprot  ),
    .m_axi_arregion(m_axi4_lo_cast.arregion),
    .m_axi_arqos   (m_axi4_lo_cast.arqos   ),
`ifndef USE_IP_GEN
    .m_axi_aruser  (                       ),
`endif
    .m_axi_arvalid (m_axi4_lo_cast.arvalid ),
    .m_axi_arready (m_axi4_li_cast.arready ),

    .m_axi_rid     (m_axi4_li_cast.rid     ),
    .m_axi_rdata   (m_axi4_li_cast.rdata   ),
    .m_axi_rresp   (m_axi4_li_cast.rresp   ),
    .m_axi_rlast   (m_axi4_li_cast.rlast   ),
`ifndef USE_IP_GEN
    .m_axi_ruser   (1'H0                   ),
`endif
    .m_axi_rvalid  (m_axi4_li_cast.rvalid  ),
    .m_axi_rready  (m_axi4_lo_cast.rready  )
);


endmodule
