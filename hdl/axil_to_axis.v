/**
 *  axil_to_axis.v
 *
 *  axi-lite (SH) <-> axis interface (CL)
 */

`include "bsg_axi_bus_pkg.vh"
`include "bsg_bladerunner_rom_pkg.vh"
 import bsg_bladerunner_rom_pkg::*;

module axil_to_axis #(
   mcl_width_p = "inv"
  ,max_out_credits_p = "inv"
  ,axil_mosi_bus_width_lp = `bsg_axil_mosi_bus_width(1)
  ,axil_miso_bus_width_lp = `bsg_axil_miso_bus_width(1)
)(
  input clk_i
  ,input reset_i
  ,input [axil_mosi_bus_width_lp-1:0] s_axil_mcl_bus_i
  ,output [axil_miso_bus_width_lp-1:0] s_axil_mcl_bus_o
  ,input mcl_v_i
  ,input [mcl_width_p-1:0] mcl_data_i
  ,output mcl_r_o
  ,output mcl_v_o
  ,output [mcl_width_p-1:0] mcl_data_o
  ,input mcl_r_i
  ,output logic [$clog2(max_out_credits_p+1)-1:0] rcv_vacancy_o
);

localparam fpga_version_p = "virtexuplus";

`declare_bsg_axil_bus_s(1, bsg_axil_mosi_bus_s, bsg_axil_miso_bus_s);

bsg_axil_mosi_bus_s s_axil_mcl_bus_i_cast;
bsg_axil_miso_bus_s s_axil_mcl_bus_o_cast;
assign s_axil_mcl_bus_i_cast = s_axil_mcl_bus_i;
assign s_axil_mcl_bus_o      = s_axil_mcl_bus_o_cast;

//// flop the input OCL bus
////---------------------------------
//axi_register_slice_light AXIL_OCL_REG_SLC (
//  .aclk         (clk_i                        ),
//  .aresetn      (~reset_i                     ),
//  .s_axi_awaddr (s_axil_mcl_bus_i_cast.awaddr ),
//  .s_axi_awprot (3'h0                         ),
//  .s_axi_awvalid(s_axil_mcl_bus_i_cast.awvalid),
//  .s_axi_awready(s_axil_mcl_bus_o_cast.awready),
//  .s_axi_wdata  (s_axil_mcl_bus_i_cast.wdata  ),
//  .s_axi_wstrb  (s_axil_mcl_bus_i_cast.wstrb  ),
//  .s_axi_wvalid (s_axil_mcl_bus_i_cast.wvalid ),
//  .s_axi_wready (s_axil_mcl_bus_o_cast.wready ),
//  .s_axi_bresp  (s_axil_mcl_bus_o_cast.bresp  ),
//  .s_axi_bvalid (s_axil_mcl_bus_o_cast.bvalid ),
//  .s_axi_bready (s_axil_mcl_bus_i_cast.bready ),
//  .s_axi_araddr (s_axil_mcl_bus_i_cast.araddr ),
//  .s_axi_arvalid(s_axil_mcl_bus_i_cast.arvalid),
//  .s_axi_arready(s_axil_mcl_bus_o_cast.arready),
//  .s_axi_rdata  (s_axil_mcl_bus_o_cast.rdata  ),
//  .s_axi_rresp  (s_axil_mcl_bus_o_cast.rresp  ),
//  .s_axi_rvalid (s_axil_mcl_bus_o_cast.rvalid ),
//  .s_axi_rready (s_axil_mcl_bus_i_cast.rready ),
//
//  .m_axi_awaddr (s_axil_mcl_mosi_r.awaddr     ),
//  .m_axi_awprot (                             ),
//  .m_axi_awvalid(s_axil_mcl_mosi_r.awvalid    ),
//  .m_axi_awready(s_axil_mcl_miso_r.awready    ),
//  .m_axi_wdata  (s_axil_mcl_mosi_r.wdata      ),
//  .m_axi_wstrb  (s_axil_mcl_mosi_r.wstrb      ),
//  .m_axi_wvalid (s_axil_mcl_mosi_r.wvalid     ),
//  .m_axi_wready (s_axil_mcl_miso_r.wready     ),
//  .m_axi_bresp  (s_axil_mcl_miso_r.bresp      ),
//  .m_axi_bvalid (s_axil_mcl_miso_r.bvalid     ),
//  .m_axi_bready (s_axil_mcl_mosi_r.bready     ),
//  .m_axi_araddr (s_axil_mcl_mosi_r.araddr     ),
//  .m_axi_arvalid(s_axil_mcl_mosi_r.arvalid    ),
//  .m_axi_arready(s_axil_mcl_miso_r.arready    ),
//  .m_axi_rdata  (s_axil_mcl_miso_r.rdata      ),
//  .m_axi_rresp  (s_axil_mcl_miso_r.rresp      ),
//  .m_axi_rvalid (s_axil_mcl_miso_r.rvalid     ),
//  .m_axi_rready (s_axil_mcl_mosi_r.rready     )
//);
//

`declare_bsg_axis_bus_s(32, bsg_axis32_mosi_bus_s, bsg_axis32_miso_bus_s);
`declare_bsg_axis_bus_s(mcl_width_p, bsg_axisN_mosi_bus_s, bsg_axisN_miso_bus_s);

bsg_axis32_mosi_bus_s mosi_axis32_bus;
bsg_axis32_miso_bus_s miso_axis32_bus;
bsg_axisN_mosi_bus_s  mosi_axisN_bus ;
bsg_axisN_miso_bus_s  miso_axisN_bus ;

// `ifdef LOCAL_FPGA
// axi_fifo_mm_s_0 axi_fifo_mm_s_axi_lite (
//   .interrupt(),                            // output wire interrupt
//   .s_axi_aclk(clk_i),                          // input wire s_axi_aclk
//   .s_axi_aresetn(~reset_i),                    // input wire s_axi_aresetn
//   .s_axi_awaddr(s_axil_mcl_mosi_r.awaddr),                      // input wire [31 : 0] s_axi_awaddr
//   .s_axi_awvalid(s_axil_mcl_mosi_r.awvalid),                    // input wire s_axi_awvalid
//   .s_axi_awready(s_axil_mcl_miso_r.awready),                    // output wire s_axi_awready
//   .s_axi_wdata(s_axil_mcl_mosi_r.wdata),                        // input wire [31 : 0] s_axi_wdata
//   .s_axi_wstrb(s_axil_mcl_mosi_r.wstrb),                        // input wire [3 : 0] s_axi_wstrb
//   .s_axi_wvalid(s_axil_mcl_mosi_r.wvalid),                      // input wire s_axi_wvalid
//   .s_axi_wready(s_axil_mcl_miso_r.wready),                      // output wire s_axi_wready
//   .s_axi_bresp(s_axil_mcl_miso_r.bresp),                        // output wire [1 : 0] s_axi_bresp
//   .s_axi_bvalid(s_axil_mcl_miso_r.bvalid),                      // output wire s_axi_bvalid
//   .s_axi_bready(s_axil_mcl_mosi_r.bready),                      // input wire s_axi_bready
//   .s_axi_araddr(s_axil_mcl_mosi_r.araddr),                      // input wire [31 : 0] s_axi_araddr
//   .s_axi_arvalid(s_axil_mcl_mosi_r.arvalid),                    // input wire s_axi_arvalid
//   .s_axi_arready(s_axil_mcl_miso_r.arready),                    // output wire s_axi_arready
//   .s_axi_rdata(s_axil_mcl_miso_r.rdata),                        // output wire [31 : 0] s_axi_rdata
//   .s_axi_rresp(s_axil_mcl_miso_r.rresp),                        // output wire [1 : 0] s_axi_rresp
//   .s_axi_rvalid(s_axil_mcl_miso_r.rvalid),                      // output wire s_axi_rvalid
//   .s_axi_rready(s_axil_mcl_mosi_r.rready),                      // input wire s_axi_rready
//   .mm2s_prmry_reset_out_n(),  // output wire mm2s_prmry_reset_out_n
//   .axi_str_txd_tvalid(mosi_axis32_bus.txd_tvalid),          // output wire axi_str_txd_tvalid
//   .axi_str_txd_tready(miso_axis32_bus.txd_tready),          // input wire axi_str_txd_tready
//   .axi_str_txd_tlast(mosi_axis32_bus.txd_tlast),            // output wire axi_str_txd_tlast
//   .axi_str_txd_tdata(mosi_axis32_bus.txd_tdata),            // output wire [31 : 0] axi_str_txd_tdata
//   .mm2s_cntrl_reset_out_n(),  // output wire mm2s_cntrl_reset_out_n
//   .axi_str_txc_tvalid(),          // output wire axi_str_txc_tvalid
//   .axi_str_txc_tready(1),          // input wire axi_str_txc_tready
//   .axi_str_txc_tlast(),            // output wire axi_str_txc_tlast
//   .axi_str_txc_tdata(),            // output wire [31 : 0] axi_str_txc_tdata
//   .s2mm_prmry_reset_out_n(),  // output wire s2mm_prmry_reset_out_n
//   .axi_str_rxd_tvalid(miso_axis32_bus.rxd_tvalid),          // input wire axi_str_rxd_tvalid
//   .axi_str_rxd_tready(mosi_axis32_bus.rxd_tready),          // output wire axi_str_rxd_tready
//   .axi_str_rxd_tlast(miso_axis32_bus.rxd_tlast ),            // input wire axi_str_rxd_tlast
//   .axi_str_rxd_tdata(miso_axis32_bus.rxd_tdata)            // input wire [31 : 0] axi_str_rxd_tdata
// );
// `else

`ifdef USE_XILINX_IP
// convert axil to axis
//---------------------------------
axi_fifo_mm_s #(
 .C_FAMILY              (fpga_version_p),
 .C_S_AXI_ID_WIDTH      (4             ),
 .C_S_AXI_ADDR_WIDTH    (32            ),
 .C_S_AXI_DATA_WIDTH    (32            ),
 .C_S_AXI4_DATA_WIDTH   (32            ),
 .C_TX_FIFO_DEPTH       (512           ),
 .C_RX_FIFO_DEPTH       (512           ),
 .C_TX_FIFO_PF_THRESHOLD(507           ),
 .C_TX_FIFO_PE_THRESHOLD(2             ),
 .C_RX_FIFO_PF_THRESHOLD(507           ),
 .C_RX_FIFO_PE_THRESHOLD(2             ),
 .C_USE_TX_CUT_THROUGH  (0             ),
 .C_DATA_INTERFACE_TYPE (0             ),
 .C_BASEADDR            (32'h80000000  ),
 .C_HIGHADDR            (32'h80000FFF  ),
 .C_AXI4_BASEADDR       (32'h80001000  ),
 .C_AXI4_HIGHADDR       (32'h80002FFF  ),
 .C_HAS_AXIS_TID        (0             ),
 .C_HAS_AXIS_TDEST      (0             ),
 .C_HAS_AXIS_TUSER      (0             ),
 .C_HAS_AXIS_TSTRB      (0             ),
 .C_HAS_AXIS_TKEEP      (0             ),
 .C_AXIS_TID_WIDTH      (4             ),
 .C_AXIS_TDEST_WIDTH    (4             ),
 .C_AXIS_TUSER_WIDTH    (4             ),
 .C_USE_RX_CUT_THROUGH  (0             ),
 .C_USE_TX_DATA         (1             ),
 .C_USE_TX_CTRL         (0             ),
 .C_USE_RX_DATA         (1             )
) axi_fifo_mm_s_axi_lite (
 .interrupt             (                          ), // output wire interrupt
 .s_axi_aclk            (clk_i                     ), // input wire s_axi_aclk
 .s_axi_aresetn         (~reset_i                  ), // input wire s_axi_aresetn
 .s_axi_awaddr          (s_axil_mcl_mosi_r.awaddr  ), // input wire [31 : 0] s_axi_awaddr
 .s_axi_awvalid         (s_axil_mcl_mosi_r.awvalid ), // input wire s_axi_awvalid
 .s_axi_awready         (s_axil_mcl_miso_r.awready ), // output wire s_axi_awready
 .s_axi_wdata           (s_axil_mcl_mosi_r.wdata   ), // input wire [31 : 0] s_axi_wdata
 .s_axi_wstrb           (s_axil_mcl_mosi_r.wstrb   ), // input wire [3 : 0] s_axi_wstrb
 .s_axi_wvalid          (s_axil_mcl_mosi_r.wvalid  ), // input wire s_axi_wvalid
 .s_axi_wready          (s_axil_mcl_miso_r.wready  ), // output wire s_axi_wready
 .s_axi_bresp           (s_axil_mcl_miso_r.bresp   ), // output wire [1 : 0] s_axi_bresp
 .s_axi_bvalid          (s_axil_mcl_miso_r.bvalid  ), // output wire s_axi_bvalid
 .s_axi_bready          (s_axil_mcl_mosi_r.bready  ), // input wire s_axi_bready
 .s_axi_araddr          (s_axil_mcl_mosi_r.araddr  ), // input wire [31 : 0] s_axi_araddr
 .s_axi_arvalid         (s_axil_mcl_mosi_r.arvalid ), // input wire s_axi_arvalid
 .s_axi_arready         (s_axil_mcl_miso_r.arready ), // output wire s_axi_arready
 .s_axi_rdata           (s_axil_mcl_miso_r.rdata   ), // output wire [31 : 0] s_axi_rdata
 .s_axi_rresp           (s_axil_mcl_miso_r.rresp   ), // output wire [1 : 0] s_axi_rresp
 .s_axi_rvalid          (s_axil_mcl_miso_r.rvalid  ), // output wire s_axi_rvalid
 .s_axi_rready          (s_axil_mcl_mosi_r.rready  ), // input wire s_axi_rready
 .s_axi4_awid           (4'h0                      ),
 .s_axi4_awaddr         (32'h0                     ),
 .s_axi4_awlen          (8'h0                      ),
 .s_axi4_awsize         (3'h0                      ),
 .s_axi4_awburst        (2'h0                      ),
 .s_axi4_awlock         (1'h0                      ),
 .s_axi4_awcache        (4'h0                      ),
 .s_axi4_awprot         (3'h0                      ),
 .s_axi4_awvalid        (1'h0                      ),
 .s_axi4_wdata          (32'h0                     ),
 .s_axi4_wstrb          (4'h0                      ),
 .s_axi4_wlast          (1'h0                      ),
 .s_axi4_wvalid         (1'h0                      ),
 .s_axi4_bready         (1'h0                      ),
 .s_axi4_arid           (4'h0                      ),
 .s_axi4_araddr         (32'h0                     ),
 .s_axi4_arlen          (8'h0                      ),
 .s_axi4_arsize         (3'h0                      ),
 .s_axi4_arburst        (2'h0                      ),
 .s_axi4_arlock         (1'h0                      ),
 .s_axi4_arcache        (4'h0                      ),
 .s_axi4_arprot         (3'h0                      ),
 .s_axi4_arvalid        (1'h0                      ),
 .s_axi4_rready         (1'h0                      ),
 .mm2s_prmry_reset_out_n(                          ), // output wire mm2s_prmry_reset_out_n
 .axi_str_txd_tvalid    (mosi_axis32_bus.txd_tvalid), // output wire axi_str_txd_tvalid
 .axi_str_txd_tready    (miso_axis32_bus.txd_tready), // input wire axi_str_txd_tready
 .axi_str_txd_tlast     (mosi_axis32_bus.txd_tlast ), // output wire axi_str_txd_tlast
 .axi_str_txd_tdata     (mosi_axis32_bus.txd_tdata ), // output wire [31 : 0] axi_str_txd_tdata
 .axi_str_txc_tready    (1'h0                      ),
 .s2mm_prmry_reset_out_n(                          ), // output wire s2mm_prmry_reset_out_n
 .axi_str_rxd_tvalid    (miso_axis32_bus.rxd_tvalid), // input wire axi_str_rxd_tvalid
 .axi_str_rxd_tready    (mosi_axis32_bus.rxd_tready), // output wire axi_str_rxd_tready
 .axi_str_rxd_tlast     (miso_axis32_bus.rxd_tlast ), // input wire axi_str_rxd_tlast
 .axi_str_rxd_tkeep     (4'h0                      ),
 .axi_str_rxd_tdata     (miso_axis32_bus.rxd_tdata ), // input wire [31 : 0] axi_str_rxd_tdata
 .axi_str_rxd_tstrb     (4'h0                      ),
 .axi_str_rxd_tdest     (4'h0                      ),
 .axi_str_rxd_tid       (4'h0                      ),
 .axi_str_rxd_tuser     (4'h0                      )
);
`else

	bsg_axil_to_fifos #(
		.num_2fifos_p(1)
		,.fifo_els_p(256)
	) axil_to_fifos (
		.*
		,.s_axil_bus_i(s_axil_mcl_bus_i_cast)
		,.s_axil_bus_o(s_axil_mcl_bus_o_cast)
		,.fifo_v_i(miso_axis32_bus.rxd_tvalid)
		,.fifo_data_i(miso_axis32_bus.rxd_tdata)
		,.fifo_rdy_o(mosi_axis32_bus.rxd_tready)
		,.fifo_v_o(mosi_axis32_bus.txd_tvalid)
		,.fifo_data_o(mosi_axis32_bus.txd_tdata)
		,.fifo_rdy_i(miso_axis32_bus.txd_tready)
	);
`endif

	assign mosi_axis32_bus.txd_tlast = 1'b1;

axis_dwidth_converter_v1_1_16_axis_dwidth_converter #(
  .C_FAMILY(fpga_version_p),
  .C_S_AXIS_TDATA_WIDTH(32),
  .C_M_AXIS_TDATA_WIDTH(mcl_width_p),
  .C_AXIS_TID_WIDTH(1),
  .C_AXIS_TDEST_WIDTH(1),
  .C_S_AXIS_TUSER_WIDTH(1),
  .C_M_AXIS_TUSER_WIDTH(1),
  .C_AXIS_SIGNAL_SET('B00000000000000000000000000010011)
) axis_32_128 (
  .aclk(clk_i),
  .aresetn(~reset_i),
  .aclken(1'H1),
  .s_axis_tvalid(mosi_axis32_bus.txd_tvalid),
  .s_axis_tready(miso_axis32_bus.txd_tready),
  .s_axis_tdata(mosi_axis32_bus.txd_tdata),
  .s_axis_tstrb(4'HF),
  .s_axis_tkeep(4'HF),
  .s_axis_tlast(mosi_axis32_bus.txd_tlast),
  .s_axis_tid(1'H0),
  .s_axis_tdest(1'H0),
  .s_axis_tuser(1'H0),
  .m_axis_tvalid(mosi_axisN_bus.txd_tvalid),  // ->
  .m_axis_tready(miso_axisN_bus.txd_tready),  // <-
  .m_axis_tdata(mosi_axisN_bus.txd_tdata),    // ->
  .m_axis_tstrb(),
  .m_axis_tkeep(mosi_axisN_bus.txd_tkeep),    // ->
  .m_axis_tlast(mosi_axisN_bus.txd_tlast),    // -> not used
  .m_axis_tid(),
  .m_axis_tdest(),
  .m_axis_tuser()
);

assign mcl_v_o                   = mosi_axisN_bus.txd_tvalid;
assign mcl_data_o                = mosi_axisN_bus.txd_tdata[mcl_width_p-1:0];
assign miso_axisN_bus.txd_tready = mcl_r_i;

// txd to mcl
//  ||
//  \/
// MCL MODULE
//  ||
//  \/
// rcv from mcl

assign miso_axisN_bus.rxd_tvalid = mcl_v_i;
assign miso_axisN_bus.rxd_tdata  = mcl_data_i;

wire [mcl_width_p-1:0] rcv_fifo_li   = miso_axisN_bus.rxd_tdata ;
wire                   rcv_fifo_r_lo                            ;
wire                   rcv_fifo_v_li = miso_axisN_bus.rxd_tvalid;

assign mosi_axisN_bus.rxd_tready = rcv_fifo_r_lo;
assign miso_axisN_bus.rxd_tlast  = mcl_v_i & mosi_axisN_bus.rxd_tready;
assign mcl_r_o                   = mosi_axisN_bus.rxd_tready;

wire [mcl_width_p-1:0] rcv_fifo_lo;
wire                   rcv_fifo_v_lo;
wire                   rcv_fifo_r_li;
wire                   rcv_fifo_yumi_li = rcv_fifo_r_li & rcv_fifo_v_lo;
wire                   rcv_fifo_last_li = rcv_fifo_yumi_li;

wire rd_fifo_enqueue = rcv_fifo_v_li & rcv_fifo_r_lo;
wire rd_fifo_dequeue = rcv_fifo_yumi_li;

bsg_counter_up_down #(
  .max_val_p (max_out_credits_p)
  ,.init_val_p(max_out_credits_p)
  ,.max_step_p(1                )
) out_credit_ctr (
  .clk_i  (clk_i          )
  ,.reset_i(reset_i        )
  ,.down_i (rd_fifo_enqueue)
  ,.up_i   (rd_fifo_dequeue) // launch remote store
  ,.count_o(rcv_vacancy_o  ) // receive credit back
);

bsg_fifo_1r1w_small #(
  .width_p           (mcl_width_p),
  .els_p             (256        ),
  .ready_THEN_valid_p(0          )
) rcv_fifo_converter (
  .clk_i  (clk_i           ),
  .reset_i(reset_i         ),
  .v_i    (rcv_fifo_v_li   ),
  .ready_o(rcv_fifo_r_lo   ),
  .data_i (rcv_fifo_li     ),
  .v_o    (rcv_fifo_v_lo   ),
  .data_o (rcv_fifo_lo     ),
  .yumi_i (rcv_fifo_yumi_li)
);

axis_dwidth_converter_v1_1_16_axis_dwidth_converter #(
  .C_FAMILY            (fpga_version_p                    ),
  .C_S_AXIS_TDATA_WIDTH(mcl_width_p                       ),
  .C_M_AXIS_TDATA_WIDTH(32                                ),
  .C_AXIS_TID_WIDTH    (1                                 ),
  .C_AXIS_TDEST_WIDTH  (1                                 ),
  .C_S_AXIS_TUSER_WIDTH(1                                 ),
  .C_M_AXIS_TUSER_WIDTH(1                                 ),
  .C_AXIS_SIGNAL_SET   ('B00000000000000000000000000010011)
) axis_128_32 (
  .aclk         (clk_i                     ),
  .aresetn      (~reset_i                  ),
  .aclken       (1'H1                      ),
  .s_axis_tvalid(rcv_fifo_v_lo             ),
  .s_axis_tready(rcv_fifo_r_li             ),
  .s_axis_tdata (rcv_fifo_lo               ),
  .s_axis_tstrb (16'HFFFF                  ),
  .s_axis_tkeep (16'HFFFF                  ),
  .s_axis_tlast (rcv_fifo_last_li          ), // miso_axisN_bus.rxd_tlast
  .s_axis_tid   (1'H0                      ),
  .s_axis_tdest (1'H0                      ),
  .s_axis_tuser (1'H0                      ),
  .m_axis_tvalid(miso_axis32_bus.rxd_tvalid),
  .m_axis_tready(mosi_axis32_bus.rxd_tready),
  .m_axis_tdata (miso_axis32_bus.rxd_tdata ),
  .m_axis_tstrb (                          ),
  .m_axis_tkeep (                          ),
  .m_axis_tlast (miso_axis32_bus.rxd_tlast ),
  .m_axis_tid   (                          ),
  .m_axis_tdest (                          ),
  .m_axis_tuser (                          )
);

endmodule
