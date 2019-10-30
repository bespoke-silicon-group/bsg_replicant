// Copyright (c) 2019, University of Washington All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this list
// of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
//
// Neither the name of the copyright holder nor the names of its contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
*  <== |mc response  | <- |~//~| <- |=returned_pkt_o | <-- rcv_vacancy_i
*      |_____________|    |____|    |                |
*  <== |mc request   | <- |~~~~| <- |=in_pkt_o       |
*      |_____________|    |____|    |________________|
*
*
*
* Note:
* the host request is fenced if any of the following conditions are met:
* 1. manycore endpoint out credits == 0
* 2. out_pkt is load AND mc_rsp_rcv_vacancy_i < max_out_credits_p
*    We are not using 0 as the receive fifo vacancy threshold because of the network latency
*
*/

`include "bsg_manycore_packet.vh"
`include "cl_manycore_pkg.v"
`include "bsg_manycore_link_to_axil_pkg.v"
// `include "bsg_manycore_addr_pkg.v" // this file should have `ifndef

module bsg_manycore_endpoint_to_fifos
  import cl_manycore_pkg::*;
  import bsg_manycore_link_to_axil_pkg::*;
  import bsg_manycore_addr_pkg::bsg_print_stat_epa_gp;
#(
  parameter mcl_fifo_width_p = "inv"
  // these are endpoint parameters
  , parameter x_cord_width_p = "inv"
  , parameter y_cord_width_p = "inv"
  , parameter addr_width_p = "inv"
  , parameter data_width_p = "inv"
  , parameter max_out_credits_p = "inv"
  , parameter load_id_width_p = "inv"
  , parameter link_sif_width_lp = `bsg_manycore_link_sif_width(addr_width_p,data_width_p,
  x_cord_width_p,y_cord_width_p,load_id_width_p)
  , localparam lg_rcv_fifo_els_lp = `BSG_WIDTH(rcv_fifo_els_gp)
) (
  input                                      clk_i
  ,input                                      reset_i
  // manycore as master
  ,output [             mcl_fifo_width_p-1:0] mc_req_o
  ,output                                     mc_req_v_o
  ,input                                      mc_req_ready_i
  ,input  [             mcl_fifo_width_p-1:0] host_rsp_i
  ,input                                      host_rsp_v_i
  ,output                                     host_rsp_ready_o
  // host as master
  ,input  [             mcl_fifo_width_p-1:0] host_req_i
  ,input                                      host_req_v_i
  ,output                                     host_req_ready_o
  ,output [             mcl_fifo_width_p-1:0] mc_rsp_o
  ,output                                     mc_rsp_v_o
  ,input                                      mc_rsp_ready_i
  // manycore link
  ,input  [            link_sif_width_lp-1:0] link_sif_i
  ,output [            link_sif_width_lp-1:0] link_sif_o
  // endpoint coord
  ,input  [               x_cord_width_p-1:0] my_x_i
  ,input  [               y_cord_width_p-1:0] my_y_i
  // receive fifo reach threshold
  ,input  [           lg_rcv_fifo_els_lp-1:0] mc_rsp_rcv_vacancy_i
  // endpoint out packet credits
  ,output [`BSG_WIDTH(max_out_credits_p)-1:0] out_credits_o
  // print stat
  ,output                                     print_stat_v_o
  ,output [                 data_width_p-1:0] print_stat_tag_o
);


  `declare_bsg_manycore_link_fifo_s(mcl_fifo_width_gp, mcl_data_width_gp, mcl_addr_width_gp,
                                    mcl_pkt_type_width_gp, mcl_load_id_width_gp, mcl_op_width_gp, mcl_op_ex_width_gp,
                                    mcl_x_cord_width_gp, mcl_y_cord_width_gp);

  bsg_mcl_request_s mc_req_lo_cast, host_req_li_cast;
  bsg_mcl_response_s host_rsp_li_cast, mc_rsp_lo_cast;

  // host as master
  assign host_req_li_cast = host_req_i;
  assign mc_rsp_o = mc_rsp_lo_cast;

  // manycore as master
  assign mc_req_o = mc_req_lo_cast;
  assign host_rsp_li_cast = host_rsp_i;


  // default op_width in manycore request packet
  // Should this be defined in bsg_manycore_packet.vh ?
  localparam mc_op_width_lp = 2;

  // manycore endpoint signals
  localparam packet_width_lp = `bsg_manycore_packet_width(addr_width_p, data_width_p,
                                                          x_cord_width_p, y_cord_width_p, load_id_width_p);

  logic                         endpoint_in_v_lo   ;
  logic                         endpoint_in_yumi_li;
  logic [     data_width_p-1:0] endpoint_in_data_lo;
  logic [(data_width_p>>3)-1:0] endpoint_in_mask_lo;
  logic [     addr_width_p-1:0] endpoint_in_addr_lo;
  logic                         endpoint_in_we_lo  ;
  logic [   x_cord_width_p-1:0] in_src_x_cord_lo   ;
  logic [   y_cord_width_p-1:0] in_src_y_cord_lo   ;

  logic                       endpoint_out_v_li     ;
  logic [packet_width_lp-1:0] endpoint_out_packet_li;
  logic                       endpoint_out_ready_lo ;

  logic [   data_width_p-1:0] returned_data_r_lo   ;
  logic [load_id_width_p-1:0] returned_load_id_r_lo;
  logic                       returned_v_r_lo      ;
  logic                       returned_fifo_full_lo;
  logic                       returned_yumi_li     ;

  logic [data_width_p-1:0] returning_data_li;
  logic                    returning_v_li   ;

  // check paramters before field slicing
  // synopsys translate_off
  initial begin
    assert (data_width_p <= mcl_data_width_gp)
      else $fatal("data width of mcl is of the range. [%m]");
    assert (addr_width_p <= mcl_load_id_width_gp)
      else $fatal("addr width of mcl is of the range. [%m]");
    assert (load_id_width_p <= mcl_load_id_width_gp)
      else $fatal("load id width of mcl is of the range. [%m]");
    assert (mc_op_width_lp <= mcl_pkt_type_width_gp)
      else $fatal("op width of mcl is of the range. [%m]");
    assert (x_cord_width_p <= mcl_x_cord_width_gp)
      else $fatal("x cord width of mcl is of the range. [%m]");
    assert (y_cord_width_p <= mcl_y_cord_width_gp)
      else $fatal("y cord width of mcl is of the range. [%m]");
  end
  // synopsys translate_on

  // manycore request to host
  // -------------------------
  assign mc_req_v_o = endpoint_in_v_lo;
  assign endpoint_in_yumi_li = mc_req_ready_i & mc_req_v_o;
  assign mc_req_lo_cast.padding = '0;
  assign mc_req_lo_cast.addr = endpoint_in_addr_lo;
  assign mc_req_lo_cast.op = endpoint_in_we_lo;
  assign mc_req_lo_cast.op_ex = endpoint_in_mask_lo;
  assign mc_req_lo_cast.payload.data = endpoint_in_data_lo;
  assign mc_req_lo_cast.src_x_cord = in_src_x_cord_lo;
  assign mc_req_lo_cast.src_y_cord = in_src_y_cord_lo;
  assign mc_req_lo_cast.y_cord = my_y_i;
  assign mc_req_lo_cast.x_cord = my_x_i;


  // manycore response to host
  // -------------------------
  assign mc_rsp_v_o = returned_v_r_lo;
  assign mc_rsp_lo_cast.padding = '0;
  assign mc_rsp_lo_cast.pkt_type = mcl_pkt_type_width_gp'({`ePacketType_data});  // Careful when casting value with macro!!!
  assign mc_rsp_lo_cast.data = returned_data_r_lo;
  assign mc_rsp_lo_cast.load_id = returned_load_id_r_lo;
  assign mc_rsp_lo_cast.y_cord = my_y_i;
  assign mc_rsp_lo_cast.x_cord = my_x_i;
  assign returned_yumi_li = mc_rsp_ready_i & mc_rsp_v_o;


  // host request to manycore
  // -------------------------
  wire fifo_req_fence = (out_credits_o == '0) ||
                        ((host_req_li_cast.op == mc_op_width_lp'({`ePacketOp_remote_load})) &&
                          mc_rsp_rcv_vacancy_i<max_out_credits_p);

  assign endpoint_out_packet_li = {
    (addr_width_p)'(host_req_li_cast.addr)
    ,(mc_op_width_lp)'(host_req_li_cast.op)
    ,(data_width_p>>3)'(host_req_li_cast.op_ex)
    ,(data_width_p)'(host_req_li_cast.payload.data)
    ,(y_cord_width_p)'(host_req_li_cast.src_y_cord)
    ,(x_cord_width_p)'(host_req_li_cast.src_x_cord)
    ,(y_cord_width_p)'(host_req_li_cast.y_cord)
    ,(x_cord_width_p)'(host_req_li_cast.x_cord)
  };
  assign endpoint_out_v_li = ~fifo_req_fence & host_req_v_i;
  assign host_req_ready_o = ~fifo_req_fence & endpoint_out_ready_lo;


  // host response to manycore
  // -------------------------
  logic returning_wr_v_r;
  always_ff @(posedge clk_i) begin
    if(reset_i)
      returning_wr_v_r <= '0;
    else
      returning_wr_v_r <= endpoint_in_yumi_li & endpoint_in_we_lo;
  end

  assign host_rsp_ready_o  = ~returning_wr_v_r;
  assign returning_data_li = returning_wr_v_r ? '0 : (data_width_p)'(host_rsp_li_cast.data);
  assign returning_v_li    = returning_wr_v_r | (host_rsp_v_i & host_rsp_ready_o);

  bsg_manycore_endpoint_standard #(
    .x_cord_width_p   (x_cord_width_p   )
    ,.y_cord_width_p   (y_cord_width_p   )
    ,.fifo_els_p       (mc_ep_fifo_els_gp)
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

  // convert endpoint word address to EPA byte address
  assign print_stat_v_o = endpoint_in_v_lo & endpoint_in_we_lo
    & ({endpoint_in_addr_lo[13:0], 2'b00} == bsg_print_stat_epa_gp);
  assign print_stat_tag_o = endpoint_in_data_lo;

endmodule
