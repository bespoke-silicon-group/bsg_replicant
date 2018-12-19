/**
 *  s_axil_fsb_adapter.sv
 *
 *  axi-lite (SH) <-> cl_bsg (CL)
 */

module s_axil_fsb_adapter (
  input clk_i
  ,input resetn_i
  ,axil_bus_t.master sh_ocl_bus
  ,input adpt_slave_v
  ,input [79:0] adpt_slave_data
  ,output adpt_slave_r
  ,output adpt_master_v
  ,output [79:0] adpt_master_data
  ,input adpt_master_r
);

parameter FPGA_VERSION = "virtexuplus";

// bus for axil flop output
axil_bus_t sh_ocl_bus_q();

// bus for axi_fifo_mm_s axis output
axis_bus_t #(.TDATA_WIDTH(32)) fifo_axis_bus();

// bus for datawidth coverter X4 output and datawidth coverter /4 input
axis_bus_t #(.TDATA_WIDTH(128)) axis_fsb_bus();


// flop the input OCL bus
//---------------------------------
axi_register_slice_light AXIL_OCL_REG_SLC (
.aclk          (clk_i),
.aresetn       (resetn_i),
.s_axi_awaddr  (sh_ocl_bus.awaddr[31:0]),
.s_axi_awprot  (3'h0),
.s_axi_awvalid (sh_ocl_bus.awvalid),
.s_axi_awready (sh_ocl_bus.awready),
.s_axi_wdata   (sh_ocl_bus.wdata[31:0]),
.s_axi_wstrb   (sh_ocl_bus.wstrb[3:0]),
.s_axi_wvalid  (sh_ocl_bus.wvalid),
.s_axi_wready  (sh_ocl_bus.wready),
.s_axi_bresp   (sh_ocl_bus.bresp),
.s_axi_bvalid  (sh_ocl_bus.bvalid),
.s_axi_bready  (sh_ocl_bus.bready),
.s_axi_araddr  (sh_ocl_bus.araddr[31:0]),
.s_axi_arvalid (sh_ocl_bus.arvalid),
.s_axi_arready (sh_ocl_bus.arready),
.s_axi_rdata   (sh_ocl_bus.rdata[31:0]),
.s_axi_rresp   (sh_ocl_bus.rresp),
.s_axi_rvalid  (sh_ocl_bus.rvalid),
.s_axi_rready  (sh_ocl_bus.rready),

.m_axi_awaddr  (sh_ocl_bus_q.awaddr[31:0]), 
.m_axi_awprot  (),
.m_axi_awvalid (sh_ocl_bus_q.awvalid),
.m_axi_awready (sh_ocl_bus_q.awready),
.m_axi_wdata   (sh_ocl_bus_q.wdata[31:0]),  
.m_axi_wstrb   (sh_ocl_bus_q.wstrb[3:0]),
.m_axi_wvalid  (sh_ocl_bus_q.wvalid), 
.m_axi_wready  (sh_ocl_bus_q.wready), 
.m_axi_bresp   (sh_ocl_bus_q.bresp),  
.m_axi_bvalid  (sh_ocl_bus_q.bvalid), 
.m_axi_bready  (sh_ocl_bus_q.bready), 
.m_axi_araddr  (sh_ocl_bus_q.araddr[31:0]), 
.m_axi_arvalid (sh_ocl_bus_q.arvalid),
.m_axi_arready (sh_ocl_bus_q.arready),
.m_axi_rdata   (sh_ocl_bus_q.rdata[31:0]),  
.m_axi_rresp   (sh_ocl_bus_q.rresp),  
.m_axi_rvalid  (sh_ocl_bus_q.rvalid), 
.m_axi_rready  (sh_ocl_bus_q.rready)
);




// convert axil to axis
//---------------------------------
axi_fifo_mm_s # (
  .C_FAMILY(FPGA_VERSION),
  .C_S_AXI_ID_WIDTH(4),
  .C_S_AXI_ADDR_WIDTH(32),
  .C_S_AXI_DATA_WIDTH(32),
  .C_S_AXI4_DATA_WIDTH(32),
  .C_TX_FIFO_DEPTH(512),
  .C_RX_FIFO_DEPTH(512),
  .C_TX_FIFO_PF_THRESHOLD(507),
  .C_TX_FIFO_PE_THRESHOLD(2),
  .C_RX_FIFO_PF_THRESHOLD(507),
  .C_RX_FIFO_PE_THRESHOLD(2),
  .C_USE_TX_CUT_THROUGH(0),
  .C_DATA_INTERFACE_TYPE(0),
  .C_BASEADDR(32'h80000000),
  .C_HIGHADDR(32'h80000FFF),
  .C_AXI4_BASEADDR(32'h80001000),
  .C_AXI4_HIGHADDR(32'h80002FFF),
  .C_HAS_AXIS_TID(0),
  .C_HAS_AXIS_TDEST(0),
  .C_HAS_AXIS_TUSER(0),
  .C_HAS_AXIS_TSTRB(0),
  .C_HAS_AXIS_TKEEP(0),
  .C_AXIS_TID_WIDTH(4),
  .C_AXIS_TDEST_WIDTH(4),
  .C_AXIS_TUSER_WIDTH(4),
  .C_USE_RX_CUT_THROUGH(0),
  .C_USE_TX_DATA(1),
  .C_USE_TX_CTRL(0),
  .C_USE_RX_DATA(1)
   ) 
axi_fifo_mm_s_axi_lite  (
 .interrupt(),                                  // output wire interrupt
 .s_axi_aclk(clk_i),                            // input wire s_axi_aclk
 .s_axi_aresetn(resetn_i),                      // input wire s_axi_aresetn
 .s_axi_awaddr(sh_ocl_bus_q.awaddr),            // input wire [31 : 0] s_axi_awaddr
 .s_axi_awvalid(sh_ocl_bus_q.awvalid),          // input wire s_axi_awvalid
 .s_axi_awready(sh_ocl_bus_q.awready),          // output wire s_axi_awready
 .s_axi_wdata(sh_ocl_bus_q.wdata),              // input wire [31 : 0] s_axi_wdata
 .s_axi_wstrb(sh_ocl_bus_q.wstrb),              // input wire [3 : 0] s_axi_wstrb
 .s_axi_wvalid(sh_ocl_bus_q.wvalid),            // input wire s_axi_wvalid
 .s_axi_wready(sh_ocl_bus_q.wready),            // output wire s_axi_wready
 .s_axi_bresp(sh_ocl_bus_q.bresp),              // output wire [1 : 0] s_axi_bresp
 .s_axi_bvalid(sh_ocl_bus_q.bvalid),            // output wire s_axi_bvalid
 .s_axi_bready(sh_ocl_bus_q.bready),            // input wire s_axi_bready
 .s_axi_araddr(sh_ocl_bus_q.araddr),            // input wire [31 : 0] s_axi_araddr
 .s_axi_arvalid(sh_ocl_bus_q.arvalid),          // input wire s_axi_arvalid
 .s_axi_arready(sh_ocl_bus_q.arready),          // output wire s_axi_arready
 .s_axi_rdata(sh_ocl_bus_q.rdata),              // output wire [31 : 0] s_axi_rdata
 .s_axi_rresp(sh_ocl_bus_q.rresp),              // output wire [1 : 0] s_axi_rresp
 .s_axi_rvalid(sh_ocl_bus_q.rvalid),            // output wire s_axi_rvalid
 .s_axi_rready(sh_ocl_bus_q.rready),            // input wire s_axi_rready
 .s_axi4_awid(4'h0),
 .s_axi4_awaddr(32'h0),
 .s_axi4_awlen(8'h0),
 .s_axi4_awsize(3'h0),
 .s_axi4_awburst(2'h0),
 .s_axi4_awlock(1'h0),
 .s_axi4_awcache(4'h0),
 .s_axi4_awprot(3'h0),
 .s_axi4_awvalid(1'h0),
 .s_axi4_wdata(32'h0),
 .s_axi4_wstrb(4'h0),
 .s_axi4_wlast(1'h0),
 .s_axi4_wvalid(1'h0),
 .s_axi4_bready(1'h0),
 .s_axi4_arid(4'h0),
 .s_axi4_araddr(32'h0),
 .s_axi4_arlen(8'h0),
 .s_axi4_arsize(3'h0),
 .s_axi4_arburst(2'h0),
 .s_axi4_arlock(1'h0),
 .s_axi4_arcache(4'h0),
 .s_axi4_arprot(3'h0),
 .s_axi4_arvalid(1'h0),
 .s_axi4_rready(1'h0),
 .mm2s_prmry_reset_out_n(),                     // output wire mm2s_prmry_reset_out_n
 .axi_str_txd_tvalid(fifo_axis_bus.txd_tvalid),          // output wire axi_str_txd_tvalid
 .axi_str_txd_tready(fifo_axis_bus.txd_tready),          // input wire axi_str_txd_tready
 .axi_str_txd_tlast(fifo_axis_bus.txd_tlast),            // output wire axi_str_txd_tlast
 .axi_str_txd_tdata(fifo_axis_bus.txd_tdata),            // output wire [31 : 0] axi_str_txd_tdata
 .axi_str_txc_tready(1'h0),
 .s2mm_prmry_reset_out_n(),                     // output wire s2mm_prmry_reset_out_n
 .axi_str_rxd_tvalid(fifo_axis_bus.rxd_tvalid),          // input wire axi_str_rxd_tvalid
 .axi_str_rxd_tready(fifo_axis_bus.rxd_tready),          // output wire axi_str_rxd_tready
 .axi_str_rxd_tlast(fifo_axis_bus.rxd_tlast),            // input wire axi_str_rxd_tlast
 .axi_str_rxd_tkeep(4'h0),
 .axi_str_rxd_tdata(fifo_axis_bus.rxd_tdata),            // input wire [31 : 0] axi_str_rxd_tdata
 .axi_str_rxd_tstrb(4'h0),
 .axi_str_rxd_tdest(4'h0),
 .axi_str_rxd_tid(4'h0),
 .axi_str_rxd_tuser(4'h0)
);

axis_dwidth_converter_v1_1_16_axis_dwidth_converter #(
  .C_FAMILY(FPGA_VERSION),
  .C_S_AXIS_TDATA_WIDTH(32),
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
  .s_axis_tstrb(4'HF),
  .s_axis_tkeep(4'HF),
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

assign adpt_master_v = axis_fsb_bus.rxd_tvalid;
assign adpt_master_data = axis_fsb_bus.rxd_tdata[79:0];
// assign axis_fsb_bus.rxd_tlast;
assign axis_fsb_bus.rxd_tready = adpt_master_r;

//  ||
//  \/
// FSB MODULE
//  ||
//  \/

assign axis_fsb_bus.txd_tvalid = adpt_slave_v;
assign axis_fsb_bus.txd_tdata = {48'h0000_0000_0000, adpt_slave_data};
assign axis_fsb_bus.txd_tlast = adpt_slave_v & axis_fsb_bus.txd_tready;

assign adpt_slave_r = axis_fsb_bus.txd_tready;


axis_dwidth_converter_v1_1_16_axis_dwidth_converter #(
  .C_FAMILY(FPGA_VERSION),
  .C_S_AXIS_TDATA_WIDTH(128),
  .C_M_AXIS_TDATA_WIDTH(32),
  .C_AXIS_TID_WIDTH(1),
  .C_AXIS_TDEST_WIDTH(1),
  .C_S_AXIS_TUSER_WIDTH(1),
  .C_M_AXIS_TUSER_WIDTH(1),
  .C_AXIS_SIGNAL_SET('B00000000000000000000000000010011)
) axis_128_32 (
  .aclk(clk_i),
  .aresetn(resetn_i),
  .aclken(1'H1),
  .s_axis_tvalid(axis_fsb_bus.txd_tvalid),
  .s_axis_tready(axis_fsb_bus.txd_tready),
  .s_axis_tdata(axis_fsb_bus.txd_tdata),
  .s_axis_tstrb(16'HFFFF),
  .s_axis_tkeep(16'HFFFF),
  .s_axis_tlast(axis_fsb_bus.txd_tlast),
  .s_axis_tid(1'H0),
  .s_axis_tdest(1'H0),
  .s_axis_tuser(1'H0),
  .m_axis_tvalid(fifo_axis_bus.rxd_tvalid),
  .m_axis_tready(fifo_axis_bus.rxd_tready),
  .m_axis_tdata(fifo_axis_bus.rxd_tdata),
  .m_axis_tstrb(),
  .m_axis_tkeep(),
  .m_axis_tlast(fifo_axis_bus.rxd_tlast),
  .m_axis_tid(),
  .m_axis_tdest(),
  .m_axis_tuser()
);

endmodule
