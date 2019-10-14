`ifndef BSG_MANYCORE_LINK_TO_AXIL_PKG_V
`define BSG_MANYCORE_LINK_TO_AXIL_PKG_V

package bsg_manycore_link_to_axil_pkg;

  typedef struct packed {
    logic [39:0] padding ;
    logic [ 7:0] pkt_type;
    logic [31:0] data    ;
    logic [31:0] load_id ;
    logic [ 7:0] y_cord  ;
    logic [ 7:0] x_cord  ;
  } bsg_mcl_response_s;

  typedef union packed {
    logic [31:0] data   ;
    logic [31:0] load_id;
  } bsg_mcl_packet_payload_u;

  typedef struct packed {
    logic [15:0]             padding   ;
    logic [31:0]             addr      ;
    logic [ 7:0]             op        ;
    logic [ 7:0]             op_ex     ;
    bsg_mcl_packet_payload_u payload   ;
    logic [ 7:0]             src_y_cord;
    logic [ 7:0]             src_x_cord;
    logic [ 7:0]             y_cord    ;
    logic [ 7:0]             x_cord    ;
  } bsg_mcl_request_s;

  // local parameters that should not change for current design
  parameter axil_fifo_els_gp = 256;
  parameter rcv_fifo_els_gp  = 64 ; // rx fifo and rcv fifo have equal size

  localparam num_endpoints_gp  = 1  ;
  localparam mcl_fifo_width_gp = 128;

  localparam axil_base_addr_width_gp = 12                          ;
  localparam axil_slot_idx_width_gp  = 32 - axil_base_addr_width_gp;
  // note: slot_idx_width + base_addr_width <= 32 (axil addr width)
  // use '=' condition here to strictly check the address is hit or not

  localparam axil_m_slot_addr_gp = 32'h0000_0000;
  // note: last axil_base_addr_width_gp bits must be 0, i.e. address is aligned
  // the following is slave slot, monitor slot ...

  // tx and rx registers
  localparam axil_mm2s_ofs_isr_gp  = 8'h00; // Interrupt Status Register
  localparam axil_mm2s_ofs_tdfv_gp = 8'h0C; // Transmit FIFO Vacancy
  localparam axil_mm2s_ofs_tdr_gp  = 8'h10; // Transmit Data Destination Register
  // localparam axil_mm2s_ofs_tlr_gp  = 8'h14; // Transmit Length Register, not used
  localparam axil_mm2s_ofs_rdfo_gp = 8'h1C; // Receive Data FIFO occupancy
  localparam axil_mm2s_ofs_rdr_gp  = 8'h20; // Receive Data Destination Register
  localparam axil_mm2s_ofs_rlr_gp  = 8'h24; // Receive Length Register

  localparam axil_mm2s_rlr_els_gp     = 4 ; // retuen RLR is fixed to 4
  localparam axil_mm2s_isr_txc_bit_gp = 27; // tx complete bit index

  // monitor registers
  localparam host_rcv_vacancy_mc_req_gp = axil_base_addr_width_gp'(32'h100);
  localparam host_rcv_vacancy_mc_res_gp = axil_base_addr_width_gp'(32'h200);
  localparam host_req_credits_out_gp    = axil_base_addr_width_gp'(32'h300);


endpackage : bsg_manycore_link_to_axil_pkg

`endif // BSG_MANYCORE_LINK_TO_AXIL_PKG_V