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


`include "bsg_defines.v"
`include "bsg_manycore_link_to_axil_pkg.v"

module bsg_axil_to_fifos_tx
  import bsg_manycore_link_to_axil_pkg::*;
#(
  parameter num_fifos_p = "inv"
  , parameter mcl_fifo_width_p = "inv" // bit wise
  , localparam lg_fifo_els_lp = `BSG_WIDTH(axil_fifo_els_gp)
) (
  input                                          clk_i
  ,input                                          reset_i
  // axil tx channel
  ,input  [           31:0]                       awaddr_i
  ,input                                          awvalid_i
  ,output                                         awready_o
  ,input  [           31:0]                       wdata_i
  ,input  [            3:0]                       wstrb_i
  ,input                                          wvalid_i
  ,output                                         wready_o
  ,output [            1:0]                       bresp_o
  ,output                                         bvalid_o
  ,input                                          bready_i
  // fifo data out
  ,output [num_fifos_p-1:0][mcl_fifo_width_p-1:0] fifo_data_o
  ,output [num_fifos_p-1:0]                       fifo_v_o
  ,input  [num_fifos_p-1:0]                       fifo_ready_i
  ,output [num_fifos_p-1:0][  lg_fifo_els_lp-1:0] tx_vacancy_o
);

  localparam sipo_els_lp = mcl_fifo_width_p/axil_data_width_gp;

  // to axil tx from axil tx
  logic [num_fifos_p-1:0][axil_data_width_gp-1:0] txs_data_lo ;
  logic [num_fifos_p-1:0]                         txs_v_lo    ;
  logic [num_fifos_p-1:0]                         txs_ready_li;

  // from stream fifo to sipo manycore packets
  logic [num_fifos_p-1:0][axil_data_width_gp-1:0] axil_fifo_data_lo ;
  logic [num_fifos_p-1:0]                         axil_fifo_v_lo    ;
  logic [num_fifos_p-1:0]                         axil_fifo_ready_li;


  // --------------------------------------------
  // axil write state machine
  // --------------------------------------------
  /*
  * axil_txs
  *             ___     ___     ___     ___     ___
  * clk     ___/   \___/   \___/   \___/   \___/   \___
  *                    _______
  * awaddr  XXXxxxxxxxX_ADDR__XXXXXXXXXXXXXXXXXXXXXXXXx
  *             _______________
  * awvalid ___/               \_______________________
  *                    ________
  * awready __________/        \_______________________
  *                             ________
  * wdata   XXXXxxxxxxxxxxxxxxxx_DATA___XXXXXXXXXXXXXXX
  *                             ________
  * wstrb   XXXXxxxxxxxxxxxxxxxx_STRB___XXXXXXXXXXXXXXX
  *                             _______
  * wvalid  ___XXXXXXXXXXXXXXXXx       \_______________
  *                             _______
  * wready  ___________________/       \_______________
  *                                     _______
  * bresp   XXXXXXXXXXXXXXXXXXXXXXXXXXXX_RESP__XXXXXXXX
  *                                     _______
  * bvalid  ___________________________/       \_______
  *         ___________________________________________
  * bready
  *
  * Although the awvalid and wvalid can be asserted at the same cycle,
  * we assume they come in series events, for simplicity.
  */

  typedef enum bit [1:0] {
    E_WR_IDLE,
    E_WR_ADDR,
    E_WR_DATA,
    E_WR_RESP
  } wr_state_e;

  wr_state_e wr_state_r, wr_state_n;

  logic write_bresp_lo;

  always_comb begin
    wr_state_n = wr_state_r;
    case (wr_state_r)
      E_WR_IDLE : begin
        if (awvalid_i)  // Normally, master asserts valid first, see the above ASCII waveform
          wr_state_n = E_WR_ADDR;
      end

      E_WR_ADDR : begin
        if (awvalid_i & awready_o)
          wr_state_n = E_WR_DATA;
      end

      E_WR_DATA : begin
        if (wvalid_i & wready_o)
          wr_state_n = E_WR_RESP;
      end

      E_WR_RESP : begin
        if (bvalid_o & bready_i)
          wr_state_n = E_WR_IDLE;
      end

      default : wr_state_n = E_WR_IDLE;
    endcase
  end

  always_ff @(posedge clk_i) begin
    if (reset_i)
      wr_state_r <= E_WR_IDLE;
    else
      wr_state_r <= wr_state_n;
  end

  wire in_wr_addr = wr_state_r == E_WR_ADDR;
  wire in_wr_data = wr_state_r == E_WR_DATA;
  wire in_wr_resp = wr_state_r == E_WR_RESP;


  logic [num_fifos_p-1:0] fifos_hit      ;
  logic [num_fifos_p-1:0] write_fifo_wait;
  logic [num_fifos_p-1:0] isr_addr_v     ;
  logic                   wr_addr_error  ;


  // ----------------------------------
  // axil SI outputs
  // ----------------------------------

  // flop the write address and bus response
  logic [31:0] wr_addr_r, wr_addr_n;
  logic [ 1:0] bresp_r, bresp_n;
  always_comb begin
    // get write address
    wr_addr_n     = (awvalid_i & awready_o) ? awaddr_i : wr_addr_r;
    // gen bus response
    wr_addr_error = ~(|fifos_hit);
    bresp_n       = (wvalid_i & wready_o) ? (wr_addr_error ? 2'b11 : 2'b00) : bresp_r; // DECERR or OKAY
  end

  always_ff @(posedge clk_i) begin
    if (reset_i) begin
      wr_addr_r <= '0;
      bresp_r   <= '0;
    end
    else begin
      wr_addr_r <= wr_addr_n;
      bresp_r   <= bresp_n;
    end
  end

  assign awready_o = in_wr_addr;  // always ready to accept address
  assign wready_o  = in_wr_data;  // always ready for the write
  assign bresp_o   = bresp_r;
  assign bvalid_o  = in_wr_resp;


  // ----------------------------------
  // stream fifo write
  // ----------------------------------

  wire [ fifo_idx_width_gp-1:0] fifo_index_li  = wr_addr_r[base_addr_width_gp+:fifo_idx_width_gp];
  wire [base_addr_width_gp-1:0] fifo_offset_li = wr_addr_r[0+:base_addr_width_gp]                ;

  for (genvar i=0; i<num_fifos_p; i++) begin : wr_addr_hit

    // axil tx addr hits the range for fifo No.i
    assign fifos_hit[i] = fifo_index_li == (i + (fifo_base_addr_gp>>base_addr_width_gp));

    // is writing transmission data register
    assign write_fifo_wait[i] = in_wr_data && fifos_hit[i] && fifo_offset_li == axil_mm2s_ofs_tdr_gp;

  end : wr_addr_hit

  assign txs_v_lo    = {num_fifos_p{wvalid_i & wready_o}} & write_fifo_wait;
  assign txs_data_lo = {num_fifos_p{wdata_i}};


  // --------------------------------------------------------
  // axil fifos to store the packets received from host
  // --------------------------------------------------------

  wire [num_fifos_p-1:0] tx_enqueue = txs_v_lo & txs_ready_li            ;
  wire [num_fifos_p-1:0] tx_dequeue = axil_fifo_v_lo & axil_fifo_ready_li;

  for (genvar i=0; i<num_fifos_p; i++) begin : tx_s

    bsg_counter_up_down #(
      .max_val_p (axil_fifo_els_gp)
      ,.init_val_p(axil_fifo_els_gp)
      ,.max_step_p(1         )
    ) tx_vacancy_counter (
      .clk_i(clk_i)
      ,.reset_i(reset_i)
      ,.down_i (tx_enqueue[i]   )
      ,.up_i   (tx_dequeue[i]   )
      ,.count_o(tx_vacancy_o[i])
    );

    bsg_fifo_1r1w_small #(
      .width_p           (axil_data_width_gp)
      ,.els_p             (axil_fifo_els_gp  )
      ,.ready_THEN_valid_p(0                 )
    ) tx_fifo (
      .clk_i  (clk_i               )
      ,.reset_i(reset_i             )
      ,.v_i    (txs_v_lo[i]         )
      ,.ready_o(txs_ready_li[i]     )
      ,.data_i (txs_data_lo[i]      )
      ,.v_o    (axil_fifo_v_lo[i]   )
      ,.data_o (axil_fifo_data_lo[i])
      ,.yumi_i (tx_dequeue[i]       )
    );

  end : tx_s


// -------------------------------------
// stream upsizer
// -------------------------------------

  localparam yumi_cnt_width_lp = $clog2(sipo_els_lp+1)            ;
  localparam sipo_yumi_els_lp  = (yumi_cnt_width_lp)'(sipo_els_lp);

  logic [num_fifos_p-1:0][          sipo_els_lp-1:0] sipo_valid_lo   ;
  logic [num_fifos_p-1:0][yumi_cnt_width_lp-1:0] sipo_yumi_cnt_li;

  for (genvar i=0; i<num_fifos_p; i++) begin : upsizer

    bsg_serial_in_parallel_out #(
      .width_p(axil_data_width_gp)
      ,.els_p  (sipo_els_lp           )
    ) sipo (
      .clk_i     (clk_i                )
      ,.reset_i   (reset_i              )
      ,.valid_i   (axil_fifo_v_lo[i]    )
      ,.data_i    (axil_fifo_data_lo[i] )
      ,.ready_o   (axil_fifo_ready_li[i])
      ,.valid_o   (sipo_valid_lo[i]     )
      ,.data_o    (fifo_data_o[i]       )
      ,.yumi_cnt_i(sipo_yumi_cnt_li[i]  )
    );

    assign fifo_v_o[i]         = &sipo_valid_lo[i];
    assign sipo_yumi_cnt_li[i] = fifo_v_o[i] & fifo_ready_i[i] ? sipo_yumi_els_lp :'0;

  end : upsizer

endmodule
