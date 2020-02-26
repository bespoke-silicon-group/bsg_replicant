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
*  bsg_mcl_axil_fifos_slave.v
*
* The host is a slave attached to the manycore endpoint.
* It receives the manycore's requests by reading the AXIL read channel.
*/

module bsg_mcl_axil_fifos_slave
  import bsg_manycore_link_to_axil_pkg::*;
#(
  parameter fifo_width_p = "inv"
  , parameter mc_write_capacity_p = "inv"
  , parameter axil_data_width_p = 32
  , localparam integer piso_els_lp = fifo_width_p/axil_data_width_p
  , localparam pkt_cnt_width_lp = `BSG_WIDTH(mc_write_capacity_p*piso_els_lp)
) (
  input                          clk_i
  ,input                          reset_i
  // host side
  ,output [axil_data_width_p-1:0] r_data_o
  ,output                         r_v_o
  ,input                          r_ready_i
  // mc side
  ,input  [     fifo_width_p-1:0] mc_req_i
  ,input                          mc_req_v_i
  ,output                         mc_req_ready_o
  ,output [ pkt_cnt_width_lp-1:0] mc_req_words_o
);

  // synopsys translate_off
  initial begin
    assert (piso_els_lp * axil_data_width_p == fifo_width_p)
      else $fatal("BSG_ERROR][%m]: manycore link fifo width %d is not multiple of axil data width %d.",
        fifo_width_p, axil_data_width_p);
  end
  // synopsys translate_on

  logic                    buf_v_lo   ;
  logic [fifo_width_p-1:0] buf_data_lo;
  logic                    buf_yumi_li;

  logic piso_ready_lo;
  assign buf_yumi_li = piso_ready_lo & buf_v_lo;

  wire piso_yumi_li = r_ready_i & r_v_o;

  bsg_fifo_1r1w_small #(
    .width_p           (fifo_width_p       ),
    .els_p             (mc_write_capacity_p),
    .ready_THEN_valid_p(0                  )  // input handshake
  ) fifo (
    .clk_i  (clk_i         ),
    .reset_i(reset_i       ),
    .v_i    (mc_req_v_i    ),
    .ready_o(mc_req_ready_o),
    .data_i (mc_req_i      ),
    .v_o    (buf_v_lo      ),
    .data_o (buf_data_lo   ),
    .yumi_i (buf_yumi_li   )
  );

  bsg_parallel_in_serial_out #(
    .width_p(axil_data_width_p),
    .els_p  (piso_els_lp      )
  ) piso (
    .clk_i  (clk_i        ),
    .reset_i(reset_i      ),
    .valid_i(buf_v_lo     ),
    .data_i (buf_data_lo  ),
    .ready_o(piso_ready_lo),
    .valid_o(r_v_o        ),
    .data_o (r_data_o     ),
    .yumi_i (piso_yumi_li )
  );

  localparam step_width_lp = `BSG_WIDTH(piso_els_lp);

  wire [step_width_lp-1:0] cnt_down_li = piso_yumi_li ? step_width_lp'(1) : '0                           ;
  wire [step_width_lp-1:0] cnt_up_li   = (mc_req_ready_o & mc_req_v_i) ? step_width_lp'(piso_els_lp) : '0;

  bsg_counter_up_down #(
    .max_val_p (mc_write_capacity_p*piso_els_lp),
    .init_val_p(0                              ),
    .max_step_p(piso_els_lp                    )
  ) cnt_mc_pkt (
    .clk_i  (clk_i         ),
    .reset_i(reset_i       ),
    .down_i (cnt_down_li   ), // release mc pkt ->
    .up_i   (cnt_up_li     ), // get mc pkt <-
    .count_o(mc_req_words_o)
  );

endmodule
