`ifndef AXIL_TO_MCL_VH
`define AXIL_TO_MCL_VH

`define declare_bsg_mcl_response_s                          \
    typedef struct packed {                                 \
       logic [39:0] padding;                                \
       logic [7:0] pkt_type;                                \
       logic [31:0] data;                                   \
       logic [31:0] load_id;                                \
       logic [7:0] y_cord;                                  \
       logic [7:0] x_cord;                                  \
    } bsg_mcl_response_s


`define declare_bsg_mcl_request_s                           \
    typedef union packed {                                  \
        logic [31:0]        data;                           \
        logic [31:0]        load_id;                        \
    } bsg_mcl_packet_payload_u;                             \
                                                            \
    typedef struct packed {                                 \
       logic [15:0] padding;                                \
       logic [31:0] addr;                                   \
       logic [7:0] op;                                      \
       logic [7:0] op_ex;                                   \
       bsg_mcl_packet_payload_u payload;                    \
       logic [7:0] src_y_cord;                              \
       logic [7:0] src_x_cord;                              \
       logic [7:0] y_cord;                                  \
       logic [7:0] x_cord;                                  \
  } bsg_mcl_request_s


package cl_mcl_pkg;

  // the base addresses of axil crossbar should be aligned to its subspace, whose size is the power of 2
  parameter axil_m_fifo_base_addr_p = 64'h00000000_00000000;
  parameter axil_s_fifo_base_addr_p = 64'h00000000_00001000;
  parameter axil_mon_base_addr_p = 64'h00000000_00002000;

  parameter HOST_RCV_VACANCY_MC_REQ_p = 32'h100;
  parameter HOST_RCV_VACANCY_MC_RES_p = 32'h200;
  parameter HOST_REQ_CREDITS_p = 32'h300;

	parameter FIFO_ISR_TC_BIT_p = 27;

endpackage : cl_mcl_pkg

`endif // AXIL_TO_MCL_VH
