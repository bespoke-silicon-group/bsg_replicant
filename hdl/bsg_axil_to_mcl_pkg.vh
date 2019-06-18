`ifndef BSG_AXIL_TO_MCL_PKG_VH
`define BSG_AXIL_TO_MCL_PKG_VH

package cl_mcl_pkg;

  typedef struct packed {
     logic [39:0] padding;
     logic [7:0] pkt_type;
     logic [31:0] data;
     logic [31:0] load_id;
     logic [7:0] y_cord;
     logic [7:0] x_cord;
  } bsg_mcl_response_s;

  typedef union packed {
      logic [31:0]        data;
      logic [31:0]        load_id;
  } bsg_mcl_packet_payload_u;

  typedef struct packed {
     logic [15:0] padding;
     logic [31:0] addr;
     logic [7:0] op;
     logic [7:0] op_ex;
     bsg_mcl_packet_payload_u payload;
     logic [7:0] src_y_cord;
     logic [7:0] src_x_cord;
     logic [7:0] y_cord;
     logic [7:0] x_cord;
  } bsg_mcl_request_s;

  // the base addresses of axil crossbar should be aligned to its subspace, whose size is the power of 2
  parameter axil_m_fifo_base_addr_p = 64'h00000000_00000000;
  parameter axil_s_fifo_base_addr_p = 64'h00000000_00001000;
  parameter axil_mon_base_addr_p = 64'h00000000_00002000;

  parameter HOST_RCV_VACANCY_MC_REQ_p = 32'h100;
  parameter HOST_RCV_VACANCY_MC_RES_p = 32'h200;
  parameter HOST_REQ_CREDITS_p = 32'h300;

  parameter FIFO_ISR_TC_BIT_p = 27;
 
  parameter axil_fifo_els_p = 256;
  parameter rcv_fifo_els_p = 64; // make the rx fifo and rcv fifo have equal size

  // local parameters that should not change for current design
  localparam num_endpoint_lp = 1;
  localparam mc_fifo_width_lp = 128;
  localparam axil_data_width_lp = 32;
  localparam axil_addr_width_lp = 32;
  localparam base_addr_width_p = 12;

  localparam axil_base_addr_p = axil_addr_width_lp'(axil_m_fifo_base_addr_p);

  localparam ofs_isr_lp  = 8'h0 ; // Interrupt Status Register
  localparam ofs_tdfv_lp = 8'hC ; // Transmit FIFO Vacancy
  localparam ofs_tdr_lp  = 8'h10; // Transmit Data Destination Register
  localparam ofs_rdfo_lp = 8'h1C; // Receive Data FIFO occupancy
  localparam ofs_rdr_lp  = 8'h20; // Receive Data Destination Register
  localparam ofs_rlr_lp  = 8'h24; // Receive Length Register
  localparam fifo_rlr_words_lp = 4; // retuen RLR is fixed, 4x4 bytes

  localparam index_addr_width_lp = (axil_addr_width_lp-base_addr_width_p);

  localparam ofs_rsp_rcv_vacancy_lp = base_addr_width_p'(HOST_RCV_VACANCY_MC_RES_p);
  localparam ofs_req_rcv_vacancy_lp = base_addr_width_p'(HOST_RCV_VACANCY_MC_REQ_p);

endpackage : cl_mcl_pkg

`endif // BSG_AXIL_TO_MCL_PKG_VH
