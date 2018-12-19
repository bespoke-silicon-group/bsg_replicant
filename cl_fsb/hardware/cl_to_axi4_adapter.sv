/**
 *  cl_to_axi4_adapter.sv
 *
 *  write FSB/Trace data to SH through axi-4 pcim interface
 */

module cl_to_axi4_adapter #(
  parameter FSB_WIDTH=80)
(
  input clk_i
  ,input resetn_i
  ,input sh_cl_flr_assert
  ,axil_bus_t.master sh_ocl_cfg_bus
  ,axi_bus_t.slave cl_sh_pcim_bus
  // trace data input
  ,axis_bus_t.master axis_data_bus
  // fsb cmd input
  ,input fsb_wvalid
  ,input [FSB_WIDTH-1:0] fsb_wdata
  ,output fsb_yumi
  // fsb1 cmd input
  ,input fsb_wvalid_copy
  ,input [FSB_WIDTH-1:0] fsb_wdata_copy
  ,output fsb_yumi_copy
  );


cfg_bus_t ocl_cfg_bus_0 ();
cfg_bus_t ocl_cfg_bus_5 ();


// control bus distributor
// --------------------------------------------
cl_ocl_slv CL_OCL_SLV (
  .clk             (clk_i           ),
  .sync_rst_n      (resetn_i        ),
  .sh_cl_flr_assert(sh_cl_flr_assert),
  .sh_ocl_bus      (sh_ocl_cfg_bus  ),
  .ocl_cfg_bus_0   (ocl_cfg_bus_0   ),
  .ocl_cfg_bus_5   (ocl_cfg_bus_5   )
);


axi_bus_t axi4_m_s00_bus ();
axi_bus_t axi4_m_s01_bus ();

// No.1 master fsb
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

m_axi4_fsb_adapter #(
  .DATA_WIDTH(512),
  .FSB_WIDTH (FSB_WIDTH )
) m_axi4_fsb (
  .clk_i         (clk_i         ),
  .resetn_i      (resetn_i      ),
  .cfg_bus       (ocl_cfg_bus_0 ),
  .cl_sh_pcim_bus(axi4_m_s00_bus),
  .atg_dst_sel   (              ),
  .fsb_wvalid    (fsb_wvalid    ),
  .fsb_wdata     (fsb_wdata     ),
  .fsb_yumi      (fsb_yumi      )
);


// No.2 master axis
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  axis_bus_t #(.TDATA_WIDTH(512)) axis_data_fifo_bus ();

  axis_data_fifo_v1_1_18_axis_data_fifo #(
    .C_FAMILY("virtexuplus"),
    .C_AXIS_TDATA_WIDTH(512),
    .C_AXIS_TID_WIDTH(1),
    .C_AXIS_TDEST_WIDTH(1),
    .C_AXIS_TUSER_WIDTH(1),
    .C_AXIS_SIGNAL_SET('B00000000000000000000000000011111),
    .C_FIFO_DEPTH(16),
    .C_FIFO_MODE(1),
    .C_IS_ACLK_ASYNC(0),
    .C_SYNCHRONIZER_STAGE(2),
    .C_ACLKEN_CONV_MODE(0)
  ) axis_512x16_fifo (
    .s_axis_aresetn(resetn_i),
    .m_axis_aresetn(1'H0),
    .s_axis_aclk(clk_i),
    .s_axis_aclken(1'H1),
    .s_axis_tvalid(axis_data_bus.txd_tvalid),
    .s_axis_tready(axis_data_bus.txd_tready),
    .s_axis_tdata(axis_data_bus.txd_tdata),
    .s_axis_tstrb(64'hFFFF_FFFF_FFFF_FFFF),
    .s_axis_tkeep(axis_data_bus.txd_tkeep),
    .s_axis_tlast(axis_data_bus.txd_tlast),
    .s_axis_tid(1'H0),
    .s_axis_tdest(1'H0),
    .s_axis_tuser(1'H0),
    .m_axis_aclk(1'H0),
    .m_axis_aclken(1'H1),
    .m_axis_tvalid(axis_data_fifo_bus.txd_tvalid),
    .m_axis_tready(axis_data_fifo_bus.txd_tready),
    .m_axis_tdata(axis_data_fifo_bus.txd_tdata),
    .m_axis_tstrb(),
    .m_axis_tkeep(axis_data_fifo_bus.txd_tkeep),
    .m_axis_tlast(axis_data_fifo_bus.txd_tlast),
    .m_axis_tid(),
    .m_axis_tdest(),
    .m_axis_tuser(),
    .axis_data_count(),
    .axis_wr_data_count(),
    .axis_rd_data_count()
  );



m_axi4_axis_adapter #(.DATA_WIDTH(512)) axis_wr_axi4 (
  .clk_i         (clk_i         ),
  .resetn_i      (resetn_i      ),
  .cfg_bus       (ocl_cfg_bus_5 ),
  .cl_sh_pcim_bus(axi4_m_s01_bus),
  .axis_data_bus (axis_data_fifo_bus ),
  .atg_dst_sel   (   )
);

// assign axi4_m_s01_bus.awid = 6'h1;
// assign axi4_m_s01_bus.awaddr = 64'h0;
// assign axi4_m_s01_bus.awlen =  8'h0;
// assign axi4_m_s01_bus.awsize = 3'h6;
// assign axi4_m_s01_bus.awvalid = 1'h0;

// assign axi4_m_s01_bus.wdata = 512'h0;
// assign axi4_m_s01_bus.wstrb = 64'h0;
// assign axi4_m_s01_bus.wlast = 1'h0;
// assign axi4_m_s01_bus.wvalid = 1'h0;

// assign axi4_m_s01_bus.bready = 1'h0;

// assign axi4_m_s01_bus.arid = 6'h1;
// assign axi4_m_s01_bus.araddr = 64'h0;
// assign axi4_m_s01_bus.arlen = 8'h0;
// assign axi4_m_s01_bus.arsize = 3'h6;
// assign axi4_m_s01_bus.awvalid = 1'h0;

// assign axi4_m_s01_bus.rready = 1'h0;

axi_bus_t #(
  .NUM_SLOTS (2  ),
  .ID_WIDTH  (6  ),
  .ADDR_WIDTH(64 ),
  .DATA_WIDTH(512)
) axi4_m_mux_bus ();


assign axi4_m_mux_bus.awid    = {axi4_m_s01_bus.awid, axi4_m_s00_bus.awid};
assign axi4_m_mux_bus.awaddr  = {axi4_m_s01_bus.awaddr, axi4_m_s00_bus.awaddr};
assign axi4_m_mux_bus.awlen   = {axi4_m_s01_bus.awlen, axi4_m_s00_bus.awlen};
assign axi4_m_mux_bus.awsize  = {axi4_m_s01_bus.awsize, axi4_m_s00_bus.awsize};
assign axi4_m_mux_bus.awvalid = {axi4_m_s01_bus.awvalid, axi4_m_s00_bus.awvalid};
assign {axi4_m_s01_bus.awready, axi4_m_s00_bus.awready} = axi4_m_mux_bus.awready;

assign axi4_m_mux_bus.wdata  = {axi4_m_s01_bus.wdata, axi4_m_s00_bus.wdata};
assign axi4_m_mux_bus.wstrb  = {axi4_m_s01_bus.wstrb, axi4_m_s00_bus.wstrb};
assign axi4_m_mux_bus.wlast  = {axi4_m_s01_bus.wlast, axi4_m_s00_bus.wlast};
assign axi4_m_mux_bus.wvalid = {axi4_m_s01_bus.wvalid, axi4_m_s00_bus.wvalid};
assign  {axi4_m_s01_bus.wready, axi4_m_s00_bus.wready} = axi4_m_mux_bus.wready;

assign {axi4_m_s01_bus.bid, axi4_m_s00_bus.bid} = axi4_m_mux_bus.bid;
assign {axi4_m_s01_bus.bresp, axi4_m_s00_bus.bresp} = axi4_m_mux_bus.bresp;
assign {axi4_m_s01_bus.bvalid, axi4_m_s00_bus.bvalid} = axi4_m_mux_bus.bvalid;
assign axi4_m_mux_bus.bready = {axi4_m_s01_bus.bready, axi4_m_s00_bus.bready};

assign axi4_m_mux_bus.arid    = {axi4_m_s01_bus.arid, axi4_m_s00_bus.arid};
assign axi4_m_mux_bus.araddr  = {axi4_m_s01_bus.araddr, axi4_m_s00_bus.araddr};
assign axi4_m_mux_bus.arlen   = {axi4_m_s01_bus.arlen, axi4_m_s00_bus.arlen};
assign axi4_m_mux_bus.arsize  = {axi4_m_s01_bus.arsize, axi4_m_s00_bus.arsize};
assign axi4_m_mux_bus.arvalid = {axi4_m_s01_bus.arvalid, axi4_m_s00_bus.arvalid};
assign {axi4_m_s01_bus.arready, axi4_m_s00_bus.arready} = axi4_m_mux_bus.arready;

assign {axi4_m_s01_bus.rid, axi4_m_s00_bus.rid} = axi4_m_mux_bus.rid;
assign {axi4_m_s01_bus.rdata, axi4_m_s00_bus.rdata} = axi4_m_mux_bus.rdata;
assign {axi4_m_s01_bus.rresp, axi4_m_s00_bus.rresp} = axi4_m_mux_bus.rresp;
assign {axi4_m_s01_bus.rlast, axi4_m_s00_bus.rlast} = axi4_m_mux_bus.rlast;
assign {axi4_m_s01_bus.rvalid, axi4_m_s00_bus.rvalid} = axi4_m_mux_bus.rvalid;
assign axi4_m_mux_bus.rready = {axi4_m_s01_bus.rready, axi4_m_s00_bus.rready};

axi_crossbar_v2_1_16_axi_crossbar #(
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
  .aresetn       (resetn_i              ),
  .s_axi_awid    (axi4_m_mux_bus.awid   ),
  .s_axi_awaddr  (axi4_m_mux_bus.awaddr ),
  .s_axi_awlen   (axi4_m_mux_bus.awlen  ),
  .s_axi_awsize  (axi4_m_mux_bus.awsize ),
  .s_axi_awburst (4'H0                  ),
  .s_axi_awlock  (2'H0                  ),
  .s_axi_awcache (8'H0                  ),
  .s_axi_awprot  (6'H0                  ),
  .s_axi_awqos   (8'H0                  ),
  .s_axi_awuser  (2'H0                  ),
  .s_axi_awvalid (axi4_m_mux_bus.awvalid),
  .s_axi_awready (axi4_m_mux_bus.awready),
  .s_axi_wid     (12'H000               ),
  .s_axi_wdata   (axi4_m_mux_bus.wdata  ),
  .s_axi_wstrb   (axi4_m_mux_bus.wstrb  ),
  .s_axi_wlast   (axi4_m_mux_bus.wlast  ),
  .s_axi_wuser   (2'H0                  ),
  .s_axi_wvalid  (axi4_m_mux_bus.wvalid ),
  .s_axi_wready  (axi4_m_mux_bus.wready ),
  .s_axi_bid     (axi4_m_mux_bus.bid    ),
  .s_axi_bresp   (axi4_m_mux_bus.bresp  ),
  .s_axi_buser   (                      ),
  .s_axi_bvalid  (axi4_m_mux_bus.bvalid ),
  .s_axi_bready  (axi4_m_mux_bus.bready ),
  .s_axi_arid    (axi4_m_mux_bus.arid   ),
  .s_axi_araddr  (axi4_m_mux_bus.araddr ),
  .s_axi_arlen   (axi4_m_mux_bus.arlen  ),
  .s_axi_arsize  (axi4_m_mux_bus.arsize ),
  .s_axi_arburst (4'H0                  ),
  .s_axi_arlock  (2'H0                  ),
  .s_axi_arcache (8'H0                  ),
  .s_axi_arprot  (6'H0                  ),
  .s_axi_arqos   (8'H0                  ),
  .s_axi_aruser  (2'H0                  ),
  .s_axi_arvalid (axi4_m_mux_bus.arvalid),
  .s_axi_arready (axi4_m_mux_bus.arready),
  .s_axi_rid     (axi4_m_mux_bus.rid    ),
  .s_axi_rdata   (axi4_m_mux_bus.rdata  ),
  .s_axi_rresp   (axi4_m_mux_bus.rresp  ),
  .s_axi_rlast   (axi4_m_mux_bus.rlast  ),
  .s_axi_ruser   (                      ),
  .s_axi_rvalid  (axi4_m_mux_bus.rvalid ),
  .s_axi_rready  (axi4_m_mux_bus.rready ),
  .m_axi_awid    (cl_sh_pcim_bus.awid   ),
  .m_axi_awaddr  (cl_sh_pcim_bus.awaddr ),
  .m_axi_awlen   (cl_sh_pcim_bus.awlen  ),
  .m_axi_awsize  (cl_sh_pcim_bus.awsize ),
  .m_axi_awburst (                      ),
  .m_axi_awlock  (                      ),
  .m_axi_awcache (                      ),
  .m_axi_awprot  (                      ),
  .m_axi_awregion(                      ),
  .m_axi_awqos   (                      ),
  .m_axi_awuser  (                      ),
  .m_axi_awvalid (cl_sh_pcim_bus.awvalid),
  .m_axi_awready (cl_sh_pcim_bus.awready),
  .m_axi_wid     (                      ),
  .m_axi_wdata   (cl_sh_pcim_bus.wdata  ),
  .m_axi_wstrb   (cl_sh_pcim_bus.wstrb  ),
  .m_axi_wlast   (cl_sh_pcim_bus.wlast  ),
  .m_axi_wuser   (                      ),
  .m_axi_wvalid  (cl_sh_pcim_bus.wvalid ),
  .m_axi_wready  (cl_sh_pcim_bus.wready ),
  .m_axi_bid     (cl_sh_pcim_bus.bid    ),
  .m_axi_bresp   (cl_sh_pcim_bus.bresp  ),
  .m_axi_buser   (1'H0                  ),
  .m_axi_bvalid  (cl_sh_pcim_bus.bvalid ),
  .m_axi_bready  (cl_sh_pcim_bus.bready ),
  .m_axi_arid    (cl_sh_pcim_bus.arid   ),
  .m_axi_araddr  (cl_sh_pcim_bus.araddr ),
  .m_axi_arlen   (cl_sh_pcim_bus.arlen  ),
  .m_axi_arsize  (cl_sh_pcim_bus.arsize ),
  .m_axi_arburst (                      ),
  .m_axi_arlock  (                      ),
  .m_axi_arcache (                      ),
  .m_axi_arprot  (                      ),
  .m_axi_arregion(                      ),
  .m_axi_arqos   (                      ),
  .m_axi_aruser  (                      ),
  .m_axi_arvalid (cl_sh_pcim_bus.arvalid),
  .m_axi_arready (cl_sh_pcim_bus.arready),
  .m_axi_rid     (cl_sh_pcim_bus.rid    ),
  .m_axi_rdata   (cl_sh_pcim_bus.rdata  ),
  .m_axi_rresp   (cl_sh_pcim_bus.rresp  ),
  .m_axi_rlast   (cl_sh_pcim_bus.rlast  ),
  .m_axi_ruser   (1'H0                  ),
  .m_axi_rvalid  (cl_sh_pcim_bus.rvalid ),
  .m_axi_rready  (cl_sh_pcim_bus.rready )
);


endmodule
