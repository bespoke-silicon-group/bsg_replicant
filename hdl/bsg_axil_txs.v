/*
* bsg_axil_txs.v
*
*/

`include "bsg_axil_to_mcl_pkg.vh"

module bsg_axil_txs
  import import bsg_axil_to_mcl_pkg::*;
#(parameter num_fifos_p = "inv") (
  input                          clk_i
  ,input                          reset_i
  // axil tx channel
  ,input  [           31:0]       awaddr_i
  ,input                          awvalid_i
  ,output                         awready_o
  ,input  [           31:0]       wdata_i
  ,input  [            3:0]       wstrb_i
  ,input                          wvalid_i
  ,output                         wready_o
  ,output [            1:0]       bresp_o
  ,output                         bvalid_o
  ,input                          bready_i
  // fifo
  ,output [num_fifos_p-1:0][31:0] txs_o
  ,output [num_fifos_p-1:0]       txs_v_o
  ,input  [num_fifos_p-1:0]       txs_ready_i
  // clear the tx complete bit of isr
  ,output [num_fifos_p-1:0]       clr_isrs_txc_o
);

  // --------------------------------------------
  // axil write state machine
  // --------------------------------------------

  // Although the awvalid and wvalid can be asserted at the same cycle,
  // we assume they come in series events, for simplicity.
  typedef enum bit [1:0] {
    E_WR_IDLE = 2'd0,
    E_WR_ADDR = 2'd1,
    E_WR_DATA = 2'd2,
    E_WR_RESP = 2'd3
  } wr_state_e;

  wr_state_e wr_state_r, wr_state_n;

  logic write_bresp_lo;

  always_comb begin
    wr_state_n = wr_state_r;
    case (wr_state_r)
      E_WR_IDLE : begin
        if (awvalid_i)
          wr_state_n = E_WR_ADDR;
      end

      E_WR_ADDR : begin
        wr_state_n = E_WR_DATA;  // always ready to accept address
      end

      E_WR_DATA : begin
        if (wvalid_i & wready_o)
          wr_state_n = E_WR_RESP;
      end

      E_WR_RESP : begin
        if (bresp_o & bready_i)
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

  // tie unused signal
  wire [num_fifos_p-1:0] unused_fifo_ready_li = txs_ready_i;

  logic [num_fifos_p-1:0] base_addr_v;
  logic [num_fifos_p-1:0] fifo_addr_v;
  logic [num_fifos_p-1:0] isr_addr_v ;

  logic        in_wr_addr;
  logic        in_wr_data;
  logic        in_wr_resp;
  logic [31:0] wr_addr_r, wr_addr_n;
  logic        awready_n ;
  logic        wready_n  ;
  logic [ 1:0] bresp_n   ;
  logic        bvalid_n  ;

  wire tx_ready = 1'b1; // always ready for the write
  always_comb begin
    // wr state
    in_wr_addr = wr_state_r == E_WR_ADDR;
    in_wr_data = wr_state_r == E_WR_DATA;
    in_wr_resp = wr_state_r == E_WR_RESP;
    // waddr channel
    awready_n  = in_wr_addr;
    wready_n   = in_wr_data & tx_ready;
    wr_addr_n  = (awvalid_i & awready_o) ? awaddr_i : wr_addr_r;
    // write response
    bresp_n    = !(|base_addr_v) & (in_wr_data | in_wr_resp) ? 2'b11 : '0; // DECERR or OKAY
    bvalid_n   = in_wr_resp & bready_i;
  end

  assign awready_o = awready_n;
  assign wready_o  = wready_n;
  assign bresp_o   = bresp_n;
  assign bvalid_o  = bvalid_n;

  always_ff @(posedge clk_i) begin
    if (reset_i) begin
      wr_addr_r <= '0;
    end
    else begin
      wr_addr_r <= wr_addr_n;
    end
  end

  for (genvar i=0; i<num_fifos_p; i++) begin : wr_addr_hit
    // axil wr addr hits the slot base address
    assign base_addr_v[i] = in_wr_data
      && (wr_addr_r[axil_base_addr_width_gp+:axil_slot_idx_width_gp]
        == axil_slot_idx_width_gp'(i+(axil_m_slot_addr_gp>>axil_base_addr_width_gp)));
    // wr command is transmission data register
    assign fifo_addr_v[i] = base_addr_v[i]
      && (wr_addr_r[0+:axil_base_addr_width_gp]
        == axil_base_addr_width_gp'(axil_mm2s_ofs_tdr_gp));
    // wr command is interrupt status register
    assign isr_addr_v[i]  = base_addr_v[i]
      && (wr_addr_r[0+:axil_base_addr_width_gp]
        == axil_base_addr_width_gp'(axil_mm2s_ofs_isr_gp));
  end : wr_addr_hit

  assign txs_v_o    = {num_fifos_p{wvalid_i&wready_o}} & fifo_addr_v;
  assign txs_o = {num_fifos_p{wdata_i}};
  assign clr_isrs_txc_o   = {num_fifos_p{wdata_i[isr_txc_bit_gp]}} & isr_addr_v;

endmodule
