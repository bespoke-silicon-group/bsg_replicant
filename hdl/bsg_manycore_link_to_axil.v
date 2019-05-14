/**
*  bsg_manycore_link_to_axil.v
*
*/

`include "bsg_defines.v"
`include "axil_to_mcl.vh"
`include "bsg_axi_bus_pkg.vh"
`include "bsg_manycore_packet.vh"
`include "bsg_bladerunner_rom_pkg.vh"

module bsg_manycore_link_to_axil #(
  // axil parameters
  parameter axil_base_addr_p = 32'h0000_0000
  , localparam axil_mosi_bus_width_lp = `bsg_axil_mosi_bus_width(1)
  , localparam axil_miso_bus_width_lp = `bsg_axil_miso_bus_width(1)
  , localparam axil_addr_width_lp = 32
  , localparam axil_data_width_lp = 32
  , localparam mc_data_width_lp = 128
  , localparam axil_fifo_els_lp = 256
  , localparam num_endpoint_lp = 1
  // endpoint parameters
  , parameter x_cord_width_p="inv"
  , parameter y_cord_width_p="inv"
  , parameter fifo_els_p = 4
  , parameter addr_width_p = "inv"
  , parameter data_width_p = "inv"
  , parameter max_out_credits_p = "inv"
  , parameter load_id_width_p = "inv"
  , localparam credits_width_lp = $clog2(max_out_credits_p+1)
  , localparam link_sif_width_lp = `bsg_manycore_link_sif_width(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p,load_id_width_p)
  , localparam packet_width_lp = `bsg_manycore_packet_width(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p,load_id_width_p)
  , localparam return_packet_width_lp =`bsg_manycore_return_packet_width(x_cord_width_p,y_cord_width_p,data_width_p,load_id_width_p)
) (
  input                               clk_i
  ,input                               reset_i
  ,input  [axil_mosi_bus_width_lp-1:0] s_axil_mcl_bus_i
  ,output [axil_miso_bus_width_lp-1:0] s_axil_mcl_bus_o
  ,input  [     link_sif_width_lp-1:0] link_sif_i
  ,output [     link_sif_width_lp-1:0] link_sif_o
  ,input  [        x_cord_width_p-1:0] my_x_i
  ,input  [        y_cord_width_p-1:0] my_y_i
);

  localparam num_slots_lp = num_endpoint_lp*2;

  // monitor signals
  logic [num_endpoint_lp-1:0][`BSG_WIDTH(max_out_credits_p)-1:0] mc_out_credits_lo;
  logic [   num_slots_lp-1:0][`BSG_WIDTH(max_out_credits_p)-1:0] rcv_vacancy_lo   ;
  logic [num_slots_lp-1:0][31:0] rcv_vacancy_lo_cast;

  logic [num_endpoint_lp*2-1:0]                       mc_fifo_v_li   ;
  logic [num_endpoint_lp*2-1:0][mc_data_width_lp-1:0] mc_fifo_data_li;
  logic [num_endpoint_lp*2-1:0]                       mc_fifo_rdy_lo ;
  logic [num_endpoint_lp*2-1:0]                       mc_fifo_v_lo   ;
  logic [num_endpoint_lp*2-1:0][mc_data_width_lp-1:0] mc_fifo_data_lo;
  logic [num_endpoint_lp*2-1:0]                       mc_fifo_rdy_li ;

  bsg_manycore_endpoint_to_fifos #(
    .num_endpoint_p   (num_endpoint_lp  )
    ,.x_cord_width_p   (x_cord_width_p   )
    ,.y_cord_width_p   (y_cord_width_p   )
    ,.fifo_els_p       (fifo_els_p       )
    ,.addr_width_p     (addr_width_p     )
    ,.data_width_p     (data_width_p     )
    ,.max_out_credits_p(max_out_credits_p)
    ,.load_id_width_p  (load_id_width_p  )
  ) mc_endpoint_to_fifos (
    .clk_i        (clk_i            )
    ,.reset_i      (reset_i          )
    ,.fifo_v_i     (mc_fifo_v_li     )
    ,.fifo_data_i  (mc_fifo_data_li  )
    ,.fifo_rdy_o   (mc_fifo_rdy_lo   )
    ,.fifo_v_o     (mc_fifo_v_lo     )
    ,.fifo_data_o  (mc_fifo_data_lo  )
    ,.fifo_rdy_i   (mc_fifo_rdy_li   )
    ,.link_sif_i   (link_sif_i       )
    ,.link_sif_o   (link_sif_o       )
    ,.my_x_i       (my_x_i           )
    ,.my_y_i       (my_y_i           )
    ,.out_credits_o(mc_out_credits_lo)
  );

  assign s_axil_mcl_bus_o = '0;


  // receive fifo to axil_to_fifos
  logic [num_slots_lp-1:0]                         axil_fifo_v_li   ;
  logic [num_slots_lp-1:0][axil_data_width_lp-1:0] axil_fifo_data_li;
  logic [num_slots_lp-1:0]                         axil_fifo_rdy_lo ;
  logic [num_slots_lp-1:0]                         axil_fifo_v_lo   ;
  logic [num_slots_lp-1:0][axil_data_width_lp-1:0] axil_fifo_data_lo;
  logic [num_slots_lp-1:0]                         axil_fifo_rdy_li ;

  logic [axil_addr_width_lp-1:0] rom_addr_li;
  logic [axil_data_width_lp-1:0] rom_data_lo;

  bsg_axil_to_fifos #(
    .num_slots_p     (num_slots_lp    )
    ,.fifo_els_p      (axil_fifo_els_lp)
    ,.axil_base_addr_p(axil_base_addr_p)
  ) axil_to_fifos (
    .clk_i           (clk_i            )
    ,.reset_i         (reset_i          )
    ,.s_axil_bus_i    ( '0)
    ,.s_axil_bus_o    ( )
    ,.fifo_v_i        (axil_fifo_v_li   )
    ,.fifo_data_i     (axil_fifo_data_li)
    ,.fifo_rdy_o      (axil_fifo_rdy_lo )
    ,.fifo_v_o        (axil_fifo_v_lo   )
    ,.fifo_data_o     (axil_fifo_data_lo)
    ,.fifo_rdy_i      (axil_fifo_rdy_li )
    ,.rom_addr_o      (rom_addr_li      )
    ,.rom_data_i      (rom_data_lo      )
    ,.rcv_vacancy_i   (rcv_vacancy_lo_cast)
    ,.mc_out_credits_i(32'(mc_out_credits_lo))
  );

  localparam lg_rom_els_lp = `BSG_SAFE_CLOG2(rom_els_p);

  bsg_bladerunner_configuration #(
    .width_p     (axil_data_width_lp)
    ,.addr_width_p(lg_rom_els_lp     )
  ) configuration_rom (
    .addr_i(rom_addr_li[2+:lg_rom_els_lp])
    ,.data_o(rom_data_lo                  )
  );


  // from receive fifo
  logic [num_slots_lp-1:0]                       rcv_fifo_v_lo;
  logic [num_slots_lp-1:0]                       rcv_fifo_r_li;
  logic [num_slots_lp-1:0][mc_data_width_lp-1:0] rcv_fifo_lo  ;

  wire [num_slots_lp-1:0] rcv_enqueue = mc_fifo_v_lo & mc_fifo_rdy_li;
  wire [num_slots_lp-1:0] rcv_dequeue = rcv_fifo_r_li & rcv_fifo_v_lo;

  wire [num_slots_lp-1:0] par_to_ser_yumi_li = axil_fifo_rdy_lo & axil_fifo_v_li;

  for (genvar i=0; i<num_slots_lp; i++) begin : mc128_to_fifo32
    bsg_counter_up_down #(
      .max_val_p (max_out_credits_p)
      ,.init_val_p(max_out_credits_p)
      ,.max_step_p(1                )
    ) rcv_vacancy_cnt (
      .clk_i  (clk_i            )
      ,.reset_i(reset_i          )
      ,.down_i (rcv_enqueue[i]   )
      ,.up_i   (rcv_dequeue[i]   )
      ,.count_o(rcv_vacancy_lo[i])
    );
    assign rcv_vacancy_lo_cast[i] = 32'(rcv_vacancy_lo[i]);

    bsg_fifo_1r1w_small #(
      .width_p           (mc_data_width_lp),
      .els_p             (axil_fifo_els_lp),
      .ready_THEN_valid_p(0               )  // for input
    ) rcv_fifo (
      .clk_i  (clk_i             )
      ,.reset_i(reset_i           )
      ,.v_i    (mc_fifo_v_lo[i]   )
      ,.ready_o(mc_fifo_rdy_li[i] )
      ,.data_i (mc_fifo_data_lo[i])
      ,.v_o    (rcv_fifo_v_lo[i]  )
      ,.data_o (rcv_fifo_lo[i]    )
      ,.yumi_i (rcv_dequeue[i]    )
    );

    bsg_parallel_in_serial_out #(
      .width_p(axil_data_width_lp                 ),
      .els_p  (mc_data_width_lp/axil_data_width_lp)
    ) data_downsizer (
      .clk_i  (clk_i                )
      ,.reset_i(reset_i              )
      ,.valid_i(rcv_fifo_v_lo[i]     )
      ,.data_i (rcv_fifo_lo[i]       )
      ,.ready_o(rcv_fifo_r_li[i]     )
      ,.valid_o(axil_fifo_v_li[i]    )
      ,.data_o (axil_fifo_data_li[i] )
      ,.yumi_i (par_to_ser_yumi_li[i])
    );
  end


  localparam valid_width_lp    = mc_data_width_lp/axil_data_width_lp;
  localparam yumi_cnt_width_lp = $clog2(valid_width_lp+1)           ;

  logic [num_slots_lp-1:0][   valid_width_lp-1:0] ser_to_par_valid_lo   ;
  logic [num_slots_lp-1:0][yumi_cnt_width_lp-1:0] ser_to_par_yumi_cnt_li;

  for (genvar i=0; i<num_slots_lp; i++) begin : fifo32_to_mc128
    bsg_serial_in_parallel_out #(
      .width_p(axil_data_width_lp                 ),
      .els_p  (mc_data_width_lp/axil_data_width_lp)
    ) data_deserialize (
      .clk_i     (clk_i                 )
      ,.reset_i   (reset_i               )
      ,.valid_i   (axil_fifo_v_lo[i]     )
      ,.data_i    (axil_fifo_data_lo[i]  )
      ,.ready_o   (axil_fifo_rdy_li[i]   )
      ,.valid_o   (ser_to_par_valid_lo[i])
      ,.data_o    (mc_fifo_data_li[i]    )
      ,.yumi_cnt_i(ser_to_par_yumi_cnt_li[i])
    );

    assign mc_fifo_v_li[i]           = &ser_to_par_valid_lo[i];
    assign ser_to_par_yumi_cnt_li[i] = (mc_fifo_v_li[i] & mc_fifo_rdy_lo[i]) ? (yumi_cnt_width_lp)'(valid_width_lp) :'0;
  end

endmodule
