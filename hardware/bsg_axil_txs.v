/*
* bsg_axil_txs.v
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
*/

`include "bsg_manycore_link_to_axil_pkg.v"

module bsg_axil_txs
  import bsg_manycore_link_to_axil_pkg::*;
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
  // to fifo, valid only, data will be dropped if not ready
  ,output [num_fifos_p-1:0][31:0] txs_o
  ,output [num_fifos_p-1:0]       txs_v_o
  // ,input  [num_fifos_p-1:0]       txs_ready_i
  // clear the tx complete bit of isr
  ,output [num_fifos_p-1:0]       clr_isrs_txc_o
);

  // tie unused signal
  // wire [num_fifos_p-1:0] unused_fifo_ready_li = txs_ready_i;

  // --------------------------------------------
  // axil write state machine
  // --------------------------------------------

  // Although the awvalid and wvalid can be asserted at the same cycle,
  // we assume they come in series events, for simplicity.
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


  logic [num_fifos_p-1:0] base_addr_hit;
  logic [num_fifos_p-1:0] fifo_addr_v;
  logic [num_fifos_p-1:0] isr_addr_v ;
  logic wr_addr_error;

  logic [31:0] wr_addr_r, wr_addr_n;
  logic [ 1:0] bresp_r, bresp_n;

  wire in_wr_addr = wr_state_r == E_WR_ADDR;
  wire in_wr_data = wr_state_r == E_WR_DATA;
  wire in_wr_resp = wr_state_r == E_WR_RESP;

  always_comb begin
    // get write address
    wr_addr_n = (awvalid_i & awready_o) ? awaddr_i : wr_addr_r;
    // gen bus response
    wr_addr_error = !(|base_addr_hit);
    bresp_n   = (wvalid_i & wready_o) ? (wr_addr_error ? 2'b11 : 2'b00) : bresp_r; // DECERR or OKAY
  end

  always_ff @(posedge clk_i) begin
    if (reset_i) begin
      wr_addr_r <= '0;
      bresp_r <= '0;
    end
    else begin
      wr_addr_r <= wr_addr_n;
      bresp_r <= bresp_n;
    end
  end

  for (genvar i=0; i<num_fifos_p; i++) begin : wr_addr_hit
    // axil wr addr hits the slot base address
    assign base_addr_hit[i] = in_wr_data
      && (wr_addr_r[axil_base_addr_width_gp+:axil_slot_idx_width_gp]
        == axil_slot_idx_width_gp'(i+(axil_m_slot_addr_gp>>axil_base_addr_width_gp)));
    // is writing transmission data register
    assign fifo_addr_v[i] = base_addr_hit[i]
      && (wr_addr_r[0+:axil_base_addr_width_gp]
        == axil_base_addr_width_gp'(axil_mm2s_ofs_tdr_gp));
    // is writing interrupt status register
    assign isr_addr_v[i]  = base_addr_hit[i]
      && (wr_addr_r[0+:axil_base_addr_width_gp]
        == axil_base_addr_width_gp'(axil_mm2s_ofs_isr_gp));
  end : wr_addr_hit

  // output signals

  // axil side
  assign awready_o = in_wr_addr;
  assign wready_o  = in_wr_data;  //always ready for the write

  assign bresp_o   = bresp_r;
  assign bvalid_o  = in_wr_resp;

  // tx stream side
  assign txs_v_o = {num_fifos_p{wvalid_i & wready_o}} & fifo_addr_v;
  assign txs_o = {num_fifos_p{wdata_i}};

  // control
  assign clr_isrs_txc_o = {num_fifos_p{wdata_i[axil_mm2s_isr_txc_bit_gp]}} & isr_addr_v;

endmodule
