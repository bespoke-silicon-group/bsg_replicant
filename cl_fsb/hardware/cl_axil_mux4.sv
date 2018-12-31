/**
*  cl_axil_mux4.sv
*
*  axil 1 to 4 multiplexer
*/

`include "bsg_axi_bus_pkg.vh"

module cl_axil_mux4 #(
  mst_mosi_bus_width_lp = `bsg_axil_mosi_bus_width(1)
  ,mst_miso_bus_width_lp = `bsg_axil_miso_bus_width(1)
  ,slv_mosi_bus_width_lp = `bsg_axil_mosi_bus_width(1)
  ,slv_miso_bus_width_lp = `bsg_axil_miso_bus_width(1)
) (
  input                              clk_i
  ,input                              reset_i
  ,input  [mst_mosi_bus_width_lp-1:0] mst_bus_i
  ,output [mst_miso_bus_width_lp-1:0] mst_bus_o
  // slave
  ,input  [slv_miso_bus_width_lp-1:0] slv_0_bus_i
  ,output [slv_mosi_bus_width_lp-1:0] slv_0_bus_o
  ,input  [slv_miso_bus_width_lp-1:0] slv_1_bus_i
  ,output [slv_mosi_bus_width_lp-1:0] slv_1_bus_o
  ,input  [slv_miso_bus_width_lp-1:0] slv_2_bus_i
  ,output [slv_mosi_bus_width_lp-1:0] slv_2_bus_o
  ,input  [slv_miso_bus_width_lp-1:0] slv_3_bus_i
  ,output [slv_mosi_bus_width_lp-1:0] slv_3_bus_o
);

  // axil_bus_t #(.NUM_SLOTS(4)) axil_s_mux_bus ();



  `declare_bsg_axil_bus_s(1, bsg_axil_mosi_bus_s, bsg_axil_miso_bus_s);

  bsg_axil_mosi_bus_s mst_bus_i_cast, slv_0_bus_o_cast, slv_1_bus_o_cast, slv_2_bus_o_cast, slv_3_bus_o_cast;
  bsg_axil_miso_bus_s mst_bus_o_cast, slv_0_bus_i_cast, slv_1_bus_i_cast, slv_2_bus_i_cast, slv_3_bus_i_cast;

  assign mst_bus_i_cast = mst_bus_i;
  assign mst_bus_o      = mst_bus_o_cast;

  assign slv_0_bus_i_cast = slv_0_bus_i;
  assign slv_0_bus_o      = slv_0_bus_o_cast;
  assign slv_1_bus_i_cast = slv_1_bus_i;
  assign slv_1_bus_o      = slv_1_bus_o_cast;
  assign slv_2_bus_i_cast = slv_2_bus_i;
  assign slv_2_bus_o      = slv_2_bus_o_cast;
  assign slv_3_bus_i_cast = slv_3_bus_i;
  assign slv_3_bus_o      = slv_3_bus_o_cast;


  `declare_bsg_axil_bus_s(4, bsg_axil_mosi_4bus_s, bsg_axil_miso_4bus_s);

  bsg_axil_mosi_4bus_s slv_mux_o_bus_cast;
  bsg_axil_miso_4bus_s slv_mux_i_bus_cast;

  assign {slv_3_bus_o_cast.awaddr, slv_2_bus_o_cast.awaddr, slv_1_bus_o_cast.awaddr, slv_0_bus_o_cast.awaddr}
    = slv_mux_o_bus_cast.awaddr;
  assign {slv_3_bus_o_cast.awvalid, slv_2_bus_o_cast.awvalid, slv_1_bus_o_cast.awvalid, slv_0_bus_o_cast.awvalid}
    = slv_mux_o_bus_cast.awvalid;
  assign {slv_3_bus_o_cast.wdata, slv_2_bus_o_cast.wdata, slv_1_bus_o_cast.wdata, slv_0_bus_o_cast.wdata}
    = slv_mux_o_bus_cast.wdata;
  assign {slv_3_bus_o_cast.wstrb, slv_2_bus_o_cast.wstrb, slv_1_bus_o_cast.wstrb, slv_0_bus_o_cast.wstrb}
    = slv_mux_o_bus_cast.wstrb;
  assign {slv_3_bus_o_cast.wvalid, slv_2_bus_o_cast.wvalid, slv_1_bus_o_cast.wvalid, slv_0_bus_o_cast.wvalid}
    = slv_mux_o_bus_cast.wvalid;

  assign {slv_3_bus_o_cast.bready, slv_2_bus_o_cast.bready, slv_1_bus_o_cast.bready, slv_0_bus_o_cast.bready}
    = slv_mux_o_bus_cast.bready;

  assign {slv_3_bus_o_cast.araddr, slv_2_bus_o_cast.araddr, slv_1_bus_o_cast.araddr, slv_0_bus_o_cast.araddr}
    = slv_mux_o_bus_cast.araddr;
  assign {slv_3_bus_o_cast.arvalid, slv_2_bus_o_cast.arvalid, slv_1_bus_o_cast.arvalid, slv_0_bus_o_cast.arvalid}
    = slv_mux_o_bus_cast.arvalid;
  assign {slv_3_bus_o_cast.rready, slv_2_bus_o_cast.rready, slv_1_bus_o_cast.rready, slv_0_bus_o_cast.rready}
    = slv_mux_o_bus_cast.rready;


  assign slv_mux_i_bus_cast.awready
    = {slv_3_bus_i_cast.awready, slv_2_bus_i_cast.awready, slv_1_bus_i_cast.awready, slv_0_bus_i_cast.awready};
  assign slv_mux_i_bus_cast.wready
    = {slv_3_bus_i_cast.wready, slv_2_bus_i_cast.wready, slv_1_bus_i_cast.wready, slv_0_bus_i_cast.wready};

  assign slv_mux_i_bus_cast.bresp
    = {slv_3_bus_i_cast.bresp, slv_2_bus_i_cast.bresp, slv_1_bus_i_cast.bresp, slv_0_bus_i_cast.bresp};
  assign slv_mux_i_bus_cast.bvalid
    = {slv_3_bus_i_cast.bvalid, slv_2_bus_i_cast.bvalid, slv_1_bus_i_cast.bvalid, slv_0_bus_i_cast.bvalid};

  assign slv_mux_i_bus_cast.arready
    = {slv_3_bus_i_cast.arready, slv_2_bus_i_cast.arready, slv_1_bus_i_cast.arready, slv_0_bus_i_cast.arready};
  assign slv_mux_i_bus_cast.rdata
    = {slv_3_bus_i_cast.rdata, slv_2_bus_i_cast.rdata, slv_1_bus_i_cast.rdata, slv_0_bus_i_cast.rdata};
  assign slv_mux_i_bus_cast.rresp
    = {slv_3_bus_i_cast.rresp, slv_2_bus_i_cast.rresp, slv_1_bus_i_cast.rresp, slv_0_bus_i_cast.rresp};
  assign slv_mux_i_bus_cast.rvalid
    = {slv_3_bus_i_cast.rvalid, slv_2_bus_i_cast.rvalid, slv_1_bus_i_cast.rvalid, slv_0_bus_i_cast.rvalid};

  localparam C_NUM_MASTER_SLOTS = 4;
  localparam C_M_AXI_BASE_ADDR
    = 256'h00000000_00003000_00000000_00002000_00000000_00001000_00000000_00000000;
  localparam C_M_AXI_ADDR_WIDTH         = {C_NUM_MASTER_SLOTS{32'h0000_000c}};
  localparam C_M_AXI_WRITE_CONNECTIVITY = {C_NUM_MASTER_SLOTS{32'h0000_0001}};
  localparam C_M_AXI_READ_CONNECTIVITY  = {C_NUM_MASTER_SLOTS{32'h0000_0001}};
  localparam C_M_AXI_WRITE_ISSUING      = {C_NUM_MASTER_SLOTS{32'h0000_0001}};
  localparam C_M_AXI_READ_ISSUING       = {C_NUM_MASTER_SLOTS{32'h0000_0001}};
  localparam C_M_AXI_SECURE             = {C_NUM_MASTER_SLOTS{32'h0000_0000}};


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
  ) inst (
    .aclk          (clk_i                     ),
    .aresetn       (~reset_i                  ),
    .s_axi_awid    (1'H0                      ),
    .s_axi_awaddr  (mst_bus_i_cast.awaddr     ),
    .s_axi_awlen   (8'H00                     ),
    .s_axi_awsize  (3'H0                      ),
    .s_axi_awburst (2'H0                      ),
    .s_axi_awlock  (1'H0                      ),
    .s_axi_awcache (4'H0                      ),
    .s_axi_awprot  (3'H0                      ),
    .s_axi_awqos   (4'H0                      ),
    .s_axi_awuser  (1'H0                      ),
    .s_axi_awvalid (mst_bus_i_cast.awvalid    ),
    .s_axi_awready (mst_bus_o_cast.awready    ),
    .s_axi_wid     (1'H0                      ),
    .s_axi_wdata   (mst_bus_i_cast.wdata      ),
    .s_axi_wstrb   (mst_bus_i_cast.wstrb      ),
    .s_axi_wlast   (1'H1                      ),
    .s_axi_wuser   (1'H0                      ),
    .s_axi_wvalid  (mst_bus_i_cast.wvalid     ),
    .s_axi_wready  (mst_bus_o_cast.wready     ),
    .s_axi_bid     (                          ),
    .s_axi_bresp   (mst_bus_o_cast.bresp      ),
    .s_axi_buser   (                          ),
    .s_axi_bvalid  (mst_bus_o_cast.bvalid     ),
    .s_axi_bready  (mst_bus_i_cast.bready     ),
    .s_axi_arid    (1'H0                      ),
    .s_axi_araddr  (mst_bus_i_cast.araddr     ),
    .s_axi_arlen   (8'H00                     ),
    .s_axi_arsize  (3'H0                      ),
    .s_axi_arburst (2'H0                      ),
    .s_axi_arlock  (1'H0                      ),
    .s_axi_arcache (4'H0                      ),
    .s_axi_arprot  (3'H0                      ),
    .s_axi_arqos   (4'H0                      ),
    .s_axi_aruser  (1'H0                      ),
    .s_axi_arvalid (mst_bus_i_cast.arvalid    ),
    .s_axi_arready (mst_bus_o_cast.arready    ),
    .s_axi_rid     (                          ),
    .s_axi_rdata   (mst_bus_o_cast.rdata      ),
    .s_axi_rresp   (mst_bus_o_cast.rresp      ),
    .s_axi_rlast   (                          ),
    .s_axi_ruser   (                          ),
    .s_axi_rvalid  (mst_bus_o_cast.rvalid     ),
    .s_axi_rready  (mst_bus_i_cast.rready     ),
    .m_axi_awid    (                          ),
    .m_axi_awaddr  (slv_mux_o_bus_cast.awaddr ),
    .m_axi_awlen   (                          ),
    .m_axi_awsize  (                          ),
    .m_axi_awburst (                          ),
    .m_axi_awlock  (                          ),
    .m_axi_awcache (                          ),
    .m_axi_awprot  (                          ),
    .m_axi_awregion(                          ),
    .m_axi_awqos   (                          ),
    .m_axi_awuser  (                          ),
    .m_axi_awvalid (slv_mux_o_bus_cast.awvalid),
    .m_axi_awready (slv_mux_i_bus_cast.awready),
    .m_axi_wid     (                          ),
    .m_axi_wdata   (slv_mux_o_bus_cast.wdata  ),
    .m_axi_wstrb   (slv_mux_o_bus_cast.wstrb  ),
    .m_axi_wlast   (                          ),
    .m_axi_wuser   (                          ),
    .m_axi_wvalid  (slv_mux_o_bus_cast.wvalid ),
    .m_axi_wready  (slv_mux_i_bus_cast.wready ),
    .m_axi_bid     ({C_NUM_MASTER_SLOTS{1'b0}}),
    .m_axi_bresp   (slv_mux_i_bus_cast.bresp  ),
    .m_axi_buser   ({C_NUM_MASTER_SLOTS{1'b0}}),
    .m_axi_bvalid  (slv_mux_i_bus_cast.bvalid ),
    .m_axi_bready  (slv_mux_o_bus_cast.bready ),
    .m_axi_arid    (                          ),
    .m_axi_araddr  (slv_mux_o_bus_cast.araddr ),
    .m_axi_arlen   (                          ),
    .m_axi_arsize  (                          ),
    .m_axi_arburst (                          ),
    .m_axi_arlock  (                          ),
    .m_axi_arcache (                          ),
    .m_axi_arprot  (                          ),
    .m_axi_arregion(                          ),
    .m_axi_arqos   (                          ),
    .m_axi_aruser  (                          ),
    .m_axi_arvalid (slv_mux_o_bus_cast.arvalid),
    .m_axi_arready (slv_mux_i_bus_cast.arready),
    .m_axi_rid     ({C_NUM_MASTER_SLOTS{1'b0}}),
    .m_axi_rdata   (slv_mux_i_bus_cast.rdata  ),
    .m_axi_rresp   (slv_mux_i_bus_cast.rresp  ),
    .m_axi_rlast   ({C_NUM_MASTER_SLOTS{1'b1}}),
    .m_axi_ruser   ({C_NUM_MASTER_SLOTS{1'b0}}),
    .m_axi_rvalid  (slv_mux_i_bus_cast.rvalid ),
    .m_axi_rready  (slv_mux_o_bus_cast.rready )
  );

endmodule
