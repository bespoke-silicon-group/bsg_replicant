/*
* bsg_axil_rxs.v
*
*/

`include "bsg_defines.v"
`include "bsg_axil_to_mcl_pkg.vh"

module bsg_axil_rxs
  import import bsg_axil_to_mcl_pkg::*;
#(parameter num_fifos_p = "inv") (
  input                          clk_i
  ,input                          reset_i
  // axil tx channel
  ,input  [           31:0]       araddr_i
  ,input                          arvalid_i
  ,output                         arready_o
  ,output [           31:0]       rdata_o
  ,output [            1:0]       rresp_o
  ,output                         rvalid_o
  ,input                          rready_i
  // fifo
  ,input  [num_fifos_p-1:0][31:0] rxs_i
  ,input  [num_fifos_p-1:0]       rxs_v_i
  ,output [num_fifos_p-1:0]       rxs_ready_o
  // read monitor registers and rom
  ,output [           31:0]       rd_addr_o
  ,input  [num_fifos_p-1:0][31:0] mm2s_regs_i
  ,input  [           31:0]       mcl_data_i
);

  // --------------------------------------------
  // axil read state machine
  // --------------------------------------------

  typedef enum bit [1:0] {
    E_RD_IDLE = 2'b0,
    E_RD_ADDR = 2'd1,
    E_RD_DATA = 2'd2
  } rd_state_e;

  rd_state_e rd_state_r, rd_state_n;

  always_comb begin
    rd_state_n = rd_state_r;
    case (rd_state_r)

      E_RD_IDLE : begin
        if (arvalid_i)
          rd_state_n = E_RD_ADDR;
      end

      E_RD_ADDR : begin
        rd_state_n = E_RD_DATA;  // always ready to accept address
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

  // tie unused signal
  wire [num_fifos_p-1:0] unused_fifo_valid_li = rxs_v_i;

  logic [num_fifos_p-1:0] base_addr_v;
  logic [num_fifos_p-1:0] fifo_addr_v;
  logic                   rom_addr_v ;

  logic        in_rd_addr;
  logic        in_rd_data;
  logic [31:0] rd_addr_r, rd_addr_n;
  logic        arready_n ;
  logic        rvalid_n  ;
  logic [ 1:0] rresp_n   ;

  wire rx_valid = 1'b1; // always valid for the read
  always_comb begin
    // rd state
    in_rd_addr = rd_state_r == E_RD_ADDR;
    in_rd_data = rd_state_r == E_RD_DATA;
    // raddr channel
    arready_n  = in_rd_addr;
    rvalid_n   = in_rd_data & rx_valid;
    rd_addr_n  = (arvalid_i & arready_o) ? araddr_i : rd_addr_r;
    // read response, raise DECERR if access address is out of range
    rresp_n    = ~(|base_addr_v | rom_addr_v) & in_rd_data ? 2'b11 : '0; // DECERR or OKAY
  end

  assign arready_o = arready_n;
  assign rvalid_o  = rvalid_n;
  assign rresp_o   = rresp_n;

  always_ff @(posedge clk_i) begin
    if (reset_i) begin
      rd_addr_r <= '0;
    end
    else begin
      rd_addr_r <= rd_addr_n;
    end
  end

  for (genvar i=0; i<num_fifos_p; i++) begin : rd_addr_hit
    // axil rd addr hits the slot base address
    assign base_addr_v[i] = in_rd_data
      && (rd_addr_r[axil_base_addr_width_gp+:axil_slot_idx_width_gp]
        == axil_slot_idx_width_gp'(i+(axil_m_slot_addr_gp>>axil_base_addr_width_gp)));
    // rd command is read data register
    assign fifo_addr_v[i] = base_addr_v[i]
      && (rd_addr_r[0+:axil_base_addr_width_gp]
        == axil_base_addr_width_gp'(axil_mm2s_ofs_rdr_gp));
  end : rd_addr_hit

  assign rxs_ready_o = {num_fifos_p{rvalid_o & rready_i}} & fifo_addr_v;

  // axil rd addr hits the monitor base address
  assign rom_addr_v = in_rd_data
    && (rd_addr_r[axil_base_addr_width_gp+:axil_slot_idx_width_gp]
      == axil_slot_idx_width_gp'(num_fifos_p+(axil_m_slot_addr_gp>>axil_base_addr_width_gp)));

  wire read_fifo = |fifo_addr_v; // get the fifo data when any fifo is ready

  logic [`BSG_SAFE_CLOG2(num_fifos_p)-1:0] rd_slot_addr_lo;

  // select the read data
  if (num_fifos_p == 1) begin : one_fifo
    assign rd_fifo_idx = fifo_addr_v;
    assign rdata_o = rom_addr_v ? mm2s_regs_i
                                : read_fifo ? rxs_i
                                            : mm2s_regs_i;
  end
  else begin : many_fifos
    bsg_encode_one_hot #(.width_p(num_fifos_p)) axil_slot_idx_encode (
      .i(base_addr_v)
      ,.addr_o(rd_slot_addr_lo)
      ,.v_o()
    );
    assign rdata_o = rom_addr_v ? mcl_data_i
                                : read_fifo ? rxs_i[rd_slot_addr_lo]
                                            : mm2s_regs_i[rd_slot_addr_lo];
  end

  assign rd_addr_o = fifo_addr_v ? '0 : rd_addr_r;

endmodule
