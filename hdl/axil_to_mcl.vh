`ifndef AXIL_TO_MCL_VH
`define AXIL_TO_MCL_VH

`define declare_bsg_mcl_packet_s  						   \
    typedef struct packed {                                \
       logic [39:0] padding;                               \
       logic [7:0] pkt_type;                               \
       logic [31:0] data;                                  \
       logic [31:0] load_id;                               \
       logic [7:0] y_cord;                                 \
       logic [7:0] x_cord;                                 \
    } bsg_mcl_return_packet_s;                             \
                                                           \
    typedef union packed {                                 \
        logic [31:0]        data;                          \
        logic [31:0]        load_id;                       \
    } bsg_mcl_packet_payload_u;                            \
                                                           \
    typedef struct packed {                                \
       logic [15:0] padding;                               \
       logic [31:0] addr;                                  \
       logic [7:0] op;                                     \
       logic [7:0] op_ex;                                  \
       bsg_mcl_packet_payload_u payload;                   \
       logic [7:0] src_y_cord;                             \
       logic [7:0] src_x_cord;                             \
       logic [7:0] y_cord;                                 \
       logic [7:0] x_cord;                                 \
	} bsg_mcl_packet_s

`endif // AXIL_TO_MCL_VH

