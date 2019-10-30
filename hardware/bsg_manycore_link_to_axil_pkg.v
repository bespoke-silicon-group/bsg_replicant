`ifndef BSG_MANYCORE_LINK_TO_AXIL_PKG_V
`define BSG_MANYCORE_LINK_TO_AXIL_PKG_V

`define declare_bsg_manycore_link_fifo_s(fifo_width_mp, data_width_mp, addr_width_mp, pkt_type_width_mp, load_id_width_mp, op_width_mp, op_ex_width_gp, x_cord_width_mp, y_cord_width_mp) \
\
  typedef struct packed { \
    logic [fifo_width_mp - pkt_type_width_mp - data_width_mp - \
          load_id_width_mp - y_cord_width_mp - x_cord_width_mp - \
          1:0]                    padding ; \
    logic [pkt_type_width_mp-1:0] pkt_type; \
    logic [    data_width_mp-1:0] data    ; \
    logic [ load_id_width_mp-1:0] load_id ; \
    logic [  y_cord_width_mp-1:0] y_cord  ; \
    logic [  x_cord_width_mp-1:0] x_cord  ; \
  } bsg_mcl_response_s; \
\
  typedef union packed { \
    logic [data_width_mp-1:0] data   ; \
    logic [data_width_mp-1:0] load_id; \
  } bsg_mcl_packet_payload_u; \
\
  typedef struct packed { \
    logic [fifo_width_mp - addr_width_mp - op_width_mp - \
          op_ex_width_gp - data_width_mp - 2*y_cord_width_mp - 2*x_cord_width_mp - \
          1:0]                    padding   ; \
    logic [  addr_width_mp-1:0]   addr      ; \
    logic [    op_width_mp-1:0]   op        ; \
    logic [ op_ex_width_gp-1:0]   op_ex     ; \
    bsg_mcl_packet_payload_u      payload   ; \
    logic [y_cord_width_mp-1:0]   src_y_cord; \
    logic [x_cord_width_mp-1:0]   src_x_cord; \
    logic [y_cord_width_mp-1:0]   y_cord    ; \
    logic [x_cord_width_mp-1:0]   x_cord    ; \
  } bsg_mcl_request_s


package bsg_manycore_link_to_axil_pkg;

  parameter mc_ep_fifo_els_gp = 4;

  // local parameters that should not change for current design
  parameter axil_fifo_els_gp = 256;
  parameter rcv_fifo_els_gp  = 64 ; // rx fifo and rcv fifo have equal size

  parameter mcl_num_fifos_gp      = 4  ;
  parameter mcl_fifo_width_gp     = 128;
  parameter mcl_data_width_gp     = 32 ;
  parameter mcl_addr_width_gp     = 32 ;
  parameter mcl_pkt_type_width_gp = 8  ;
  parameter mcl_op_width_gp       = 8  ;
  parameter mcl_op_ex_width_gp    = 8  ;
  parameter mcl_x_cord_width_gp   = 8  ;
  parameter mcl_y_cord_width_gp   = 8  ;

  localparam mcl_load_id_width_gp = mcl_data_width_gp;

  parameter axil_data_width_gp = 32;

  parameter base_addr_width_gp = 12           ;
  parameter rom_base_addr_gp   = 32'h0000_0000;
  parameter fifo_base_addr_gp  = 32'h0000_1000; // the 2nd fifo address += (1<<base_addr_width_gp)

  localparam fifo_idx_width_gp = 32 - base_addr_width_gp;
  // note: slot_idx_width + base_addr_width <= 32 (axil addr width)
  // use '=' condition here to strictly check the address is hit or not

  parameter axil_rx_addr_width_gp = 8;
  // tx and rx registers
  parameter axil_mm2s_ofs_isr_gp  = 8'h00; // Interrupt Status Register
  parameter axil_mm2s_ofs_tdfv_gp = 8'h0C; // Transmit FIFO word Vacancy
  parameter axil_mm2s_ofs_tdr_gp  = 8'h10; // Transmit Data Destination Register
  // parameter axil_mm2s_ofs_tlr_gp  = 8'h14; // Transmit Length Register, not used
  parameter axil_mm2s_ofs_rdfo_gp = 8'h1C; // Receive Data FIFO word occupancy
  parameter axil_mm2s_ofs_rdr_gp  = 8'h20; // Receive Data Destination Register
  parameter axil_mm2s_ofs_rlr_gp  = 8'h24; // Receive Length Register

  parameter mcl_ofs_rcvfv_gp       = 8'h2C; // Receive fifo vacancy of the mcl receive fifo
  parameter mcl_ofs_edp_credits_gp = 8'h30; // out credits of the mcl endpoint standard
  // 2 base address are mapped to 1 endpoint standard

  // monitor registers
  // localparam host_rcv_vacancy_mc_req_gp = base_addr_width_gp'(32'h100);
  // localparam host_rcv_vacancy_mc_res_gp = base_addr_width_gp'(32'h200);
  // localparam host_req_credits_out_gp    = base_addr_width_gp'(32'h300);

endpackage : bsg_manycore_link_to_axil_pkg

`endif // BSG_MANYCORE_LINK_TO_AXIL_PKG_V
