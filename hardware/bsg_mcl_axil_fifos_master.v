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
*  bsg_mcl_axil_fifos_master.v
*
* The host is a master attached to the manycore endpoint.
* It sends requests and receives response through AXIL write and read channels
*/

`include "bsg_manycore_packet.vh"

module bsg_mcl_axil_fifos_master
  import bsg_manycore_link_to_axil_pkg::*;
#(
  parameter fifo_width_p = "inv"
  , parameter host_credits_p = "inv"
  , parameter axil_data_width_p = 32
  , localparam integer sipo_els_lp = fifo_width_p/axil_data_width_p
  , localparam credit_width_lp = `BSG_WIDTH(sipo_els_lp*host_credits_p)
) (
  input                          clk_i
  ,input                          reset_i
  // host side
  ,input  [axil_data_width_p-1:0] w_data_i
  ,input                          w_v_i
  ,output                         w_ready_o
  ,output [axil_data_width_p-1:0] r_data_o
  ,output                         r_v_o
  ,input                          r_ready_i
  // mc side
  ,output [     fifo_width_p-1:0] host_req_o
  ,output                         host_req_v_o
  ,input                          host_req_ready_i
  ,output [  credit_width_lp-1:0] host_credits_o
  ,input  [     fifo_width_p-1:0] mc_rsp_i
  ,input                          mc_rsp_v_i
  ,output                         mc_rsp_ready_o
);

  localparam integer piso_els_lp = sipo_els_lp;
  // synopsys translate_off
  initial begin
    assert (sipo_els_lp * axil_data_width_p == fifo_width_p)
      else $fatal("BSG_ERROR][%m]: manycore link fifo width %d is not multiple of axil data width %d.",
        fifo_width_p, axil_data_width_p);
  end
  // synopsys translate_on

  localparam sipo_cnt_width_lp = $clog2(sipo_els_lp+1);

  // set the two credits equal for simplicity, they can differ though. 
  localparam host_read_credits_lp = host_credits_p;
  localparam host_request_credits_lp = host_credits_p;
  
  localparam host_tx_fifo_els_lp = sipo_els_lp*host_request_credits_lp - (sipo_els_lp+1);


  // --------------------------------------------------------
  //                  host request to mc
  // --------------------------------------------------------

  // buf fifo tx
  logic [axil_data_width_p-1:0] req_buf_data_lo;
  logic                         req_buf_v_lo   ;
  logic                         req_buf_yumi_li;

  logic req_sipo_ready_lo;
  assign req_buf_yumi_li = req_buf_v_lo & req_sipo_ready_lo;

  // sipo tx
  logic [fifo_width_p-1:0] req_sipof_data_lo;
  logic                    req_sipof_v_lo   ;
  logic                    req_sipo_ready_li;

  // module tx
  logic pause_host_req;

  assign host_req_o        = req_sipof_data_lo;
  assign host_req_v_o      = ~pause_host_req & req_sipof_v_lo;
  assign req_sipo_ready_li = ~pause_host_req & host_req_ready_i;

  wire req_sipof_yumi_li = req_sipo_ready_li & host_req_v_o;

  bsg_fifo_1r1w_small #(
    .width_p           (axil_data_width_p  ),
    .els_p             (host_tx_fifo_els_lp),
    .ready_THEN_valid_p(0                  )
  ) fifo_req (
    .clk_i  (clk_i          ),
    .reset_i(reset_i        ),
    .v_i    (w_v_i          ),
    .ready_o(w_ready_o      ),
    .data_i (w_data_i       ),
    .v_o    (req_buf_v_lo   ),
    .data_o (req_buf_data_lo),
    .yumi_i (req_buf_yumi_li)
  );

  // sipof has elements sipo_els_lp+1
  bsg_serial_in_parallel_out_full #(
    .width_p(axil_data_width_p),
    .els_p  (sipo_els_lp      )
  ) sipof_req (
    .clk_i  (clk_i            ),
    .reset_i(reset_i          ),
    .v_i    (req_buf_v_lo     ),
    .ready_o(req_sipo_ready_lo),
    .data_i (req_buf_data_lo  ),
    .data_o (req_sipof_data_lo),
    .v_o    (req_sipof_v_lo   ),
    .yumi_i (req_sipof_yumi_li)
  );


  // --------------------------------------------------------
  //                  mc response to host
  // --------------------------------------------------------

  logic [fifo_width_p-1:0] rsp_buf_data_lo;
  logic                    rsp_buf_v_lo   ;
  logic                    rsp_buf_yumi_li;

  logic rsp_piso_ready_lo;
  assign rsp_buf_yumi_li = rsp_piso_ready_lo & rsp_buf_v_lo;

  wire rsp_piso_yumi_li = r_ready_i & r_v_o;

  bsg_fifo_1r1w_small #(
    .width_p           (fifo_width_p       ),
    .els_p             (host_read_credits_lp),
    .ready_THEN_valid_p(0                  )
  ) fifo_rsp (
    .clk_i  (clk_i          ),
    .reset_i(reset_i        ),
    .v_i    (mc_rsp_v_i     ),
    .ready_o(mc_rsp_ready_o ),
    .data_i (mc_rsp_i       ),
    .v_o    (rsp_buf_v_lo   ),
    .data_o (rsp_buf_data_lo),
    .yumi_i (rsp_buf_yumi_li)
  );

  bsg_parallel_in_serial_out #(
    .width_p(axil_data_width_gp),
    .els_p  (piso_els_lp       )
  ) piso_rsp (
    .clk_i  (clk_i            ),
    .reset_i(reset_i          ),
    .valid_i(rsp_buf_v_lo     ),
    .data_i (rsp_buf_data_lo  ),
    .ready_o(rsp_piso_ready_lo),
    .valid_o(r_v_o            ),
    .data_o (r_data_o         ),
    .yumi_i (rsp_piso_yumi_li )
  );


  // --------------------------------------------------------
  //                      Flow Control
  // --------------------------------------------------------

  wire [sipo_cnt_width_lp-1:0] cnt_down_li = (w_ready_o & w_v_i) ? sipo_cnt_width_lp'(1) : '0;
  wire [sipo_cnt_width_lp-1:0] cnt_up_li   = req_sipof_yumi_li ?
                                              sipo_cnt_width_lp'(sipo_els_lp) :
                                              '0;

  // Host will read this vacancy and update its request credits.
  // In general, we should set vacancy >= host credits to not overflow the fifo, here we set them equal
  // vacancy [words] := sipo_els_lp*host_request_credits_lp = elements of (req fifo + sipof) 
  bsg_counter_up_down #(
    .max_val_p (sipo_els_lp*host_request_credits_lp),
    .init_val_p(sipo_els_lp*host_request_credits_lp),
    .max_step_p(sipo_els_lp                    )
  ) cnt_req_credit (
    .clk_i  (clk_i         ),
    .reset_i(reset_i       ),
    .down_i (cnt_down_li   ),
    .up_i   (cnt_up_li     ),
    .count_o(host_credits_o)
  );


  // cast the request packet and detect the host read
  `declare_bsg_manycore_link_fifo_s(fifo_width_p, mcl_addr_width_gp, mcl_data_width_gp, mcl_x_cord_width_gp, mcl_y_cord_width_gp);

  // host as master
  bsg_mcl_request_s  host_req_lo_cast;
  assign host_req_lo_cast = host_req_o;

  logic [`BSG_WIDTH(host_read_credits_lp)-1:0] read_credits_lo;

  // pause the host request if:
  // 1) endpoint is out of credits (this is implemented outside)
  // 2) this module is out of read credits

  wire is_host_read_req = host_req_lo_cast.op == 8'(`ePacketOp_remote_load);
  assign pause_host_req = is_host_read_req && (read_credits_lo == 0);

  wire launching_read = host_req_v_o & host_req_ready_i & is_host_read_req;
  wire returned_read  = rsp_buf_yumi_li;

  bsg_counter_up_down #(
    .max_val_p (host_read_credits_lp),
    .init_val_p(host_read_credits_lp),
    .max_step_p(1                )
  ) cnt_rd_credit (
    .clk_i  (clk_i          ),
    .reset_i(reset_i        ),
    .down_i (launching_read ), // launch remote store ->
    .up_i   (returned_read  ), // receive credit back <-
    .count_o(read_credits_lo)
  );

endmodule
