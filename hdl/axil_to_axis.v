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
  ,axil_base_addr_p = "inv"
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

`declare_bsg_axis_bus_s(32, bsg_axis32_mosi_bus_s, bsg_axis32_miso_bus_s);
`declare_bsg_axis_bus_s(mcl_width_p, bsg_axisN_mosi_bus_s, bsg_axisN_miso_bus_s);

bsg_axis32_mosi_bus_s mosi_axis32_bus;
bsg_axis32_miso_bus_s miso_axis32_bus;
bsg_axisN_mosi_bus_s  mosi_axisN_bus ;
bsg_axisN_miso_bus_s  miso_axisN_bus ;

	bsg_axil_to_fifos #(
		.num_2fifos_p(1)
		,.fifo_els_p(256)
    ,.axil_base_addr_p(axil_base_addr_p)
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
    ,.addr_o()
    ,.data_i(0)
	);

	assign mosi_axis32_bus.txd_tlast = 1'b1;

logic [3:0] ser_to_par_valid_lo;
assign mosi_axisN_bus.txd_tvalid = &ser_to_par_valid_lo;
wire [$clog2(4+1)-1:0] ser_to_par_yumi_cnt_li = (mosi_axisN_bus.txd_tvalid && miso_axisN_bus.txd_tready) ? 3'd4 : 3'd0;

bsg_serial_in_parallel_out #(
  .width_p(32),
  .els_p  (4 )
) fifo_32_to_128 (  .*,
  .valid_i   (mosi_axis32_bus.txd_tvalid),
  .data_i    (mosi_axis32_bus.txd_tdata ),
  .ready_o   (miso_axis32_bus.txd_tready),
  .valid_o   (ser_to_par_valid_lo       ),
  .data_o    (mosi_axisN_bus.txd_tdata  ),
  .yumi_cnt_i(ser_to_par_yumi_cnt_li    )
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


wire par_to_ser_yumi_li = mosi_axis32_bus.rxd_tready & miso_axis32_bus.rxd_tvalid;
bsg_parallel_in_serial_out #(
  .width_p(32),
  .els_p  (4)
) fifo_128_32 (  .*,
  .valid_i(rcv_fifo_v_lo             ),
  .data_i (rcv_fifo_lo               ),
  .ready_o(rcv_fifo_r_li             ),
  .valid_o(miso_axis32_bus.rxd_tvalid),
  .data_o (miso_axis32_bus.rxd_tdata ),
  .yumi_i (par_to_ser_yumi_li        )
);


endmodule
