/**
*  cl_axil_mux4.sv
*
*  axil 1 to 4 multiplexer
*/


module cl_axil_mux4 (
  input wire aclk
  ,input wire aresetn
  ,axil_bus_t.master axil_m_bus
  ,axil_bus_t.slave axil_s_m00_bus
  ,axil_bus_t.slave axil_s_m01_bus
  ,axil_bus_t.slave axil_s_m02_bus
  ,axil_bus_t.slave axil_s_m03_bus
);

  axil_bus_t #(.NUM_SLOTS(4)) axil_s_mux_bus ();

  assign {
    axil_s_m03_bus.awaddr
    ,axil_s_m02_bus.awaddr
    ,axil_s_m01_bus.awaddr
    ,axil_s_m00_bus.awaddr
  }  = axil_s_mux_bus.awaddr;
  assign {
    axil_s_m03_bus.awvalid
    ,axil_s_m02_bus.awvalid
    ,axil_s_m01_bus.awvalid
    ,axil_s_m00_bus.awvalid
  } = axil_s_mux_bus.awvalid;
  assign axil_s_mux_bus.awready
    = {axil_s_m03_bus.awready
      ,axil_s_m02_bus.awready
      ,axil_s_m01_bus.awready
      ,axil_s_m00_bus.awready};
  assign {
    axil_s_m03_bus.wdata
    ,axil_s_m02_bus.wdata
    ,axil_s_m01_bus.wdata
    ,axil_s_m00_bus.wdata
  }     = axil_s_mux_bus.wdata;
  assign {
    axil_s_m03_bus.wstrb
    ,axil_s_m02_bus.wstrb
    ,axil_s_m01_bus.wstrb
    ,axil_s_m00_bus.wstrb
  }     = axil_s_mux_bus.wstrb;
  assign {
    axil_s_m03_bus.wvalid
    ,axil_s_m02_bus.wvalid
    ,axil_s_m01_bus.wvalid
    ,axil_s_m00_bus.wvalid
  }   = axil_s_mux_bus.wvalid;
  assign axil_s_mux_bus.wready
    = {axil_s_m03_bus.wready
      ,axil_s_m02_bus.wready
      ,axil_s_m01_bus.wready
      ,axil_s_m00_bus.wready};
  assign axil_s_mux_bus.bresp
    = {axil_s_m03_bus.bresp
      ,axil_s_m02_bus.bresp
      ,axil_s_m01_bus.bresp
      ,axil_s_m00_bus.bresp};
  assign axil_s_mux_bus.bvalid
    = {axil_s_m03_bus.bvalid
      ,axil_s_m02_bus.bvalid
      ,axil_s_m01_bus.bvalid
      ,axil_s_m00_bus.bvalid};
  assign {
    axil_s_m03_bus.bready
    ,axil_s_m02_bus.bready
    ,axil_s_m01_bus.bready
    ,axil_s_m00_bus.bready
  }   = axil_s_mux_bus.bready;
  assign {
    axil_s_m03_bus.araddr
    ,axil_s_m02_bus.araddr
    ,axil_s_m01_bus.araddr
    ,axil_s_m00_bus.araddr
  }   = axil_s_mux_bus.araddr;
  assign {
    axil_s_m03_bus.arvalid
    ,axil_s_m02_bus.arvalid
    ,axil_s_m01_bus.arvalid
    ,axil_s_m00_bus.arvalid
  } = axil_s_mux_bus.arvalid;
  assign axil_s_mux_bus.arready
    = {
      axil_s_m03_bus.arready
      ,axil_s_m02_bus.arready
      ,axil_s_m01_bus.arready
      ,axil_s_m00_bus.arready
    };
  assign axil_s_mux_bus.rdata
    = {
      axil_s_m03_bus.rdata
      ,axil_s_m02_bus.rdata
      ,axil_s_m01_bus.rdata
      ,axil_s_m00_bus.rdata
    };
  assign axil_s_mux_bus.rresp
    = {
      axil_s_m03_bus.rresp
      ,axil_s_m02_bus.rresp
      ,axil_s_m01_bus.rresp
      ,axil_s_m00_bus.rresp
    };
  assign axil_s_mux_bus.rvalid
    = {
      axil_s_m03_bus.rvalid
      ,axil_s_m02_bus.rvalid
      ,axil_s_m01_bus.rvalid
      ,axil_s_m00_bus.rvalid
    };
  assign {
    axil_s_m03_bus.rready
    ,axil_s_m02_bus.rready
    ,axil_s_m01_bus.rready
    ,axil_s_m00_bus.rready
  }   = axil_s_mux_bus.rready;

  localparam C_NUM_MASTER_SLOTS = 4;
  localparam C_M_AXI_BASE_ADDR
    = 256'h00000000_00003000_00000000_00002000_00000000_00001000_00000000_00000000;
  localparam C_M_AXI_ADDR_WIDTH = {C_NUM_MASTER_SLOTS{32'h0000_000c}};
  localparam C_M_AXI_WRITE_CONNECTIVITY = {C_NUM_MASTER_SLOTS{32'h0000_0001}};
  localparam C_M_AXI_READ_CONNECTIVITY = {C_NUM_MASTER_SLOTS{32'h0000_0001}};
  localparam C_M_AXI_WRITE_ISSUING = {C_NUM_MASTER_SLOTS{32'h0000_0001}};
  localparam C_M_AXI_READ_ISSUING = {C_NUM_MASTER_SLOTS{32'h0000_0001}};
  localparam C_M_AXI_SECURE = {C_NUM_MASTER_SLOTS{32'h0000_0000}};


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
    .aclk          (aclk                      ),
    .aresetn       (aresetn                   ),
    .s_axi_awid    (1'H0                      ),
    .s_axi_awaddr  (axil_m_bus.awaddr         ),
    .s_axi_awlen   (8'H00                     ),
    .s_axi_awsize  (3'H0                      ),
    .s_axi_awburst (2'H0                      ),
    .s_axi_awlock  (1'H0                      ),
    .s_axi_awcache (4'H0                      ),
    .s_axi_awprot  (3'H0                      ),
    .s_axi_awqos   (4'H0                      ),
    .s_axi_awuser  (1'H0                      ),
    .s_axi_awvalid (axil_m_bus.awvalid        ),
    .s_axi_awready (axil_m_bus.awready        ),
    .s_axi_wid     (1'H0                      ),
    .s_axi_wdata   (axil_m_bus.wdata          ),
    .s_axi_wstrb   (axil_m_bus.wstrb          ),
    .s_axi_wlast   (1'H1                      ),
    .s_axi_wuser   (1'H0                      ),
    .s_axi_wvalid  (axil_m_bus.wvalid         ),
    .s_axi_wready  (axil_m_bus.wready         ),
    .s_axi_bid     (                          ),
    .s_axi_bresp   (axil_m_bus.bresp          ),
    .s_axi_buser   (                          ),
    .s_axi_bvalid  (axil_m_bus.bvalid         ),
    .s_axi_bready  (axil_m_bus.bready         ),
    .s_axi_arid    (1'H0                      ),
    .s_axi_araddr  (axil_m_bus.araddr         ),
    .s_axi_arlen   (8'H00                     ),
    .s_axi_arsize  (3'H0                      ),
    .s_axi_arburst (2'H0                      ),
    .s_axi_arlock  (1'H0                      ),
    .s_axi_arcache (4'H0                      ),
    .s_axi_arprot  (3'H0                      ),
    .s_axi_arqos   (4'H0                      ),
    .s_axi_aruser  (1'H0                      ),
    .s_axi_arvalid (axil_m_bus.arvalid        ),
    .s_axi_arready (axil_m_bus.arready        ),
    .s_axi_rid     (                          ),
    .s_axi_rdata   (axil_m_bus.rdata          ),
    .s_axi_rresp   (axil_m_bus.rresp          ),
    .s_axi_rlast   (                          ),
    .s_axi_ruser   (                          ),
    .s_axi_rvalid  (axil_m_bus.rvalid         ),
    .s_axi_rready  (axil_m_bus.rready         ),
    .m_axi_awid    (                          ),
    .m_axi_awaddr  (axil_s_mux_bus.awaddr     ),
    .m_axi_awlen   (                          ),
    .m_axi_awsize  (                          ),
    .m_axi_awburst (                          ),
    .m_axi_awlock  (                          ),
    .m_axi_awcache (                          ),
    .m_axi_awprot  (                          ),
    .m_axi_awregion(                          ),
    .m_axi_awqos   (                          ),
    .m_axi_awuser  (                          ),
    .m_axi_awvalid (axil_s_mux_bus.awvalid    ),
    .m_axi_awready (axil_s_mux_bus.awready    ),
    .m_axi_wid     (                          ),
    .m_axi_wdata   (axil_s_mux_bus.wdata      ),
    .m_axi_wstrb   (axil_s_mux_bus.wstrb      ),
    .m_axi_wlast   (                          ),
    .m_axi_wuser   (                          ),
    .m_axi_wvalid  (axil_s_mux_bus.wvalid     ),
    .m_axi_wready  (axil_s_mux_bus.wready     ),
    .m_axi_bid     ({C_NUM_MASTER_SLOTS{1'b0}}),
    .m_axi_bresp   (axil_s_mux_bus.bresp      ),
    .m_axi_buser   ({C_NUM_MASTER_SLOTS{1'b0}}),
    .m_axi_bvalid  (axil_s_mux_bus.bvalid     ),
    .m_axi_bready  (axil_s_mux_bus.bready     ),
    .m_axi_arid    (                          ),
    .m_axi_araddr  (axil_s_mux_bus.araddr     ),
    .m_axi_arlen   (                          ),
    .m_axi_arsize  (                          ),
    .m_axi_arburst (                          ),
    .m_axi_arlock  (                          ),
    .m_axi_arcache (                          ),
    .m_axi_arprot  (                          ),
    .m_axi_arregion(                          ),
    .m_axi_arqos   (                          ),
    .m_axi_aruser  (                          ),
    .m_axi_arvalid (axil_s_mux_bus.arvalid    ),
    .m_axi_arready (axil_s_mux_bus.arready    ),
    .m_axi_rid     ({C_NUM_MASTER_SLOTS{1'b0}}),
    .m_axi_rdata   (axil_s_mux_bus.rdata      ),
    .m_axi_rresp   (axil_s_mux_bus.rresp      ),
    .m_axi_rlast   ({C_NUM_MASTER_SLOTS{1'b1}}),
    .m_axi_ruser   ({C_NUM_MASTER_SLOTS{1'b0}}),
    .m_axi_rvalid  (axil_s_mux_bus.rvalid     ),
    .m_axi_rready  (axil_s_mux_bus.rready     )
  );

endmodule
