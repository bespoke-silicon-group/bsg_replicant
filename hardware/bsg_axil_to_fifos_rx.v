// Copyright (c) 2019, University of Washington All rights reserved.  //
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

module bsg_axil_to_fifos_rx
  import bsg_manycore_link_to_axil_pkg::*;
#(
  parameter num_fifos_p = "inv"
  , parameter fifo_width_p = "inv" // bit wise
  , parameter rom_addr_width_p = "inv"
  , parameter max_out_credits_p = "inv"
  , localparam lg_fifo_els_lp = `BSG_WIDTH(axil_fifo_els_gp)
  , localparam lg_rcv_fifo_els_lp = `BSG_WIDTH(rcv_fifo_els_gp)
) (
  input                                                              clk_i
  ,input                                                              reset_i
  // axil rx channel
  ,input  [                             31:0]                         araddr_i
  ,input                                                              arvalid_i
  ,output                                                             arready_o
  ,output [                             31:0]                         rdata_o
  ,output [                              1:0]                         rresp_o
  ,output                                                             rvalid_o
  ,input                                                              rready_i
  // fifo data in
  ,input  [                  num_fifos_p-1:0][      fifo_width_p-1:0] fifo_data_i
  ,input  [                  num_fifos_p-1:0]                         fifo_v_i
  ,output [                  num_fifos_p-1:0]                         fifo_ready_o
  // mcl status
  ,output [                  num_fifos_p-1:0][lg_rcv_fifo_els_lp-1:0] rcv_vacancy_o
  ,input  [                  num_fifos_p-1:0][    lg_fifo_els_lp-1:0] tx_vacancy_i
  ,input  [`BSG_WIDTH(max_out_credits_p)-1:0]                         mcl_credits_i
  ,output [             rom_addr_width_p-1:0]                         rom_addr_o
  ,input  [                 fifo_width_p-1:0]                         rom_data_i
);


  localparam rx_bytes_lp = fifo_width_p/8                 ; // num of bytes per fifo element
  localparam piso_lp     = fifo_width_p/axil_data_width_gp; // this really should be int!


  // receive fifo to piso
  logic [num_fifos_p-1:0][fifo_width_p-1:0] rcv_fifo_data_lo ;
  logic [num_fifos_p-1:0]                   rcv_fifo_v_lo    ;
  logic [num_fifos_p-1:0]                   rcv_fifo_ready_li;

  // piso to stream fifo input
  logic [num_fifos_p-1:0][axil_data_width_gp-1:0] piso_data_lo ;
  logic [num_fifos_p-1:0]                         piso_v_li    ;
  logic [num_fifos_p-1:0]                         piso_ready_lo;

  // stream fifo to axil rx module input
  logic [num_fifos_p-1:0][fifo_width_p-1:0] rxs_data_li ;
  logic [num_fifos_p-1:0]                   rxs_v_li    ;
  logic [num_fifos_p-1:0]                   rxs_ready_lo;


  // -------------------------------------
  // receive fifos
  // -------------------------------------

  wire [num_fifos_p-1:0] rcv_enqueue = fifo_v_i & fifo_ready_o          ;
  wire [num_fifos_p-1:0] rcv_dequeue = rcv_fifo_ready_li & rcv_fifo_v_lo;

  for (genvar i=0; i<num_fifos_p; i++) begin : rcv_buf

    bsg_counter_up_down #(
      .max_val_p (rcv_fifo_els_gp),
      .init_val_p(rcv_fifo_els_gp),
      .max_step_p(1              )
    ) rcv_vacancy_cnt (
      .clk_i  (clk_i           ),
      .reset_i(reset_i         ),
      .down_i (rcv_enqueue[i]  ),
      .up_i   (rcv_dequeue[i]  ),
      .count_o(rcv_vacancy_o[i])  // ==> to axil rx
    );

    bsg_fifo_1r1w_small #(
      .width_p           (fifo_width_p   ),
      .els_p             (rcv_fifo_els_gp),
      .ready_THEN_valid_p(0              )  // input handshake
    ) rcv_fifo (
      .clk_i  (clk_i              ),
      .reset_i(reset_i            ),
      .v_i    (fifo_v_i[i]        ),
      .ready_o(fifo_ready_o[i]    ),
      .data_i (fifo_data_i[i]     ),
      .v_o    (rcv_fifo_v_lo[i]   ),
      .data_o (rcv_fifo_data_lo[i]),
      .yumi_i (rcv_dequeue[i]     )
    );

  end : rcv_buf


  // -------------------------------------
  // stream downsizer
  // -------------------------------------

  wire [num_fifos_p-1:0] piso_yumi_li = piso_ready_lo & piso_v_li;

  for (genvar i=0; i<num_fifos_p; i++) begin : dnsizer

    bsg_parallel_in_serial_out #(
      .width_p(axil_data_width_gp),
      .els_p  (piso_lp           )
    ) piso (
      .clk_i  (clk_i               ),
      .reset_i(reset_i             ),
      .valid_i(rcv_fifo_v_lo[i]    ),
      .data_i (rcv_fifo_data_lo[i] ),
      .ready_o(rcv_fifo_ready_li[i]),
      .valid_o(piso_v_li[i]        ),
      .data_o (piso_data_lo[i]     ),
      .yumi_i (piso_yumi_li[i]     )
    );

  end : dnsizer


  // --------------------------------------------------------
  // axil fifos
  // --------------------------------------------------------

  logic [num_fifos_p-1:0][lg_fifo_els_lp-1:0] rx_occupancy_lo;

  wire [num_fifos_p-1:0] rx_enqueue = piso_v_li & piso_ready_lo;
  wire [num_fifos_p-1:0] rx_dequeue = rxs_ready_lo & rxs_v_li;

  for (genvar i=0; i<num_fifos_p; i++) begin : rx

    bsg_counter_up_down #(
      .max_val_p (axil_fifo_els_gp)
      ,.init_val_p(0         )
      ,.max_step_p(1         )
    ) rx_occupancy_counter (
      .clk_i(clk_i)
      ,.reset_i(reset_i)
      ,.down_i (rx_dequeue[i]     )
      ,.up_i   (rx_enqueue[i]     )
      ,.count_o(rx_occupancy_lo[i])
    );

    bsg_fifo_1r1w_small #(
      .width_p           (fifo_width_p    ),
      .els_p             (axil_fifo_els_gp),
      .ready_THEN_valid_p(0               )
    ) mm2s_fifo (
      .clk_i  (clk_i           ),
      .reset_i(reset_i         ),
      .v_i    (piso_v_li[i]    ),
      .ready_o(piso_ready_lo[i]),
      .data_i (piso_data_lo[i] ),
      .v_o    (rxs_v_li[i]     ),
      .data_o (rxs_data_li[i]  ),
      .yumi_i (rx_dequeue[i]   )
    );

  end : rx


  // --------------------------------------------
  // axil read state machine
  // --------------------------------------------
  /*
  * axil_rxs
  *             ___     ___     ___     ___     ___
  * clk     ___/   \___/   \___/   \___/   \___/   \___
  *             _______
  * araddr  XXXX_ADDR__XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
  *             _______
  * arvalid ___/       \_______________________________
  *         ___________________________________________
  * arready
  *                                     _______
  * rdata   XXXXXXXXXXXXXXXXXXXXXXXXXXXX_DATA__XXXXXXXX
  *                                     _______
  * rresp   XXXXXXXXXXXXXXXXXXXXXXXXXXXX_RESP__XXXXXXXX
  *                                     _______
  * rvalid  ___________________________/       \_______
  *         ___________________________________________
  * rready
  */

  typedef enum bit [1:0] {
    E_RD_IDLE,
    E_RD_ADDR,
    E_RD_DATA
  } rd_state_e;

  rd_state_e rd_state_r, rd_state_n;

  always_comb begin
    rd_state_n = rd_state_r;
    case (rd_state_r)

      E_RD_IDLE : begin  // always ready to accept address
        if (arvalid_i)
          rd_state_n = E_RD_ADDR;
      end

      E_RD_ADDR : begin  // the rd address is flopped
        rd_state_n = E_RD_DATA;
      end

      E_RD_DATA : begin
        if (rvalid_o & rready_i)
          rd_state_n = E_RD_IDLE;
      end

      default : rd_state_n = E_RD_IDLE;
    endcase
  end

  always_ff @(posedge clk_i) begin
    if (reset_i)
      rd_state_r <= E_RD_IDLE;
    else
      rd_state_r <= rd_state_n;
  end

  wire in_rd_idle = rd_state_r == E_RD_IDLE;
  wire in_rd_addr = rd_state_r == E_RD_ADDR;
  wire in_rd_data = rd_state_r == E_RD_DATA;

  logic [base_addr_width_gp-1:0] fifo_offset_li;
  logic [ fifo_idx_width_gp-1:0] fifo_index_li ;
  logic [       num_fifos_p-1:0] fifos_base_hit;
  logic                          rom_base_hit  ;
  logic [       num_fifos_p-1:0] read_fifo_wait;
  logic [       num_fifos_p-1:0] rdata_batch_v ;
  logic [axil_data_width_gp-1:0] rdata_r       ;


  // ----------------------------------
  // axil SI outputs
  // ----------------------------------

  // flop the read address and read response
  logic [31:0] rd_addr_r, rd_addr_n;
  logic [ 1:0] rresp_n  ;
  always_comb begin
    // raddr channel
    rd_addr_n = (arvalid_i & arready_o & in_rd_idle) ? araddr_i : rd_addr_r;
    // read response, raise DECERR if access address is out of range
    rresp_n   = ~(|fifos_base_hit | rom_base_hit) & in_rd_data ? 2'b11 : '0; // DECERR or OKAY
  end

  always_ff @(posedge clk_i) begin
    if (reset_i) begin
      rd_addr_r <= '0;
    end
    else begin
      rd_addr_r <= rd_addr_n;
    end
  end

  assign arready_o = in_rd_addr | in_rd_idle;  // always ready to accept address
  assign rvalid_o  = in_rd_data;  // always valid for the read
  assign rdata_o   = rom_base_hit ? rom_data_i : rdata_r;
  assign rresp_o   = rresp_n;


  // ----------------------------------
  // read stream fifo
  // ----------------------------------

  assign fifo_index_li  = rd_addr_r[base_addr_width_gp+:fifo_idx_width_gp];
  assign fifo_offset_li = rd_addr_r[0+:base_addr_width_gp];

  for (genvar i=0; i<num_fifos_p; i++) begin : rd_addr_hit

    // axil rd addr hits the range for fifo No.i
    assign fifos_base_hit[i] = fifo_index_li == i + (fifo_base_addr_gp>>base_addr_width_gp);

    // rd command is read data register
    assign read_fifo_wait[i] = in_rd_data && fifos_base_hit[i] && fifo_offset_li == axil_mm2s_ofs_rdr_gp;

    // rd fifo has more than piso words
    assign rdata_batch_v[i] = rx_occupancy_lo[i] >= piso_lp;

  end : rd_addr_hit

  // rx stream ready signal to dequeue the axil fifo
  assign rxs_ready_lo = {num_fifos_p{rvalid_o & rready_i}} & read_fifo_wait;

  // Note: We assume here that casting to 32-bit wide axil rdata port is enough to avoid any bit loss.
  if (num_fifos_p == 1) begin : one  // very unlikely used in credit-based mc link, but who knows...

    always_comb begin
      case (fifo_offset_li[0+:axil_rx_addr_width_gp])
        axil_mm2s_ofs_tdfv_gp  : rdata_r = tx_vacancy_i;
        axil_mm2s_ofs_rdr_gp   : rdata_r = rxs_data_li;
        axil_mm2s_ofs_rdfo_gp  : rdata_r = {rx_occupancy_lo[lg_fifo_els_lp-1:2],2'b00};
        axil_mm2s_ofs_rlr_gp   : rdata_r = rdata_batch_v ? rx_bytes_lp : 0;
        mcl_ofs_rcvfv_gp       : rdata_r = rcv_vacancy_o;
        mcl_ofs_edp_credits_gp : rdata_r = mcl_credits_i;
        default                : rdata_r = fifo_width_p'(32'hBEEF_DEAD);
      endcase
    end

  end : one

  else begin : many

    logic [`BSG_SAFE_CLOG2(num_fifos_p)-1:0] fifos_idx_lo;
    bsg_encode_one_hot #(.width_p(num_fifos_p)) encode_rx_idx (
      .i     (fifos_base_hit),
      .addr_o(fifos_idx_lo  ),
      .v_o   (              )
    );

    always_comb begin : rd_regs
      case (fifo_offset_li[0+:axil_rx_addr_width_gp])
        axil_mm2s_ofs_tdfv_gp  : rdata_r = tx_vacancy_i[fifos_idx_lo];
        axil_mm2s_ofs_rdr_gp   : rdata_r = rxs_data_li[fifos_idx_lo];
        axil_mm2s_ofs_rdfo_gp  : rdata_r = {rx_occupancy_lo[fifos_idx_lo][lg_fifo_els_lp-1:2],2'b00};
        axil_mm2s_ofs_rlr_gp   : rdata_r = rdata_batch_v[fifos_idx_lo] ? rx_bytes_lp : 0;
        mcl_ofs_rcvfv_gp       : rdata_r = rcv_vacancy_o[fifos_idx_lo];
        mcl_ofs_edp_credits_gp : rdata_r = mcl_credits_i;  // axil base sub spaces are mapped to the same mcl endpoint out credits
        default                : rdata_r = fifo_width_p'(32'hBEEF_DEAD);
      endcase
    end : rd_regs

  end : many


  // ----------------------------------
  // rom address
  // ----------------------------------

  assign rom_base_hit = fifo_index_li == (rom_base_addr_gp>>base_addr_width_gp);
  assign rom_addr_o = rom_addr_width_p'(fifo_offset_li);

  // synopsys translate_off
  initial begin
    assert (rom_addr_width_p <= base_addr_width_gp)
      else $fatal("The range of axil base address is smaller than that of rom address. [%m]");
  end
  // synopsys translate_on

endmodule
