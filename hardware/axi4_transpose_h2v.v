/**
*  axi4_transpose_h2v.v
*
*/

`include "bsg_axi_bus_pkg.vh"

module axi4_transpose_h2v #(
  parameter num_axi4_p = "inv"
  , parameter axi4_id_width_p = "inv"
  , parameter axi4_addr_width_p = "inv"
  , parameter axi4_data_width_p = "inv"
  , localparam axi4_mosi_bus_width_lp = `bsg_axi4_mosi_bus_width(1, axi4_id_width_p, axi4_addr_width_p, axi4_data_width_p)
  , localparam axi4_miso_bus_width_lp = `bsg_axi4_miso_bus_width(1, axi4_id_width_p, axi4_addr_width_p, axi4_data_width_p)
) (
  input  [                       num_axi4_p-1:0][axi4_mosi_bus_width_lp-1:0] s_axi4_hs_i
  ,output [                       num_axi4_p-1:0][axi4_miso_bus_width_lp-1:0] s_axi4_hs_o
  ,output [num_axi4_p*axi4_mosi_bus_width_lp-1:0]                             m_axi4_vs_o
  ,input  [num_axi4_p*axi4_miso_bus_width_lp-1:0]                             m_axi4_vs_i
);

  `declare_bsg_axi4_bus_s(1, axi4_id_width_p, axi4_addr_width_p, axi4_data_width_p, bsg_axi4_mosi_horizontal_s, bsg_axi4_miso_horizontal_s);
  bsg_axi4_mosi_horizontal_s [num_axi4_p-1:0] s_axi4_h_li_cast;
  bsg_axi4_miso_horizontal_s [num_axi4_p-1:0] s_axi4_h_lo_cast;

  assign s_axi4_h_li_cast = s_axi4_hs_i;
  assign s_axi4_hs_o = s_axi4_h_lo_cast;

  `declare_bsg_axi4_bus_s(num_axi4_p, axi4_id_width_p, axi4_addr_width_p, axi4_data_width_p, bsg_axi4_mosi_vertical_s, bsg_axi4_miso_vertical_s);
  bsg_axi4_mosi_vertical_s m_axi4_v_lo_cast;
  bsg_axi4_miso_vertical_s m_axi4_v_li_cast;

  assign m_axi4_vs_o = m_axi4_v_lo_cast;
  assign m_axi4_v_li_cast = m_axi4_vs_i;

  for (genvar i = 0; i < num_axi4_p; i++) begin : axi4_transpose_h2v
    assign {
      m_axi4_v_lo_cast.awid[i]
      ,m_axi4_v_lo_cast.awaddr[i]
      ,m_axi4_v_lo_cast.awlen[i]
      ,m_axi4_v_lo_cast.awsize[i]
      ,m_axi4_v_lo_cast.awburst[i]
      ,m_axi4_v_lo_cast.awlock[i]
      ,m_axi4_v_lo_cast.awcache[i]
      ,m_axi4_v_lo_cast.awprot[i]
      ,m_axi4_v_lo_cast.awqos[i]
      ,m_axi4_v_lo_cast.awregion[i]
      ,m_axi4_v_lo_cast.awvalid[i]

      ,m_axi4_v_lo_cast.wdata[i]
      ,m_axi4_v_lo_cast.wstrb[i]
      ,m_axi4_v_lo_cast.wlast[i]
      ,m_axi4_v_lo_cast.wvalid[i]

      ,m_axi4_v_lo_cast.bready[i]

      ,m_axi4_v_lo_cast.arid[i]
      ,m_axi4_v_lo_cast.araddr[i]
      ,m_axi4_v_lo_cast.arlen[i]
      ,m_axi4_v_lo_cast.arsize[i]
      ,m_axi4_v_lo_cast.arburst[i]
      ,m_axi4_v_lo_cast.arlock[i]
      ,m_axi4_v_lo_cast.arcache[i]
      ,m_axi4_v_lo_cast.arprot[i]
      ,m_axi4_v_lo_cast.arqos[i]
      ,m_axi4_v_lo_cast.arregion[i]
      ,m_axi4_v_lo_cast.arvalid[i]

      ,m_axi4_v_lo_cast.rready[i]
    } = s_axi4_h_li_cast[i];

    assign s_axi4_h_lo_cast[i] =
    {
      m_axi4_v_li_cast.awready[i]
      ,m_axi4_v_li_cast.wready[i]

      ,m_axi4_v_li_cast.bid[i]
      ,m_axi4_v_li_cast.bresp[i]
      ,m_axi4_v_li_cast.bvalid[i]

      ,m_axi4_v_li_cast.arready[i]

      ,m_axi4_v_li_cast.rid[i]
      ,m_axi4_v_li_cast.rdata[i]
      ,m_axi4_v_li_cast.rresp[i]
      ,m_axi4_v_li_cast.rlast[i]
      ,m_axi4_v_li_cast.rvalid[i]
    };
  end

endmodule
