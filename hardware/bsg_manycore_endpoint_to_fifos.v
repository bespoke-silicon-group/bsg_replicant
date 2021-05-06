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
//

/*
 *  bsg_manycore_endpoint_to_fifos.v
 *
 * Convert manycore packets into FIFO data streams, with fields aligned
 * to 8-bit/1-byte boundaries, or vice-versa.
 *
 */

module bsg_manycore_endpoint_to_fifos
  import bsg_manycore_pkg::*;
#(
  parameter fifo_width_p = "inv"
  // these are endpoint parameters
  , parameter x_cord_width_p = "inv"
  , localparam x_cord_width_pad_lp = `BSG_CDIV(x_cord_width_p,8)*8
  , parameter y_cord_width_p = "inv"
  , localparam y_cord_width_pad_lp = `BSG_CDIV(y_cord_width_p,8)*8
  , parameter addr_width_p = "inv"
  , localparam addr_width_pad_lp = `BSG_CDIV(addr_width_p,8)*8
  , parameter data_width_p = "inv"
  , localparam data_width_pad_lp = `BSG_CDIV(data_width_p,8)*8
  , localparam reg_id_width_pad_lp = `BSG_CDIV(bsg_manycore_reg_id_width_gp,8)*8
  , parameter credit_counter_width_p = `BSG_WIDTH(32)
  , parameter ep_fifo_els_p = "inv"
  , parameter rev_fifo_els_p="inv" // for FIFO credit counting.

  , parameter link_sif_width_lp = `bsg_manycore_link_sif_width(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p)
  , parameter debug_p = 0
) (
  input                                      clk_i
  ,input                                      reset_i

  // manycore request
  ,output [                 fifo_width_p-1:0] mc_req_o
  ,output                                     mc_req_v_o
  ,input                                      mc_req_ready_i

  // endpoint request
  ,input  [                 fifo_width_p-1:0] endpoint_req_i
  ,input                                      endpoint_req_v_i
  ,output                                     endpoint_req_ready_o

  // manycore response
  ,output [                 fifo_width_p-1:0] mc_rsp_o
  ,output                                     mc_rsp_v_o
  ,input                                      mc_rsp_ready_i

  // endpoint response
  ,input  [                 fifo_width_p-1:0] endpoint_rsp_i
  ,input                                      endpoint_rsp_v_i
  ,output                                     endpoint_rsp_ready_o

  // manycore link
  ,input  [            link_sif_width_lp-1:0] link_sif_i
  ,output [            link_sif_width_lp-1:0] link_sif_o

  ,output [credit_counter_width_p-1:0] out_credits_used_o

  );

  `declare_bsg_manycore_packet_aligned_s(fifo_width_p, addr_width_pad_lp, data_width_pad_lp, x_cord_width_pad_lp, y_cord_width_pad_lp);
  `declare_bsg_manycore_packet_s(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p);

  // Endpoint Request
  logic                 packet_ep_req_v_li;
  bsg_manycore_packet_s packet_ep_req_li;
  logic                 packet_ep_req_ready_lo;

  bsg_manycore_packet_aligned_s endpoint_req_cast_li;
  assign endpoint_req_cast_li = endpoint_req_i;

  // Manycore Response
  logic                                packet_mc_rsp_v_lo;
  bsg_manycore_return_packet_s         packet_mc_rsp_lo;
  logic                                packet_mc_rsp_yumi_li;

  bsg_manycore_return_packet_aligned_s mc_rsp_cast_lo;
  assign mc_rsp_o                    = mc_rsp_cast_lo;

  // Manycore Request
  logic                 packet_mc_req_v_lo;
  bsg_manycore_packet_s packet_mc_req_lo;
  logic                 packet_mc_req_yumi_li;

  bsg_manycore_packet_aligned_s mc_req_cast_lo;
  assign mc_req_o = mc_req_cast_lo;

  // Endpoint Response
  logic                 packet_ep_rsp_v_li;
  bsg_manycore_return_packet_s packet_ep_rsp_li;

  bsg_manycore_return_packet_aligned_s endpoint_rsp_cast_li;
  assign endpoint_rsp_cast_li = endpoint_rsp_i;

  // Endpoint Request
  // -------------------------
  assign packet_ep_req_v_li = endpoint_req_v_i;
  assign endpoint_req_ready_o = packet_ep_req_ready_lo;

  assign packet_ep_req_li.addr       = addr_width_p'(endpoint_req_cast_li.addr);
  assign packet_ep_req_li.op_v2      = bsg_manycore_packet_op_e'(endpoint_req_cast_li.op_v2);
  assign packet_ep_req_li.reg_id     = bsg_manycore_reg_id_width_gp'(endpoint_req_cast_li.reg_id);
  assign packet_ep_req_li.payload    = endpoint_req_cast_li.payload;
  assign packet_ep_req_li.src_y_cord = y_cord_width_p'(endpoint_req_cast_li.src_y_cord);
  assign packet_ep_req_li.src_x_cord = x_cord_width_p'(endpoint_req_cast_li.src_x_cord);
  assign packet_ep_req_li.y_cord     = y_cord_width_p'(endpoint_req_cast_li.y_cord);
  assign packet_ep_req_li.x_cord     = x_cord_width_p'(endpoint_req_cast_li.x_cord);

  // Manycore Response
  // -----------------------------
  assign mc_rsp_v_o = packet_mc_rsp_v_lo;
  assign packet_mc_rsp_yumi_li = mc_rsp_ready_i & mc_rsp_v_o; // TODO: Fix

  assign mc_rsp_cast_lo.padding  = '0;
  assign mc_rsp_cast_lo.pkt_type = 8'(packet_mc_rsp_lo.pkt_type);
  assign mc_rsp_cast_lo.data     = data_width_pad_lp'(packet_mc_rsp_lo.data);
  assign mc_rsp_cast_lo.reg_id   = reg_id_width_pad_lp'(packet_mc_rsp_lo.reg_id);
  assign mc_rsp_cast_lo.y_cord   = y_cord_width_pad_lp'(packet_mc_rsp_lo.y_cord);
  assign mc_rsp_cast_lo.x_cord   = x_cord_width_pad_lp'(packet_mc_rsp_lo.x_cord);

  // Manycore Request
  // ----------------------------
  assign mc_req_v_o = packet_mc_req_v_lo;
  assign packet_mc_req_yumi_li = mc_req_ready_i & mc_req_v_o; // TODO: Fix

  assign mc_req_cast_lo.padding    = '0;
  assign mc_req_cast_lo.addr       = addr_width_pad_lp'(packet_mc_req_lo.addr);
  assign mc_req_cast_lo.op_v2      = 8'(packet_mc_req_lo.op_v2);
  assign mc_req_cast_lo.reg_id     = reg_id_width_pad_lp'(packet_mc_req_lo.reg_id);
  assign mc_req_cast_lo.payload    = data_width_pad_lp'(packet_mc_req_lo.payload);
  assign mc_req_cast_lo.src_y_cord = y_cord_width_pad_lp'(packet_mc_req_lo.src_y_cord);
  assign mc_req_cast_lo.src_x_cord = x_cord_width_pad_lp'(packet_mc_req_lo.src_x_cord);
  assign mc_req_cast_lo.y_cord     = y_cord_width_pad_lp'(packet_mc_req_lo.y_cord);
  assign mc_req_cast_lo.x_cord     = x_cord_width_pad_lp'(packet_mc_req_lo.x_cord);

  // Endpoint Response
  // -----------------------------
  assign packet_ep_rsp_v_li = endpoint_rsp_v_i;
  assign endpoint_rsp_ready_o = '1;

  assign packet_ep_rsp_li.pkt_type = bsg_manycore_return_packet_type_e'(endpoint_rsp_cast_li.pkt_type);
  assign packet_ep_rsp_li.data = data_width_p'(endpoint_rsp_cast_li.data);
  assign packet_ep_rsp_li.reg_id = bsg_manycore_reg_id_width_gp'(endpoint_rsp_cast_li.reg_id);
  assign packet_ep_rsp_li.y_cord = y_cord_width_p'(endpoint_rsp_cast_li.y_cord);
  assign packet_ep_rsp_li.x_cord = x_cord_width_p'(endpoint_rsp_cast_li.x_cord);

  bsg_manycore_endpoint_fc #(
    .x_cord_width_p(x_cord_width_p)
    ,.y_cord_width_p(y_cord_width_p)
    ,.fifo_els_p(ep_fifo_els_p)
    ,.addr_width_p(addr_width_p)
    ,.data_width_p(data_width_p)
    ,.credit_counter_width_p(credit_counter_width_p)
  //  ,.use_credits_for_local_fifo_p(1'b1)
    ,.rev_fifo_els_p(rev_fifo_els_p) // TODO: Increase size
  ) epsd (
    .clk_i(clk_i)
    ,.reset_i(reset_i)

    ,.link_sif_i(link_sif_i)
    ,.link_sif_o(link_sif_o)

    // Manycore Request -> Endpoint
    ,.packet_v_o(packet_mc_req_v_lo)
    ,.packet_o(packet_mc_req_lo)
    ,.packet_yumi_i(packet_mc_req_yumi_li)

    // Endpoint Request -> Manycore
    ,.packet_v_i(packet_ep_req_v_li)
    ,.packet_i(packet_ep_req_li)
    ,.packet_credit_or_ready_o(packet_ep_req_ready_lo)

    // Manycore Response -> Endpoint
    ,.return_packet_o(packet_mc_rsp_lo)
    ,.return_packet_v_o(packet_mc_rsp_v_lo)
    ,.return_packet_yumi_i(packet_mc_rsp_yumi_li)

    // Endpoint Response -> Manycore
    ,.return_packet_i(packet_ep_rsp_li)
    ,.return_packet_v_i(packet_ep_rsp_v_li)

    ,.out_credits_used_o(out_credits_used_o)
  );

  // synopsys translate_off
  always @(posedge clk_i) begin
    if (debug_p & packet_ep_req_v_li & packet_ep_req_ready_lo) begin
      $display("bsg_manycore_endpoint_to_fifos: op_v2=%d", packet_ep_req_li.op_v2);
      $display("bsg_manycore_endpoint_to_fifos: addr=%h", packet_ep_req_li.addr);
      $display("bsg_manycore_endpoint_to_fifos: data=%h", packet_ep_req_li.payload.data);
      $display("bsg_manycore_endpoint_to_fifos: reg_id=%h", packet_ep_req_li.reg_id);
      $display("bsg_manycore_endpoint_to_fifos: x_cord=%d", packet_ep_req_li.x_cord);
      $display("bsg_manycore_endpoint_to_fifos: y_cord=%d", packet_ep_req_li.y_cord);
      $display("bsg_manycore_endpoint_to_fifos: src_x_cord=%d", packet_ep_req_li.src_x_cord);
      $display("bsg_manycore_endpoint_to_fifos: src_y_cord=%d", packet_ep_req_li.src_y_cord);
    end
  end

  always @(posedge clk_i) begin
    if (debug_p & mc_rsp_v_o & mc_rsp_ready_i) begin
      $display("bsg_manycore_endpoint_to_fifos (response): type=%s", packet_mc_rsp_lo.pkt_type.name());
      $display("bsg_manycore_endpoint_to_fifos (response): data=%h", mc_rsp_cast_lo.data);
      $display("bsg_manycore_endpoint_to_fifos (response): reg_id=%h", mc_rsp_cast_lo.reg_id);
    end
  end
  // synopsys translate_on

endmodule
