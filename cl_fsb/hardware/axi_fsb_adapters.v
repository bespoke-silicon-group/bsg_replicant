/**
 *  axi_fsb_adapters.v
 *
 *   master -> slave
 *  1. axil -> fsb
 *  2. axi4 -> fsb
 *  3. fsb -> axi4
 */

`include "bsg_axi_bus_pkg.vh"

module axi_fsb_adapters #(
  fsb_width_p = "inv"
  ,axi_id_width_p = "inv"
  ,axi_addr_width_p = "inv"
  ,axi_data_width_p = "inv"

  ,axil_mosi_bus_width_lp = `bsg_axil_mosi_bus_width(1)
  ,axil_miso_bus_width_lp = `bsg_axil_miso_bus_width(1)

  ,axi_mosi_bus_width_lp = `bsg_axi_mosi_bus_width(1, axi_id_width_p, axi_addr_width_p, axi_data_width_p)
  ,axi_miso_bus_width_lp = `bsg_axi_miso_bus_width(1, axi_id_width_p, axi_addr_width_p, axi_data_width_p)
)(
    input clk_i
    ,input reset_i

  // axil -> fsb
  ,input [axil_mosi_bus_width_lp-1:0] s0_axil_bus_i
  ,output [axil_miso_bus_width_lp-1:0] s0_axil_bus_o
  ,input  [axil_mosi_bus_width_lp-1:0] s1_axil_bus_i
  ,output [axil_miso_bus_width_lp-1:0] s1_axil_bus_o
  ,input  [axil_mosi_bus_width_lp-1:0] s2_axil_bus_i
  ,output [axil_miso_bus_width_lp-1:0] s2_axil_bus_o

  ,cfg_bus_t.master ocl_cfg_bus_0

  // axi4 pcis -> fsb
  ,input  [ axi_mosi_bus_width_lp-1:0] s1_axi_bus_i
  ,output [ axi_miso_bus_width_lp-1:0] s1_axi_bus_o

  // from fsb slave 0
  ,input m0_fsb_v_i
  ,input [fsb_width_p-1:0] m0_fsb_data_i
  ,output m0_fsb_yumi_o
  // to fsb slave
  ,output m0_fsb_v_o
  ,output [fsb_width_p-1:0] m0_fsb_data_o
  ,input m0_fsb_ready_i

  // from fsb slave 1
  ,input m1_fsb_v_i
  ,input [fsb_width_p-1:0] m1_fsb_data_i
  ,output m1_fsb_yumi_o
  // to fsb slave
  ,output m1_fsb_v_o
  ,output [fsb_width_p-1:0] m1_fsb_data_o
  ,input m1_fsb_ready_i


  // fsb -> axi4 pcim
  ,input  [axi_miso_bus_width_lp-1:0] m_axi_bus_i
  ,output [axi_mosi_bus_width_lp-1:0] m_axi_bus_o

  // from fsb master
  ,input s_fsb_v_i
  ,input [fsb_width_p-1:0] s_fsb_data_i
  ,output s_fsb_yumi_o

);

s_axil_m_fsb_adapter #(.fsb_width_p(fsb_width_p)) s_axil_to_m_fsb (
  .clk_i       (clk_i               ),
  .reset_i    (reset_i),
  .sh_ocl_bus_i(s0_axil_bus_i ),
  .sh_ocl_bus_o(s0_axil_bus_o ),
  .m_fsb_v_i   (m0_fsb_v_i         ),
  .m_fsb_data_i(m0_fsb_data_i      ),
  .m_fsb_r_o   (m0_fsb_yumi_o         ),
  .m_fsb_v_o   (m0_fsb_v_o         ),
  .m_fsb_data_o(m0_fsb_data_o      ),
  .m_fsb_r_i   (m0_fsb_ready_i         )
);

// Simply loop back 4x128bits without FSB client.
// TODO: AXI4-512bit bus should be able to write single FSB packet (128bit,80bit).
s_axi4_m_fsb_adapter s_axi4_to_m_fsb (
  .clk_i        (clk_i                ),
  .reset_i     (reset_i ),
  .sh_ocl_bus_i (s1_axil_bus_i  ),
  .sh_ocl_bus_o (s1_axil_bus_o  ),
  .sh_pcis_bus_i(s1_axi_bus_i   ),
  .sh_pcis_bus_o(s1_axi_bus_o   )
);

m_axi4_fsb_adapter #(
  .axi4_width_p(512        ),
  .fsb_width_p (fsb_width_p)
) m_axi4_fsb (
  .clk_i      (clk_i        ),
  .reset_i    (reset_i      ),
  .cfg_bus    (ocl_cfg_bus_0),
  .m_axi_bus_i(m_axi_bus_i  ),
  .m_axi_bus_o(m_axi_bus_o  ),
  .atg_dst_sel(             ),
  .fsb_wvalid (s_fsb_v_i    ),
  .fsb_wdata  (s_fsb_data_i ),
  .fsb_yumi   (s_fsb_yumi_o )
);



endmodule