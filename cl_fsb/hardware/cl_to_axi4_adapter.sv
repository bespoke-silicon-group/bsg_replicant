/**
 *  cl_to_axi4_adapter.sv
 *
 *  write FSB/Trace data to SH through axi-4 pcim interface
 */

`include "bsg_axi_bus_pkg.vh"

module cl_to_axi4_adapter #(
  fsb_width_p = 80
  ,data_width_p = 512
  ,pcis_id_width_p = 6
  ,pcis_addr_width_p = 64
  ,pcis_data_width_p = 512
  ,pcim_id_width_p = 6
  ,pcim_addr_width_p = 64
  ,pcim_data_width_p = 512
  ,axil_mosi_bus_width_lp = `bsg_axil_mosi_bus_width(1)
  ,axil_miso_bus_width_lp = `bsg_axil_miso_bus_width(1)
  ,pcis_mosi_bus_width_lp = `bsg_axi_mosi_bus_width(1, pcis_id_width_p, pcis_addr_width_p, pcis_data_width_p)
  ,pcis_miso_bus_width_lp = `bsg_axi_miso_bus_width(1, pcis_id_width_p, pcis_addr_width_p, pcis_data_width_p)
  ,pcim_mosi_bus_width_lp = `bsg_axi_mosi_bus_width(1, pcim_id_width_p, pcim_addr_width_p, pcim_data_width_p)
  ,pcim_miso_bus_width_lp = `bsg_axi_miso_bus_width(1, pcim_id_width_p, pcim_addr_width_p, pcim_data_width_p)
  ,axis_bus_width_lp = `bsg_axis_bus_width(data_width_p)
  )(
  input clk_i
  ,input reset_i
  ,input sh_cl_flr_assert
  // axil config bus
  ,input  [axil_mosi_bus_width_lp-1:0] s_ocl_bus_i
  ,output [axil_miso_bus_width_lp-1:0] s_ocl_bus_o
  // pcis slave bus
  ,input  [pcis_mosi_bus_width_lp-1:0] s_pcis_bus_i
  ,output [pcis_miso_bus_width_lp-1:0] s_pcis_bus_o
  // pcim master bus
  ,input  [pcim_miso_bus_width_lp-1:0] m_pcim_bus_i
  ,output [pcim_mosi_bus_width_lp-1:0] m_pcim_bus_o
  // trace data in
  ,input  [axis_bus_width_lp-1:0] s_axis_bus_i
  ,output [axis_bus_width_lp-1:0] s_axis_bus_o
  // fsb data in
  ,input fsb_wvalid
  ,input [fsb_width_p-1:0] fsb_wdata
  ,output fsb_yumi
  );

// --------------------------------
`declare_bsg_axil_bus_s(1, bsg_axil_mosi_bus_s, bsg_axil_miso_bus_s);
bsg_axil_mosi_bus_s s_ocl_bus_i_cast;
bsg_axil_miso_bus_s s_ocl_bus_o_cast;
assign s_ocl_bus_i_cast = s_ocl_bus_i;
assign s_ocl_bus_o      = s_ocl_bus_o_cast;

// --------------------------------
`declare_bsg_axi_bus_s(1, pcis_id_width_p, pcis_addr_width_p, pcis_data_width_p,
  bsg_pcis_mosi_bus_s, bsg_pcis_miso_bus_s);
bsg_pcis_mosi_bus_s s_pcis_bus_i_cast;
bsg_pcis_miso_bus_s s_pcis_bus_o_cast;
assign s_pcis_bus_i_cast = s_pcis_bus_i;
assign s_pcis_bus_o      = s_pcis_bus_o_cast;

// --------------------------------
`declare_bsg_axi_bus_s(1, pcim_id_width_p, pcim_addr_width_p, pcim_data_width_p,
  bsg_pcim_mosi_bus_s, bsg_pcim_miso_bus_s);
bsg_pcim_mosi_bus_s m_pcim_bus_o_cast, sh_pcim_mosi_bus_0, sh_pcim_mosi_bus_1;
bsg_pcim_miso_bus_s m_pcim_bus_i_cast, sh_pcim_miso_bus_0, sh_pcim_miso_bus_1;
assign m_pcim_bus_i_cast = m_pcim_bus_i;
assign m_pcim_bus_o      = m_pcim_bus_o_cast;

// --------------------------------
`declare_bsg_axis_bus_s(512, bsg_axisx512_mosi_bus_s, bsg_axisx512_miso_bus_s);
bsg_axisx512_mosi_bus_s s_axis_bus_i_cast;
bsg_axisx512_miso_bus_s s_axis_bus_o_cast;
assign s_axis_bus_i_cast = s_axis_bus_i;
assign s_axis_bus_o = s_axis_bus_o_cast;


cfg_bus_t ocl_cfg_bus_0 ();
cfg_bus_t ocl_cfg_bus_5 ();

axil_bus_t sh_ocl_cfg_bus();

assign sh_ocl_cfg_bus.awaddr = s_ocl_bus_i_cast.awaddr;
assign sh_ocl_cfg_bus.awvalid = s_ocl_bus_i_cast.awvalid;
assign sh_ocl_cfg_bus.wdata = s_ocl_bus_i_cast.wdata;
assign sh_ocl_cfg_bus.wstrb = s_ocl_bus_i_cast.wstrb;
assign sh_ocl_cfg_bus.wvalid = s_ocl_bus_i_cast.wvalid;
assign sh_ocl_cfg_bus.bready = s_ocl_bus_i_cast.bready;
assign sh_ocl_cfg_bus.araddr = s_ocl_bus_i_cast.araddr;
assign sh_ocl_cfg_bus.arvalid = s_ocl_bus_i_cast.arvalid;
assign sh_ocl_cfg_bus.rready = s_ocl_bus_i_cast.rready;

assign s_ocl_bus_o_cast.awready = sh_ocl_cfg_bus.awready;
assign s_ocl_bus_o_cast.wready = sh_ocl_cfg_bus.wready;
assign s_ocl_bus_o_cast.bresp = sh_ocl_cfg_bus.bresp;
assign s_ocl_bus_o_cast.bvalid = sh_ocl_cfg_bus.bvalid;
assign s_ocl_bus_o_cast.arready = sh_ocl_cfg_bus.arready;
assign s_ocl_bus_o_cast.rdata = sh_ocl_cfg_bus.rdata;
assign s_ocl_bus_o_cast.rresp = sh_ocl_cfg_bus.rresp;
assign s_ocl_bus_o_cast.rvalid = sh_ocl_cfg_bus.rvalid;

// control bus distributor
// --------------------------------------------
cl_ocl_slv CL_OCL_SLV (
  .clk             (clk_i           ),
  .sync_rst_n      (~reset_i        ),
  .sh_cl_flr_assert(sh_cl_flr_assert),
  .sh_ocl_bus      (sh_ocl_cfg_bus  ),
  .ocl_cfg_bus_0   (ocl_cfg_bus_0   ),
  .ocl_cfg_bus_5   (ocl_cfg_bus_5   )
);


`declare_bsg_axi_bus_s(1, pcim_id_width_p, pcim_addr_width_p, pcim_data_width_p,
  bsg_pcim_mosi_s, bsg_pcim_miso_s);
bsg_pcim_mosi_s axi_mosi_bus_0, axi_mosi_bus_1;
bsg_pcim_miso_s axi_miso_bus_0, axi_miso_bus_1;

// No.1 master fsb
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

m_axi4_fsb_adapter #(
  .axi4_width_p(512),
  .fsb_width_p (fsb_width_p )
) m_axi4_fsb (
  .clk_i         (clk_i         ),
  .reset_i      (reset_i      ),
  .cfg_bus       (ocl_cfg_bus_0 ),
  .m_axi_bus_i(axi_miso_bus_0),
  .m_axi_bus_o(axi_mosi_bus_0),
  .atg_dst_sel   (              ),
  .fsb_wvalid    (fsb_wvalid    ),
  .fsb_wdata     (fsb_wdata     ),
  .fsb_yumi      (fsb_yumi      )
);


// No.2 master axis
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bsg_axisx512_mosi_bus_s axis_mosi_bus_r;
bsg_axisx512_miso_bus_s axis_miso_bus_r;

  // bsg_fifo_1r1w_small #(
  //   .width_p(512)
  //   ,.els_p             (8)
  //   ,.ready_THEN_valid_p(0))
  // axis_fifo_512 (
  //   .clk_i  (clk_i),
  //   .reset_i(reset_i),
  //   .v_i    (s_axis_bus_i_cast.txd_tvalid),
  //   .ready_o(s_axis_bus_o_cast.txd_tready),
  //   .data_i (s_axis_bus_i_cast.txd_tdata ),
  //   .v_o    (axis_mosi_bus_r.txd_tvalid),
  //   .data_o (axis_mosi_bus_r.txd_tdata),
  //   .yumi_i (axis_miso_bus_r.txd_tready)
  // );



m_axi4_axis_adapter axi4_adapter (
  .clk_i       (clk_i        ),
  .reset_i     (reset_i      ),
  .cfg_bus     (ocl_cfg_bus_5),
  .m_axi_bus_i (axi_miso_bus_1  ),
  .m_axi_bus_o (axi_mosi_bus_1  ),
  .s_axis_bus_i(s_axis_bus_i_cast ),
  .s_axis_bus_o(s_axis_bus_o_cast ),
  .atg_dst_sel (             )
);


`declare_bsg_axi_bus_s(2, pcim_id_width_p, pcim_addr_width_p, pcim_data_width_p,
  bsg_axi_mosi_2bus_s, bsg_axi_miso_2bus_s);
bsg_axi_mosi_2bus_s axi_mosi_mux_bus;
bsg_axi_miso_2bus_s axi_miso_mux_bus;


assign axi_mosi_mux_bus.awid    = {axi_mosi_bus_1.awid, axi_mosi_bus_0.awid};
assign axi_mosi_mux_bus.awaddr  = {axi_mosi_bus_1.awaddr, axi_mosi_bus_0.awaddr};
assign axi_mosi_mux_bus.awlen   = {axi_mosi_bus_1.awlen, axi_mosi_bus_0.awlen};
assign axi_mosi_mux_bus.awsize  = {axi_mosi_bus_1.awsize, axi_mosi_bus_0.awsize};
assign axi_mosi_mux_bus.awvalid = {axi_mosi_bus_1.awvalid, axi_mosi_bus_0.awvalid};
assign {axi_miso_bus_1.awready, axi_miso_bus_0.awready} = axi_miso_mux_bus.awready;

assign axi_mosi_mux_bus.wdata  = {axi_mosi_bus_1.wdata, axi_mosi_bus_0.wdata};
assign axi_mosi_mux_bus.wstrb  = {axi_mosi_bus_1.wstrb, axi_mosi_bus_0.wstrb};
assign axi_mosi_mux_bus.wlast  = {axi_mosi_bus_1.wlast, axi_mosi_bus_0.wlast};
assign axi_mosi_mux_bus.wvalid = {axi_mosi_bus_1.wvalid, axi_mosi_bus_0.wvalid};
assign  {axi_miso_bus_1.wready, axi_miso_bus_0.wready} = axi_miso_mux_bus.wready;

assign {axi_miso_bus_1.bid, axi_miso_bus_0.bid} = axi_miso_mux_bus.bid;
assign {axi_miso_bus_1.bresp, axi_miso_bus_0.bresp} = axi_miso_mux_bus.bresp;
assign {axi_miso_bus_1.bvalid, axi_miso_bus_0.bvalid} = axi_miso_mux_bus.bvalid;
assign axi_mosi_mux_bus.bready = {axi_mosi_bus_1.bready, axi_mosi_bus_0.bready};

assign axi_mosi_mux_bus.arid    = {axi_mosi_bus_1.arid, axi_mosi_bus_0.arid};
assign axi_mosi_mux_bus.araddr  = {axi_mosi_bus_1.araddr, axi_mosi_bus_0.araddr};
assign axi_mosi_mux_bus.arlen   = {axi_mosi_bus_1.arlen, axi_mosi_bus_0.arlen};
assign axi_mosi_mux_bus.arsize  = {axi_mosi_bus_1.arsize, axi_mosi_bus_0.arsize};
assign axi_mosi_mux_bus.arvalid = {axi_mosi_bus_1.arvalid, axi_mosi_bus_0.arvalid};
assign {axi_miso_bus_1.arready, axi_miso_bus_0.arready} = axi_miso_mux_bus.arready;

assign {axi_miso_bus_1.rid, axi_miso_bus_0.rid} = axi_miso_mux_bus.rid;
assign {axi_miso_bus_1.rdata, axi_miso_bus_0.rdata} = axi_miso_mux_bus.rdata;
assign {axi_miso_bus_1.rresp, axi_miso_bus_0.rresp} = axi_miso_mux_bus.rresp;
assign {axi_miso_bus_1.rlast, axi_miso_bus_0.rlast} = axi_miso_mux_bus.rlast;
assign {axi_miso_bus_1.rvalid, axi_miso_bus_0.rvalid} = axi_miso_mux_bus.rvalid;
assign axi_mosi_mux_bus.rready = {axi_mosi_bus_1.rready, axi_mosi_bus_0.rready};

axi_crossbar_v2_1_18_axi_crossbar #(
  .C_FAMILY                   ("virtexuplus"       ),
  .C_NUM_SLAVE_SLOTS          (2                   ),
  .C_NUM_MASTER_SLOTS         (1                   ),
  .C_AXI_ID_WIDTH             (6                   ),
  .C_AXI_ADDR_WIDTH           (64                  ),
  .C_AXI_DATA_WIDTH           (512                 ),
  .C_AXI_PROTOCOL             (0                   ),
  .C_NUM_ADDR_RANGES          (1                   ),
  .C_M_AXI_BASE_ADDR          (64'H0000000000000000),
  .C_M_AXI_ADDR_WIDTH         (32'H00000040        ),
  .C_S_AXI_BASE_ID            (64'H0000000000000000),
  .C_S_AXI_THREAD_ID_WIDTH    (64'H0000000500000005),
  .C_AXI_SUPPORTS_USER_SIGNALS(0                   ),
  .C_AXI_AWUSER_WIDTH         (1                   ),
  .C_AXI_ARUSER_WIDTH         (1                   ),
  .C_AXI_WUSER_WIDTH          (1                   ),
  .C_AXI_RUSER_WIDTH          (1                   ),
  .C_AXI_BUSER_WIDTH          (1                   ),
  .C_M_AXI_WRITE_CONNECTIVITY (32'H00000003        ),
  .C_M_AXI_READ_CONNECTIVITY  (32'H00000003        ),
  .C_R_REGISTER               (0                   ),
  .C_S_AXI_SINGLE_THREAD      (64'H0000000000000000),
  .C_S_AXI_WRITE_ACCEPTANCE   (64'H0000000200000002),
  .C_S_AXI_READ_ACCEPTANCE    (64'H0000000200000002),
  .C_M_AXI_WRITE_ISSUING      (32'H00000004        ),
  .C_M_AXI_READ_ISSUING       (32'H00000004        ),
  .C_S_AXI_ARB_PRIORITY       (64'H0000000000000000),
  .C_M_AXI_SECURE             (32'H00000000        ),
  .C_CONNECTIVITY_MODE        (1                   )
) inst (
  .aclk          (clk_i                 ),
  .aresetn       (~reset_i              ),
  .s_axi_awid    (axi_mosi_mux_bus.awid   ),
  .s_axi_awaddr  (axi_mosi_mux_bus.awaddr ),
  .s_axi_awlen   (axi_mosi_mux_bus.awlen  ),
  .s_axi_awsize  (axi_mosi_mux_bus.awsize ),
  .s_axi_awburst (4'H0                  ),
  .s_axi_awlock  (2'H0                  ),
  .s_axi_awcache (8'H0                  ),
  .s_axi_awprot  (6'H0                  ),
  .s_axi_awqos   (8'H0                  ),
  .s_axi_awuser  (2'H0                  ),
  .s_axi_awvalid (axi_mosi_mux_bus.awvalid),
  .s_axi_awready (axi_miso_mux_bus.awready),
  .s_axi_wid     (12'H000               ),
  .s_axi_wdata   (axi_mosi_mux_bus.wdata  ),
  .s_axi_wstrb   (axi_mosi_mux_bus.wstrb  ),
  .s_axi_wlast   (axi_mosi_mux_bus.wlast  ),
  .s_axi_wuser   (2'H0                  ),
  .s_axi_wvalid  (axi_mosi_mux_bus.wvalid ),
  .s_axi_wready  (axi_miso_mux_bus.wready ),
  .s_axi_bid     (axi_miso_mux_bus.bid    ),
  .s_axi_bresp   (axi_miso_mux_bus.bresp  ),
  .s_axi_buser   (                      ),
  .s_axi_bvalid  (axi_miso_mux_bus.bvalid ),
  .s_axi_bready  (axi_mosi_mux_bus.bready ),
  .s_axi_arid    (axi_mosi_mux_bus.arid   ),
  .s_axi_araddr  (axi_mosi_mux_bus.araddr ),
  .s_axi_arlen   (axi_mosi_mux_bus.arlen  ),
  .s_axi_arsize  (axi_mosi_mux_bus.arsize ),
  .s_axi_arburst (4'H0                  ),
  .s_axi_arlock  (2'H0                  ),
  .s_axi_arcache (8'H0                  ),
  .s_axi_arprot  (6'H0                  ),
  .s_axi_arqos   (8'H0                  ),
  .s_axi_aruser  (2'H0                  ),
  .s_axi_arvalid (axi_mosi_mux_bus.arvalid),
  .s_axi_arready (axi_miso_mux_bus.arready),
  .s_axi_rid     (axi_miso_mux_bus.rid    ),
  .s_axi_rdata   (axi_miso_mux_bus.rdata  ),
  .s_axi_rresp   (axi_miso_mux_bus.rresp  ),
  .s_axi_rlast   (axi_miso_mux_bus.rlast  ),
  .s_axi_ruser   (                      ),
  .s_axi_rvalid  (axi_miso_mux_bus.rvalid ),
  .s_axi_rready  (axi_mosi_mux_bus.rready ),
  .m_axi_awid    (m_pcim_bus_o_cast.awid   ),
  .m_axi_awaddr  (m_pcim_bus_o_cast.awaddr ),
  .m_axi_awlen   (m_pcim_bus_o_cast.awlen  ),
  .m_axi_awsize  (m_pcim_bus_o_cast.awsize ),
  .m_axi_awburst (                      ),
  .m_axi_awlock  (                      ),
  .m_axi_awcache (                      ),
  .m_axi_awprot  (                      ),
  .m_axi_awregion(                      ),
  .m_axi_awqos   (                      ),
  .m_axi_awuser  (                      ),
  .m_axi_awvalid (m_pcim_bus_o_cast.awvalid),
  .m_axi_awready (m_pcim_bus_i_cast.awready),
  .m_axi_wid     (                      ),
  .m_axi_wdata   (m_pcim_bus_o_cast.wdata  ),
  .m_axi_wstrb   (m_pcim_bus_o_cast.wstrb  ),
  .m_axi_wlast   (m_pcim_bus_o_cast.wlast  ),
  .m_axi_wuser   (                      ),
  .m_axi_wvalid  (m_pcim_bus_o_cast.wvalid ),
  .m_axi_wready  (m_pcim_bus_i_cast.wready ),
  .m_axi_bid     (m_pcim_bus_i_cast.bid    ),
  .m_axi_bresp   (m_pcim_bus_i_cast.bresp  ),
  .m_axi_buser   (1'H0                  ),
  .m_axi_bvalid  (m_pcim_bus_i_cast.bvalid ),
  .m_axi_bready  (m_pcim_bus_o_cast.bready ),
  .m_axi_arid    (m_pcim_bus_o_cast.arid   ),
  .m_axi_araddr  (m_pcim_bus_o_cast.araddr ),
  .m_axi_arlen   (m_pcim_bus_o_cast.arlen  ),
  .m_axi_arsize  (m_pcim_bus_o_cast.arsize ),
  .m_axi_arburst (                      ),
  .m_axi_arlock  (                      ),
  .m_axi_arcache (                      ),
  .m_axi_arprot  (                      ),
  .m_axi_arregion(                      ),
  .m_axi_arqos   (                      ),
  .m_axi_aruser  (                      ),
  .m_axi_arvalid (m_pcim_bus_o_cast.arvalid),
  .m_axi_arready (m_pcim_bus_i_cast.arready),
  .m_axi_rid     (m_pcim_bus_i_cast.rid    ),
  .m_axi_rdata   (m_pcim_bus_i_cast.rdata  ),
  .m_axi_rresp   (m_pcim_bus_i_cast.rresp  ),
  .m_axi_rlast   (m_pcim_bus_i_cast.rlast  ),
  .m_axi_ruser   (1'H0                  ),
  .m_axi_rvalid  (m_pcim_bus_i_cast.rvalid ),
  .m_axi_rready  (m_pcim_bus_o_cast.rready )
);


endmodule
