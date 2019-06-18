/*
* bsg_axil_to_fifos_tx.v
*
*
*/

`include "bsg_axil_to_mcl_pkg.vh"

module bsg_axil_to_fifos_tx
import cl_mcl_pkg::*;
#(
  parameter num_fifos_p = "inv"
) (
  input                                      clk_i
  ,
  input                                      reset_i
  ,
  input  [           31:0]                   awaddr_i
  ,
  input                                      awvalid_i
  ,
  output                                     awready_o
  ,
  input  [           31:0]                   wdata_i
  ,
  input  [            3:0]                   wstrb_i
  ,
  input                                      wvalid_i
  ,
  output                                     wready_o
  ,
  output [            1:0]                   bresp_o
  ,
  output                                     bvalid_o
  ,
  input                                      bready_i
  // fifos
  ,
  output [num_fifos_p-1:0][31:0] tx_data_o
  ,
  output [num_fifos_p-1:0]                   tx_v_o
  ,
  input  [num_fifos_p-1:0]                   tx_ready_i
  ,
  output [num_fifos_p-1:0]                   clear_isr_tc_o  // clear the tx complete bit of isr
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

  always_ff @(posedge clk_i)
    if (reset_i)
      wr_state_r <= E_WR_IDLE;
    else
      wr_state_r <= wr_state_n;


  logic write_bresp_lo;

  always_comb begin

    wr_state_n = wr_state_r;

    case (wr_state_r)

      E_WR_IDLE :
        begin
          if (awvalid_i)
            wr_state_n = E_WR_ADDR;
        end

      E_WR_ADDR :
        begin
          wr_state_n = E_WR_DATA;  // always ready to accept address
        end

      E_WR_DATA :
        begin
          if (wvalid_i & wready_o)
            wr_state_n = E_WR_RESP;
        end

      E_WR_RESP :
        begin
          if (bresp_o & bready_i)
            wr_state_n = E_WR_IDLE;
        end

      default : wr_state_n = E_WR_IDLE;

    endcase
  end

  wire [num_fifos_p-1:0] unused_fifo_ready_li = tx_ready_i;
  wire                   tx_ready             = 1'b1        ; // always ready for the write

  logic [num_fifos_p-1:0] base_addr_v;
  logic [num_fifos_p-1:0] fifo_addr_v;
  logic [num_fifos_p-1:0] isr_addr_v;

  logic        in_wr_addr;
  logic        in_wr_data;
  logic        in_wr_resp;
  logic [31:0] wr_addr_r, wr_addr_n;
  logic        awready_n ;
  logic        wready_n  ;
  logic [ 1:0] bresp_n   ;
  logic        bvalid_n  ;

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
  assign wready_o = wready_n;
  assign bresp_o = bresp_n;
  assign bvalid_o = bvalid_n;


  always_ff @(posedge clk_i) begin
    if (reset_i) begin
      wr_addr_r <= '0;
    end
    else begin
      wr_addr_r <= wr_addr_n;
    end
  end

  for (genvar i=0; i<num_fifos_p; i++) begin : axi_wr_addr
    assign base_addr_v[i] = in_wr_data && (wr_addr_r[base_addr_width_p+:index_addr_width_lp] == index_addr_width_lp'(i+(axil_base_addr_p>>base_addr_width_p)));
    assign fifo_addr_v[i] = base_addr_v[i] && (wr_addr_r[0+:base_addr_width_p] == base_addr_width_p'(ofs_tdr_lp));
    assign isr_addr_v[i]  = base_addr_v[i] && (wr_addr_r[0+:base_addr_width_p] == base_addr_width_p'(ofs_isr_lp));
  end

  assign tx_v_o    = {num_fifos_p{wvalid_i&wready_o}} & fifo_addr_v;
  assign tx_data_o = {num_fifos_p{wdata_i}};

  assign clear_isr_o = isr_addr_v & wdata_i[FIFO_ISR_TC_BIT_p];

endmodule

