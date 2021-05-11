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

  ,input  [               x_cord_width_p-1:0] global_x_i
  ,input  [               y_cord_width_p-1:0] global_y_i

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
  bsg_manycore_return_packet_aligned_s mc_rsp_cast_lo;
  assign mc_rsp_o                    = mc_rsp_cast_lo;

  logic                                    mc_rsp_v_r_lo;
  logic                                    mc_rsp_yumi_li;
  logic                                    mc_rsp_fifo_full_lo;
  bsg_manycore_return_packet_type_e        mc_rsp_pkt_type_r_lo;
  logic [                data_width_p-1:0] mc_rsp_data_r_lo;
  wire  [bsg_manycore_reg_id_width_gp-1:0] mc_rsp_reg_id_r_lo;

  // Manycore Request
  bsg_manycore_packet_aligned_s mc_req_cast_lo;
  assign mc_req_o = mc_req_cast_lo;

  logic                         mc_req_v_lo;
  logic                         mc_req_yumi_li;
  logic [     data_width_p-1:0] mc_req_data_lo;
  logic [(data_width_p>>3)-1:0] mc_req_mask_lo;
  logic [     addr_width_p-1:0] mc_req_addr_lo;
  logic                         mc_req_we_lo;
  bsg_manycore_load_info_s      mc_req_load_info_lo;
  logic [   x_cord_width_p-1:0] mc_req_src_x_cord_lo;
  logic [   y_cord_width_p-1:0] mc_req_src_y_cord_lo;

  // Endpoint Response
  logic                    ep_rsp_v_li;
  logic [data_width_p-1:0] ep_rsp_data_li;

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
  assign mc_rsp_v_o = mc_rsp_v_r_lo;
  assign mc_rsp_yumi_li = mc_rsp_ready_i & mc_rsp_v_o;

  assign mc_rsp_cast_lo.padding  = '0;
  assign mc_rsp_cast_lo.pkt_type = 8'(mc_rsp_pkt_type_r_lo);
  assign mc_rsp_cast_lo.data     = data_width_pad_lp'(mc_rsp_data_r_lo);
  assign mc_rsp_cast_lo.reg_id   = 8'(mc_rsp_reg_id_r_lo);
  assign mc_rsp_cast_lo.y_cord   = y_cord_width_pad_lp'(global_y_i);
  assign mc_rsp_cast_lo.x_cord   = x_cord_width_pad_lp'(global_x_i);

  // Manycore Request
  // ----------------------------
  assign mc_req_v_o = mc_req_v_lo;
  assign mc_req_yumi_li = mc_req_ready_i & mc_req_v_o;

  assign mc_req_cast_lo.padding    = '0;
  assign mc_req_cast_lo.addr       = addr_width_pad_lp'(mc_req_addr_lo);
  assign mc_req_cast_lo.op_v2      = mc_req_we_lo ? e_remote_store: e_remote_load;
  assign mc_req_cast_lo.reg_id     = 8'(mc_req_mask_lo);
  assign mc_req_cast_lo.payload    = data_width_p'(mc_req_data_lo);
  assign mc_req_cast_lo.src_y_cord = y_cord_width_pad_lp'(mc_req_src_y_cord_lo);
  assign mc_req_cast_lo.src_x_cord = x_cord_width_pad_lp'(mc_req_src_x_cord_lo);
  assign mc_req_cast_lo.y_cord     = y_cord_width_pad_lp'(global_y_i);
  assign mc_req_cast_lo.x_cord     = x_cord_width_pad_lp'(global_x_i);

  // Endpoint Response
  // ---------------------------

  // Delay automatic write responses by one cycle. This is dictated
  // by the endpoint standard, which expects a 1-cycle SRAM interface
  // to maintain full throughput.
  //
  // Responses are NOT sent for reads, just for writes. The
  // endpoint_standard handles amoswap responses and they do not need
  // to be handled here.
  // 
  // The endpoint is always ready for a response.
  logic ep_rsp_wr_v_r;
  always_ff @(posedge clk_i) begin
    if(reset_i)
      ep_rsp_wr_v_r <= '0;
    else
      ep_rsp_wr_v_r <= mc_req_yumi_li & mc_req_we_lo;
  end

  assign ep_rsp_data_li = '0; // ep_rsp data is zero by default. The endpoint cannot respond.
  assign ep_rsp_v_li    = ep_rsp_wr_v_r;

  bsg_manycore_endpoint_standard #(
    .x_cord_width_p(x_cord_width_p)
    ,.y_cord_width_p(y_cord_width_p)
    ,.fifo_els_p(ep_fifo_els_p)
    ,.addr_width_p(addr_width_p)
    ,.data_width_p(data_width_p)
    ,.credit_counter_width_p(credit_counter_width_p)
    ,.rev_fifo_els_p(rev_fifo_els_p)
  ) epsd (
    .clk_i(clk_i)
    ,.reset_i(reset_i)

    ,.link_sif_i(link_sif_i)
    ,.link_sif_o(link_sif_o)

    // Manycore Request -> Endpoint
    ,.in_v_o(mc_req_v_lo)
    ,.in_yumi_i(mc_req_yumi_li)
    ,.in_data_o(mc_req_data_lo)
    ,.in_mask_o(mc_req_mask_lo)
    ,.in_addr_o(mc_req_addr_lo)
    ,.in_we_o(mc_req_we_lo)
    ,.in_load_info_o(mc_req_load_info_lo)
    ,.in_src_x_cord_o(mc_req_src_x_cord_lo)
    ,.in_src_y_cord_o(mc_req_src_y_cord_lo)

    // Endpoint Request -> Manycore
    ,.out_v_i(packet_ep_req_v_li)
    ,.out_packet_i(packet_ep_req_li)
    ,.out_credit_or_ready_o(packet_ep_req_ready_lo)

    // Manycore Response -> Endpoint
    ,.returned_v_r_o(mc_rsp_v_r_lo)
    ,.returned_yumi_i(mc_rsp_yumi_li)
    ,.returned_data_r_o(mc_rsp_data_r_lo)
    ,.returned_reg_id_r_o(mc_rsp_reg_id_r_lo)
    ,.returned_pkt_type_r_o(mc_rsp_pkt_type_r_lo)
    ,.returned_fifo_full_o(mc_rsp_fifo_full_lo) // TODO: Assert

    // Endpoint Response -> Manycore
    ,.returning_data_i(ep_rsp_data_li)
    ,.returning_v_i(ep_rsp_v_li)

    ,.out_credits_used_o(out_credits_used_o)

    ,.global_x_i(global_x_i)
    ,.global_y_i(global_y_i)
  );

  // synopsys translate_off
  always @(negedge clk_i) begin
    if (~reset_i & mc_req_v_lo) begin
       assert (mc_req_we_lo) else $error("[BSG_ERROR] Host interface cannot respond to read requests");
    end
  end

  always @(posedge clk_i) begin
    if (debug_p & packet_ep_req_v_li & packet_ep_req_ready_lo) begin
      $display("bsg_manycore_endpoint_to_fifos (request (host->mc)): op_v2=%d", packet_ep_req_li.op_v2);
      $display("bsg_manycore_endpoint_to_fifos (request (host->mc)): addr=%h", packet_ep_req_li.addr);
      $display("bsg_manycore_endpoint_to_fifos (request (host->mc)): data=%h", packet_ep_req_li.payload.data);
      $display("bsg_manycore_endpoint_to_fifos (request (host->mc)): reg_id=%h", packet_ep_req_li.reg_id);
      $display("bsg_manycore_endpoint_to_fifos (request (host->mc)): x_cord=%d", packet_ep_req_li.x_cord);
      $display("bsg_manycore_endpoint_to_fifos (request (host->mc)): y_cord=%d", packet_ep_req_li.y_cord);
      $display("bsg_manycore_endpoint_to_fifos (request (host->mc)): src_x_cord=%d", packet_ep_req_li.src_x_cord);
      $display("bsg_manycore_endpoint_to_fifos (request (host->mc)): src_y_cord=%d", packet_ep_req_li.src_y_cord);
    end
  end

  always @(posedge clk_i) begin
    if (debug_p & mc_rsp_v_o & mc_rsp_ready_i) begin
      $display("bsg_manycore_endpoint_to_fifos (response (mc->host)): type=%s", mc_rsp_pkt_type_r_lo.name());
      $display("bsg_manycore_endpoint_to_fifos (response (mc->host)): data=%h", mc_rsp_cast_lo.data);
      $display("bsg_manycore_endpoint_to_fifos (response (mc->host)): reg_id=%h", mc_rsp_cast_lo.reg_id);
    end
  end
  // synopsys translate_on

endmodule
