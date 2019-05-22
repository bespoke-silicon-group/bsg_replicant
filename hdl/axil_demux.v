/**
*  axil_demux.v
*
*/

`include "bsg_axi_bus_pkg.vh"

module axil_demux #(
  parameter slot_num_p = "inv"
  , localparam axil_mosi_bus_width_lp = `bsg_axil_mosi_bus_width(1)
  , localparam axil_miso_bus_width_lp = `bsg_axil_miso_bus_width(1)
) (
  input                                                           clk_i
  ,input                                                           reset_i
  ,input  [axil_mosi_bus_width_lp-1:0]                             s_axil_bus_i
  ,output [axil_miso_bus_width_lp-1:0]                             s_axil_bus_o
  ,output [            slot_num_p-1:0][axil_mosi_bus_width_lp-1:0] m_axil_demux_o
  ,input  [            slot_num_p-1:0][axil_miso_bus_width_lp-1:0] m_axil_demux_i
);

//---------------------------------------------
// Concatenated Signals
//---------------------------------------------
  `declare_bsg_axil_bus_s(1, bsg_s_axil_i_bus_s, bsg_s_axil_o_bus_s);
  bsg_s_axil_i_bus_s s_axil_bus_i_cast;
  bsg_s_axil_o_bus_s s_axil_bus_o_cast;

  assign s_axil_bus_i_cast = s_axil_bus_i;
  assign s_axil_bus_o      = s_axil_bus_o_cast;


  `declare_bsg_axil_bus_s(slot_num_p, bsg_m_axil_buses_o_s, bsg_m_axil_buses_i_s);
  bsg_m_axil_buses_o_s m_axil_buses_lo_cast;
  bsg_m_axil_buses_i_s m_axil_buses_li_cast;

  localparam axil_base_addr_width_lp = 16;

  localparam                     axil_base_addr_width_els = {slot_num_p{32'(axil_base_addr_width_lp)}};
  localparam [slot_num_p*64-1:0] m_axil_base_address_lp   = {64'(1<<axil_base_addr_width_lp), 64'(0)} ;


  axi_crossbar_v2_1_18_axi_crossbar #(
    .C_FAMILY                   ("virtexuplus"              ),
    .C_NUM_SLAVE_SLOTS          (1                          ),
    .C_NUM_MASTER_SLOTS         (slot_num_p                 ),
    .C_AXI_ID_WIDTH             (1                          ),
    .C_AXI_ADDR_WIDTH           (32                         ),
    .C_AXI_DATA_WIDTH           (32                         ),
    .C_AXI_PROTOCOL             (2                          ),
    .C_NUM_ADDR_RANGES          (1                          ),
    .C_M_AXI_BASE_ADDR          (m_axil_base_address_lp     ),
    .C_M_AXI_ADDR_WIDTH         (axil_base_addr_width_els   ),
    .C_S_AXI_BASE_ID            (32'H00000000               ),
    .C_S_AXI_THREAD_ID_WIDTH    (32'H00000000               ),
    .C_AXI_SUPPORTS_USER_SIGNALS(0                          ),
    .C_AXI_AWUSER_WIDTH         (1                          ),
    .C_AXI_ARUSER_WIDTH         (1                          ),
    .C_AXI_WUSER_WIDTH          (1                          ),
    .C_AXI_RUSER_WIDTH          (1                          ),
    .C_AXI_BUSER_WIDTH          (1                          ),
    .C_M_AXI_WRITE_CONNECTIVITY ({slot_num_p{32'h0000_0001}}),
    .C_M_AXI_READ_CONNECTIVITY  ({slot_num_p{32'h0000_0001}}),
    .C_R_REGISTER               (1                          ),
    .C_S_AXI_SINGLE_THREAD      (32'H00000001               ),
    .C_S_AXI_WRITE_ACCEPTANCE   (32'H00000001               ),
    .C_S_AXI_READ_ACCEPTANCE    (32'H00000001               ),
    .C_M_AXI_WRITE_ISSUING      ({slot_num_p{32'h0000_0001}}),
    .C_M_AXI_READ_ISSUING       ({slot_num_p{32'h0000_0001}}),
    .C_S_AXI_ARB_PRIORITY       (32'H00000000               ),
    .C_M_AXI_SECURE             ({slot_num_p{32'h0000_0000}}),
    .C_CONNECTIVITY_MODE        (0                          )
  ) axil_demux (
    .aclk          (clk_i                       ),
    .aresetn       (~reset_i                    ),
    .s_axi_awid    (1'H0                        ),
    .s_axi_awaddr  (s_axil_bus_i_cast.awaddr    ),
    .s_axi_awlen   (8'H00                       ),
    .s_axi_awsize  (3'H0                        ),
    .s_axi_awburst (2'H0                        ),
    .s_axi_awlock  (1'H0                        ),
    .s_axi_awcache (4'H0                        ),
    .s_axi_awprot  (3'H0                        ),
    .s_axi_awqos   (4'H0                        ),
    .s_axi_awuser  (1'H0                        ),
    .s_axi_awvalid (s_axil_bus_i_cast.awvalid   ),
    .s_axi_awready (s_axil_bus_o_cast.awready   ),
    .s_axi_wid     (1'H0                        ),
    .s_axi_wdata   (s_axil_bus_i_cast.wdata     ),
    .s_axi_wstrb   (s_axil_bus_i_cast.wstrb     ),
    .s_axi_wlast   (1'H1                        ),
    .s_axi_wuser   (1'H0                        ),
    .s_axi_wvalid  (s_axil_bus_i_cast.wvalid    ),
    .s_axi_wready  (s_axil_bus_o_cast.wready    ),
    .s_axi_bid     (                            ),
    .s_axi_bresp   (s_axil_bus_o_cast.bresp     ),
    .s_axi_buser   (                            ),
    .s_axi_bvalid  (s_axil_bus_o_cast.bvalid    ),
    .s_axi_bready  (s_axil_bus_i_cast.bready    ),
    .s_axi_arid    (1'H0                        ),
    .s_axi_araddr  (s_axil_bus_i_cast.araddr    ),
    .s_axi_arlen   (8'H00                       ),
    .s_axi_arsize  (3'H0                        ),
    .s_axi_arburst (2'H0                        ),
    .s_axi_arlock  (1'H0                        ),
    .s_axi_arcache (4'H0                        ),
    .s_axi_arprot  (3'H0                        ),
    .s_axi_arqos   (4'H0                        ),
    .s_axi_aruser  (1'H0                        ),
    .s_axi_arvalid (s_axil_bus_i_cast.arvalid   ),
    .s_axi_arready (s_axil_bus_o_cast.arready   ),
    .s_axi_rid     (                            ),
    .s_axi_rdata   (s_axil_bus_o_cast.rdata     ),
    .s_axi_rresp   (s_axil_bus_o_cast.rresp     ),
    .s_axi_rlast   (                            ),
    .s_axi_ruser   (                            ),
    .s_axi_rvalid  (s_axil_bus_o_cast.rvalid    ),
    .s_axi_rready  (s_axil_bus_i_cast.rready    ),
    .m_axi_awid    (                            ),
    .m_axi_awaddr  (m_axil_buses_lo_cast.awaddr ),
    .m_axi_awlen   (                            ),
    .m_axi_awsize  (                            ),
    .m_axi_awburst (                            ),
    .m_axi_awlock  (                            ),
    .m_axi_awcache (                            ),
    .m_axi_awprot  (                            ),
    .m_axi_awregion(                            ),
    .m_axi_awqos   (                            ),
    .m_axi_awuser  (                            ),
    .m_axi_awvalid (m_axil_buses_lo_cast.awvalid),
    .m_axi_awready (m_axil_buses_li_cast.awready),
    .m_axi_wid     (                            ),
    .m_axi_wdata   (m_axil_buses_lo_cast.wdata  ),
    .m_axi_wstrb   (m_axil_buses_lo_cast.wstrb  ),
    .m_axi_wlast   (                            ),
    .m_axi_wuser   (                            ),
    .m_axi_wvalid  (m_axil_buses_lo_cast.wvalid ),
    .m_axi_wready  (m_axil_buses_li_cast.wready ),
    .m_axi_bid     ({slot_num_p{1'b0}}          ),
    .m_axi_bresp   (m_axil_buses_li_cast.bresp  ),
    .m_axi_buser   ({slot_num_p{1'b0}}          ),
    .m_axi_bvalid  (m_axil_buses_li_cast.bvalid ),
    .m_axi_bready  (m_axil_buses_lo_cast.bready ),
    .m_axi_arid    (                            ),
    .m_axi_araddr  (m_axil_buses_lo_cast.araddr ),
    .m_axi_arlen   (                            ),
    .m_axi_arsize  (                            ),
    .m_axi_arburst (                            ),
    .m_axi_arlock  (                            ),
    .m_axi_arcache (                            ),
    .m_axi_arprot  (                            ),
    .m_axi_arregion(                            ),
    .m_axi_arqos   (                            ),
    .m_axi_aruser  (                            ),
    .m_axi_arvalid (m_axil_buses_lo_cast.arvalid),
    .m_axi_arready (m_axil_buses_li_cast.arready),
    .m_axi_rid     ({slot_num_p{1'b0}}          ),
    .m_axi_rdata   (m_axil_buses_li_cast.rdata  ),
    .m_axi_rresp   (m_axil_buses_li_cast.rresp  ),
    .m_axi_rlast   ({slot_num_p{1'b1}}          ),
    .m_axi_ruser   ({slot_num_p{1'b0}}          ),
    .m_axi_rvalid  (m_axil_buses_li_cast.rvalid ),
    .m_axi_rready  (m_axil_buses_lo_cast.rready )
  );


  `declare_bsg_axil_bus_s(1, bsg_m_axil_o_bus_s, bsg_m_axil_i_bus_s);
  bsg_m_axil_o_bus_s [slot_num_p-1:0] m_axil_demux_lo_cast;
  bsg_m_axil_i_bus_s [slot_num_p-1:0] m_axil_demux_li_cast;

  assign m_axil_demux_o       = m_axil_demux_lo_cast;
  assign m_axil_demux_li_cast = m_axil_demux_i;

  for (genvar i=0; i<slot_num_p; i=i+1) begin

    always_comb begin: axi_master_input

      m_axil_buses_li_cast.awready[i] = m_axil_demux_li_cast[i].awready;
      m_axil_buses_li_cast.wready[i] = m_axil_demux_li_cast[i].wready;

      m_axil_buses_li_cast.bresp[i*2+:2] = m_axil_demux_li_cast[i].bresp;
      m_axil_buses_li_cast.bvalid[i] = m_axil_demux_li_cast[i].bvalid;

      m_axil_buses_li_cast.arready[i] = m_axil_demux_li_cast[i].arready;

      m_axil_buses_li_cast.rdata[i*32+:32] = m_axil_demux_li_cast[i].rdata;
      m_axil_buses_li_cast.rresp[i*2+:2] = m_axil_demux_li_cast[i].rresp;
      m_axil_buses_li_cast.rvalid[i] = m_axil_demux_li_cast[i].rvalid;

    end


    always_comb begin: axil_master_output

      m_axil_demux_lo_cast[i].awaddr = m_axil_buses_lo_cast.awaddr[i*32+:32];
      m_axil_demux_lo_cast[i].awvalid = m_axil_buses_lo_cast.awvalid[i];

      m_axil_demux_lo_cast[i].wdata = m_axil_buses_lo_cast.wdata[i*32+:32];
      m_axil_demux_lo_cast[i].wstrb = m_axil_buses_lo_cast.wstrb[i*4+:4];
      m_axil_demux_lo_cast[i].wvalid = m_axil_buses_lo_cast.wvalid[i];

      m_axil_demux_lo_cast[i].bready = m_axil_buses_lo_cast.bready[i];

      m_axil_demux_lo_cast[i].araddr = m_axil_buses_lo_cast.araddr[i*32+:32];
      m_axil_demux_lo_cast[i].arvalid = m_axil_buses_lo_cast.arvalid[i];

      m_axil_demux_lo_cast[i].rready = m_axil_buses_lo_cast.rready[i];

    end
  end

endmodule
