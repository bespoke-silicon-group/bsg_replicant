/*
*  bsg_manycore_endpoint_to_fifos.v
*
*  fifo (HOST side) <-> manycore_link_endpoint (MC side)
*
*          Rx & TX         16B           manycore
*           FIFOs     slicing logic  endpoing standard
*       _____________      ____      ________________
*  ==> |host request | -> |~~~~| -> |=out_pkt_i      | --> out_credits_o
*      |_____________|    |____|    |                |
*  ==> |host response| -> |~~~~| -> |=returning_pkt_i|
*      |_____________|    |____|    |                | ==>
*                                   |                |
*       _____________      ____     |                | <==
*  <== |mc response  | <- |~//~| <- |=returned_pkt_o | <-- rcv_fifo_th_i
*      |_____________|    |____|    |                |
*  <== |mc request   | <- |~~~~| <- |=in_pkt_o       |
*      |_____________|    |____|    |________________|
*
*
*
* Note:
* the host request is disabled if any of the following conditions are met:
* 1. manycore endpoint out credits == 0
* 2. out_pkt is load AND rcv_fifo_th_i := rcv_vacancy < max_out_credits_p == 1
*    We are not using 0 as the receive fifo vacancy threshold because of the network latency
*
*/

`include "bsg_manycore_packet.vh"
`include "cl_manycore_pkg.v"
`include "bsg_manycore_link_to_axil_pkg.v"

module bsg_manycore_endpoint_to_fifos
  import cl_manycore_pkg::*;
  import bsg_manycore_link_to_axil_pkg::*;
#(
  parameter fifo_width_p = "inv"
  // these are endpoint parameters
  , parameter x_cord_width_p = "inv"
  , parameter y_cord_width_p = "inv"
  , parameter addr_width_p = "inv"
  , parameter data_width_p = "inv"
  , parameter max_out_credits_p = "inv"
  , parameter load_id_width_p = "inv"
  , parameter link_sif_width_lp = `bsg_manycore_link_sif_width(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p,load_id_width_p)
) (
  input                                                        clk_i
  ,input                                                        reset_i
  // fifo to endpoint
  ,input  [                              1:0]                   fifo_v_i
  ,input  [                              1:0][fifo_width_p-1:0] fifo_data_i
  ,output [                              1:0]                   fifo_ready_o
  // endpoint to fifo
  ,output [                              1:0]                   fifo_v_o
  ,output [                              1:0][fifo_width_p-1:0] fifo_data_o
  ,input  [                              1:0]                   fifo_ready_i
  // manycore link
  ,input  [            link_sif_width_lp-1:0]                   link_sif_i
  ,output [            link_sif_width_lp-1:0]                   link_sif_o
  // endpoint coord
  ,input  [               x_cord_width_p-1:0]                   my_x_i
  ,input  [               y_cord_width_p-1:0]                   my_y_i
  // receive fifo reach threshold
  ,input                                                        rcv_fifo_th_i
  // endpoint out packet credits
  ,output [`BSG_WIDTH(max_out_credits_p)-1:0]                   out_credits_o
  // print stat
  ,output                                                       print_stat_v_o
  ,output [                 data_width_p-1:0]                   print_stat_tag_o
);

  localparam mc_req_op_width_p = 2; // default op_width in manycore request packet

  // fifo pair signals
  bsg_mcl_request_s  fifo_req_li, mc_req_lo;
  bsg_mcl_response_s fifo_rsp_li, mc_rsp_lo;

  logic fifo_req_v_li, fifo_req_ready_lo;
  logic fifo_rsp_v_li, fifo_rsp_ready_lo;
  logic mc_req_v_lo, mc_req_ready_li;
  logic mc_rsp_v_lo, mc_rsp_ready_li;

  // fifo request to manycore
  assign fifo_req_v_li   = fifo_v_i[0];
  assign fifo_req_li     = fifo_data_i[0];
  assign fifo_ready_o[0] = fifo_req_ready_lo;

  // fifo response to manycore
  assign fifo_rsp_v_li   = fifo_v_i[1];
  assign fifo_rsp_li     = fifo_data_i[1];
  assign fifo_ready_o[1] = fifo_rsp_ready_lo;

  // manycore response to fifo
  assign fifo_v_o[0]     = mc_rsp_v_lo;
  assign fifo_data_o[0]  = mc_rsp_lo;
  assign mc_rsp_ready_li = fifo_ready_i[0];

  // manycore request to fifo
  assign fifo_v_o[1]     = mc_req_v_lo;
  assign fifo_data_o[1]  = mc_req_lo;
  assign mc_req_ready_li = fifo_ready_i[1];


  // manycore endpoint signals
  localparam packet_width_lp = `bsg_manycore_packet_width(addr_width_p, data_width_p, x_cord_width_p, y_cord_width_p, load_id_width_p);
  localparam data_mask_width_lp = (data_width_p>>3);

  logic                         endpoint_in_v_lo   ;
  logic                         endpoint_in_yumi_li;
  logic[      data_width_p-1:0] endpoint_in_data_lo;
  logic[data_mask_width_lp-1:0] endpoint_in_mask_lo;
  logic[      addr_width_p-1:0] endpoint_in_addr_lo;
  logic                         endpoint_in_we_lo  ;
  logic[    x_cord_width_p-1:0] in_src_x_cord_lo   ;
  logic[    y_cord_width_p-1:0] in_src_y_cord_lo   ;

  logic                      endpoint_out_v_li     ;
  logic[packet_width_lp-1:0] endpoint_out_packet_li;
  logic                      endpoint_out_ready_lo ;

  logic[   data_width_p-1:0] returned_data_r_lo   ;
  logic[load_id_width_p-1:0] returned_load_id_r_lo;
  logic                      returned_v_r_lo      ;
  logic                      returned_fifo_full_lo;
  logic                      returned_yumi_li     ;

  logic[data_width_p-1:0] returning_data_li;
  logic                   returning_v_li   ;


  // host request to manycore
  // -------------------------
  wire fifo_req_enable = !( (out_credits_o == '0) || ((fifo_req_li.op==8'(`ePacketOp_remote_load)) && rcv_fifo_th_i) );

  assign endpoint_out_packet_li = {
    (addr_width_p)'(fifo_req_li.addr)
    ,(mc_req_op_width_p)'(fifo_req_li.op)
    ,data_mask_width_lp'(fifo_req_li.op_ex)
    ,(data_width_p)'(fifo_req_li.payload.data)
    ,(y_cord_width_p)'(fifo_req_li.src_y_cord)
    ,(x_cord_width_p)'(fifo_req_li.src_x_cord)
    ,(y_cord_width_p)'(fifo_req_li.y_cord)
    ,(x_cord_width_p)'(fifo_req_li.x_cord)
  };
  assign endpoint_out_v_li = fifo_req_enable & fifo_req_v_li;
  assign fifo_req_ready_lo = fifo_req_enable & endpoint_out_ready_lo;

  // host response to manycore
  // -------------------------
  logic returning_wr_v_r;
  always_ff @(posedge clk_i) begin
    if(reset_i)
      returning_wr_v_r <= '0;
    else
      returning_wr_v_r <= endpoint_in_yumi_li & endpoint_in_we_lo;
  end

  assign fifo_rsp_ready_lo = ~returning_wr_v_r;
  assign returning_data_li = returning_wr_v_r ? '0 : (data_width_p)'(fifo_rsp_li.data);
  assign returning_v_li    = returning_wr_v_r | (fifo_rsp_v_li & fifo_rsp_ready_lo);

  // manycore request to host
  // -------------------------
  assign mc_req_v_lo = endpoint_in_v_lo;
  assign endpoint_in_yumi_li = mc_req_ready_li & mc_req_v_lo;
  assign mc_req_lo.padding = '0;
  assign mc_req_lo.addr = (32)'(endpoint_in_addr_lo);
  assign mc_req_lo.op = (8)'(endpoint_in_we_lo);
  assign mc_req_lo.op_ex = (8)'(endpoint_in_mask_lo);
  assign mc_req_lo.payload.data = (32)'(endpoint_in_data_lo);
  assign mc_req_lo.src_x_cord = (8)'(in_src_x_cord_lo);
  assign mc_req_lo.src_y_cord = (8)'(in_src_y_cord_lo);
  assign mc_req_lo.y_cord = (8)'(my_y_i);
  assign mc_req_lo.x_cord = (8)'(my_x_i);

  // manycore response to host
  // -------------------------
  assign mc_rsp_v_lo = returned_v_r_lo;
  assign mc_rsp_lo.padding = '0;
  assign mc_rsp_lo.pkt_type = 8'({`ePacketType_data});  // Curly braces must be kept!
  assign mc_rsp_lo.data = 32'(returned_data_r_lo);
  assign mc_rsp_lo.load_id = 32'(returned_load_id_r_lo);
  assign mc_rsp_lo.y_cord = 8'(my_y_i);
  assign mc_rsp_lo.x_cord = 8'(my_x_i);
  assign returned_yumi_li = mc_rsp_ready_li & mc_rsp_v_lo;


  bsg_manycore_endpoint_standard #(
    .x_cord_width_p   (x_cord_width_p   )
    ,.y_cord_width_p   (y_cord_width_p   )
    ,.fifo_els_p       (4                )
    ,.addr_width_p     (addr_width_p     )
    ,.data_width_p     (data_width_p     )
    ,.max_out_credits_p(max_out_credits_p)
    ,.load_id_width_p  (load_id_width_p  )
  ) epsd (
    .clk_i               (clk_i                 )
    ,.reset_i             (reset_i               )

    ,.link_sif_i          (link_sif_i            )
    ,.link_sif_o          (link_sif_o            )

    // manycore packet -> fifo
    ,.in_v_o              (endpoint_in_v_lo      )
    ,.in_yumi_i           (endpoint_in_yumi_li   )
    ,.in_data_o           (endpoint_in_data_lo   )
    ,.in_mask_o           (endpoint_in_mask_lo   )
    ,.in_addr_o           (endpoint_in_addr_lo   )
    ,.in_we_o             (endpoint_in_we_lo     )
    ,.in_src_x_cord_o     (in_src_x_cord_lo      )
    ,.in_src_y_cord_o     (in_src_y_cord_lo      )

    // fifo -> manycore packet
    ,.out_v_i             (endpoint_out_v_li     )
    ,.out_packet_i        (endpoint_out_packet_li)
    ,.out_ready_o         (endpoint_out_ready_lo )

    // manycore credit -> fifo
    ,.returned_data_r_o   (returned_data_r_lo    )
    ,.returned_load_id_r_o(returned_load_id_r_lo )
    ,.returned_v_r_o      (returned_v_r_lo       )
    ,.returned_fifo_full_o(returned_fifo_full_lo )
    // always 1'b1 if returned_fifo_p is not set
    ,.returned_yumi_i     (returned_yumi_li      )

    // fifo -> manycore credit
    ,.returning_data_i    (returning_data_li     )
    ,.returning_v_i       (returning_v_li        )

    ,.out_credits_o       (out_credits_o         )
    ,.my_x_i              (my_x_i                )
    ,.my_y_i              (my_y_i                )
  );


  assign print_stat_v_o = endpoint_in_v_lo & endpoint_in_we_lo
    & ({endpoint_in_addr_lo[13:0], 2'b00} == 16'h0D0C);
  assign print_stat_tag_o = endpoint_in_data_lo;

endmodule
