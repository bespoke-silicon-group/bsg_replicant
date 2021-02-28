
`include "bp_common_defines.svh"

module bp_cce_to_mc_bridge
 import bp_common_pkg::*;
 import bsg_manycore_pkg::*;
 #(parameter bp_params_e bp_params_p = e_bp_default_cfg
   `declare_bp_proc_params(bp_params_p)
   `declare_bp_bedrock_mem_if_widths(paddr_width_p, word_width_gp, lce_id_width_p, lce_assoc_p, cce)

   , parameter host_enable_p                   = 0

   , parameter mc_max_outstanding_p            = "inv"
   , parameter mc_x_cord_width_p               = "inv"
   , parameter mc_x_subcord_width_p            = "inv"
   , parameter mc_y_cord_width_p               = "inv"
   , parameter mc_y_subcord_width_p            = "inv"
   , parameter mc_data_width_p                 = "inv"
   , parameter mc_addr_width_p                 = "inv"
   , parameter mc_vcache_block_size_in_words_p = "inv"
   , parameter mc_vcache_size_p                = "inv"
   , parameter mc_vcache_sets_p                = "inv"
   , parameter mc_num_tiles_x_p                = "inv"
   , parameter mc_num_tiles_y_p                = "inv"

   , localparam mc_packet_width_lp      = `bsg_manycore_packet_width(mc_addr_width_p, mc_data_width_p, mc_x_cord_width_p, mc_y_cord_width_p)
   , localparam mc_link_sif_width_lp    = `bsg_manycore_link_sif_width(mc_addr_width_p, mc_data_width_p, mc_x_cord_width_p, mc_y_cord_width_p)
   )
  (input                                      clk_i
   , input                                    reset_i

   , input [cce_mem_msg_width_lp-1:0]         io_cmd_i
   , input                                    io_cmd_v_i
   , output logic                             io_cmd_ready_o

   , output logic [cce_mem_msg_width_lp-1:0]  io_resp_o
   , output logic                             io_resp_v_o
   , input                                    io_resp_yumi_i

   , output logic [cce_mem_msg_width_lp-1:0]  io_cmd_o
   , output logic                             io_cmd_v_o
   , input                                    io_cmd_yumi_i

   , input [cce_mem_msg_width_lp-1:0]         io_resp_i
   , input                                    io_resp_v_i
   , output logic                             io_resp_ready_o

   , input [mc_link_sif_width_lp-1:0]         link_sif_i
   , output logic [mc_link_sif_width_lp-1:0]  link_sif_o

   , input [mc_x_cord_width_p-1:0]            my_x_i
   , input [mc_y_cord_width_p-1:0]            my_y_i
   );

  `declare_bp_bedrock_mem_if(paddr_width_p, word_width_gp, lce_id_width_p, lce_assoc_p, cce);
  `declare_bsg_manycore_packet_s(mc_addr_width_p, mc_data_width_p, mc_x_cord_width_p, mc_y_cord_width_p);
  `bp_cast_i(bp_bedrock_cce_mem_msg_s, io_cmd);
  `bp_cast_o(bp_bedrock_cce_mem_msg_s, io_resp);
  `bp_cast_o(bp_bedrock_cce_mem_msg_s, io_cmd);
  `bp_cast_i(bp_bedrock_cce_mem_msg_s, io_resp);

  // TODO: This should be set in bsg_replicant
  typedef struct packed
  {
    logic [15:0] reserved;
    logic [31:0] addr;
    logic [7:0]  op_v2;
    logic [7:0]  reg_id;
    logic [31:0] payload;
    logic [7:0]  y_src;
    logic [7:0]  x_src;
    logic [7:0]  y_dst;
    logic [7:0]  x_dst;
  }  host_request_packet_s;

  typedef struct packed
  {
    logic [63:0] reserved;
    logic [7:0]  op_v2;
    logic [31:0] payload;
    logic [7:0]  reg_id;
    logic [7:0]  y_dst;
    logic [7:0]  x_dst;
  }  host_response_packet_s;

  // BP EPA Map
  // dev: 0 -- CFG
  //      1 -- CLINT
  typedef struct packed
  {
    logic [3:0]  dev;
    logic [11:0] addr;
  } bp_epa_s;

  localparam mc_link_bp_req_fifo_addr_gp     = 20'h0_1000;
  localparam mc_link_bp_req_credits_addr_gp  = 20'h0_2000;
  localparam mc_link_bp_resp_fifo_addr_gp    = 20'h0_3000;
  localparam mc_link_bp_resp_entries_addr_gp = 20'h0_4000;
  localparam mc_link_mc_req_fifo_addr_gp     = 20'h0_5000;
  localparam mc_link_mc_req_entries_addr_gp  = 20'h0_6000;

  bp_bedrock_cce_mem_msg_s io_cmd_li;
  bsg_manycore_global_addr_s io_cmd_eva_li;
  logic io_cmd_v_li, io_cmd_yumi_lo;
  bsg_fifo_1r1w_small
   #(.width_p($bits(bp_bedrock_cce_mem_msg_s)), .els_p(mc_max_outstanding_p))
   header_fifo
    (.clk_i(clk_i)
     ,.reset_i(reset_i)

     ,.data_i(io_cmd_cast_i)
     ,.v_i(io_cmd_v_i)
     ,.ready_o(io_cmd_ready_o)

     ,.data_o(io_cmd_li)
     ,.v_o(io_cmd_v_li)
     ,.yumi_i(io_cmd_yumi_lo)
     );
  assign io_cmd_eva_li = io_cmd_li.header.addr;

  logic                              in_v_lo;
  logic [mc_data_width_p-1:0]        in_data_lo;
  logic [(mc_data_width_p>>3)-1:0]   in_mask_lo;
  logic [mc_addr_width_p-1:0]        in_addr_lo;
  logic                              in_we_lo;
  bsg_manycore_load_info_s           in_load_info_lo;
  logic [mc_x_cord_width_p-1:0]      in_src_x_cord_lo;
  logic [mc_y_cord_width_p-1:0]      in_src_y_cord_lo;
  logic                              in_yumi_li;

  logic [mc_data_width_p-1:0]        returning_data_li;
  logic                              returning_v_li;

  logic                              out_v_li;
  bsg_manycore_packet_s              out_packet_li;
  logic                              out_ready_lo;

  logic [mc_data_width_p-1:0]        returned_data_r_lo;
  logic [4:0]                        returned_reg_id_r_lo;
  logic                              returned_v_r_lo, returned_yumi_li;
  bsg_manycore_return_packet_type_e  returned_pkt_type_r_lo;
  logic                              returned_fifo_full_lo;
  logic                              returned_credit_v_r_lo;
  logic [4:0]                        returned_credit_reg_id_r_lo;

  logic [3:0]                        out_credits_lo;

  bsg_manycore_endpoint_standard
   #(.x_cord_width_p(mc_x_cord_width_p)
     ,.y_cord_width_p(mc_y_cord_width_p)
    ,.fifo_els_p(2)
    ,.data_width_p(mc_data_width_p)
    ,.addr_width_p(mc_addr_width_p)

    ,.max_out_credits_p(15)
    ,.warn_out_of_credits_p(0)
    ,.debug_p(1)
    )
   blackparrot_endpoint
   (.clk_i(clk_i)
    ,.reset_i(reset_i)

    ,.link_sif_i(link_sif_i)
    ,.link_sif_o(link_sif_o)

    //--------------------------------------------------------
    // 1. in_request signal group
    ,.in_v_o(in_v_lo)
    ,.in_data_o(in_data_lo)
    ,.in_mask_o(in_mask_lo)
    ,.in_addr_o(in_addr_lo)
    ,.in_we_o(in_we_lo)
    ,.in_load_info_o(in_load_info_lo)
    ,.in_src_x_cord_o(in_src_x_cord_lo)
    ,.in_src_y_cord_o(in_src_y_cord_lo)
    ,.in_yumi_i(in_yumi_li)

    //--------------------------------------------------------
    // 2. out_response signal group
    //    responses that will send back to the network
    ,.returning_data_i(returning_data_li)
    ,.returning_v_i(returning_v_li)

    //--------------------------------------------------------
    // 3. out_request signal group
    //    request that will send to the network
    ,.out_v_i(out_v_li)
    ,.out_packet_i(out_packet_li)
    ,.out_credit_or_ready_o(out_ready_lo)

    //--------------------------------------------------------
    // 4. in_response signal group
    //    responses that send back from the network
    //    the node shold always be ready to receive this response.
    ,.returned_data_r_o(returned_data_r_lo)
    ,.returned_reg_id_r_o(returned_reg_id_r_lo)
    ,.returned_v_r_o(returned_v_r_lo)
    ,.returned_pkt_type_r_o(returned_pkt_type_r_lo)
    ,.returned_yumi_i(returned_yumi_li)
    ,.returned_fifo_full_o()

    ,.returned_credit_v_r_o(returned_credit_v_r_lo)
    ,.returned_credit_reg_id_r_o(returned_credit_reg_id_r_lo)

    ,.out_credits_o(out_credits_lo)

    ,.global_x_i(my_x_i)
    ,.global_y_i(my_y_i)
    );

  // DRAM hash function
  localparam vcache_word_offset_width_lp = `BSG_SAFE_CLOG2(mc_vcache_block_size_in_words_p);
  localparam hash_bank_input_width_lp = mc_data_width_p-1-2-vcache_word_offset_width_lp;
  localparam hash_bank_index_width_lp = $clog2(((2**hash_bank_input_width_lp)+(2*mc_num_tiles_x_p)-1)/(mc_num_tiles_x_p*2));

  logic [mc_x_subcord_width_p:0] hash_bank_lo;  // {bot_not_top, x_cord}
  logic [hash_bank_index_width_lp-1:0] hash_bank_index_lo;
  wire [hash_bank_input_width_lp-1:0] hash_bank_input = io_cmd_eva_li[2+vcache_word_offset_width_lp+:hash_bank_input_width_lp];
  hash_function
   #(.banks_p(mc_num_tiles_x_p*2)
     ,.width_p(hash_bank_input_width_lp)
     ,.vcache_sets_p(mc_vcache_sets_p)
     )
   hashb
    (.i(hash_bank_input)
     ,.bank_o(hash_bank_lo)
     ,.index_o(hash_bank_index_lo)
     );

  logic [(mc_data_width_p>>3)-1:0] store_mask;
  always_comb
    case (io_cmd_li.header.size)
       e_bedrock_msg_size_1: store_mask = 4'h1 << io_cmd_eva_li.low_bits;
       e_bedrock_msg_size_2: store_mask = 4'h3 << io_cmd_eva_li.low_bits;
       default:              store_mask = 4'hf << io_cmd_eva_li.low_bits;
    endcase

  localparam trans_id_width_lp = `BSG_SAFE_CLOG2(mc_max_outstanding_p);
  logic [trans_id_width_lp-1:0] trans_id_lo;
  logic trans_id_v_lo, trans_id_yumi_li;
  logic [mc_data_width_p-1:0] mmio_resp_data_lo;
  logic [trans_id_width_lp-1:0] mmio_resp_id_lo;
  logic mmio_resp_v_lo, mmio_resp_yumi_li;
  logic mmio_returned_v_li;

  wire [(mc_data_width_p>>3)-1:0] returned_id_li = returned_v_r_lo ? returned_reg_id_r_lo : returned_credit_reg_id_r_lo;
  logic [bsg_manycore_reg_id_width_gp-1:0] mmio_returned_reg_id_li;
  bsg_manycore_reg_id_decode
   #(.data_width_p(mc_data_width_p))
   reg_id_decode
    (.data_i(returned_data_r_lo)
     ,.mask_i(returned_id_li)
     ,.reg_id_o(mmio_returned_reg_id_li)
     );

  wire [mc_data_width_p-1:0] mmio_returned_data_li = returned_data_r_lo;
  bsg_fifo_reorder
   #(.width_p(mc_data_width_p), .els_p(mc_max_outstanding_p))
   return_data_fifo
    (.clk_i(clk_i)
     ,.reset_i(reset_i)

     ,.fifo_alloc_id_o(trans_id_lo[0+:trans_id_width_lp])
     ,.fifo_alloc_v_o(trans_id_v_lo)
     ,.fifo_alloc_yumi_i(trans_id_yumi_li)

     // We write an entry on credit return in order to determine when to send
     //   back a store response.  A little inefficent, but allocating storage for
     //   worst case (all loads) isn't unreasonable
     ,.write_id_i(mmio_returned_reg_id_li[0+:trans_id_width_lp])
     ,.write_data_i(mmio_returned_data_li)
     ,.write_v_i(mmio_returned_v_li)

     ,.fifo_deq_data_o(mmio_resp_data_lo)
     ,.fifo_deq_id_o(mmio_resp_id_lo)
     ,.fifo_deq_v_o(mmio_resp_v_lo)
     ,.fifo_deq_yumi_i(mmio_resp_yumi_li)

     ,.empty_o()
     );

  bp_bedrock_cce_mem_msg_header_s mmio_header_lo;
  bsg_mem_1r1w
   #(.width_p($bits(io_cmd_li.header)), .els_p(mc_max_outstanding_p))
   return_headers
    (.w_clk_i(clk_i)
     ,.w_reset_i(reset_i)

     ,.w_v_i(trans_id_yumi_li)
     ,.w_addr_i(trans_id_lo)
     ,.w_data_i(io_cmd_li.header)

     ,.r_v_i(mmio_resp_yumi_li)
     ,.r_addr_i(mmio_resp_id_lo)
     ,.r_data_o(mmio_header_lo)
     );
  bp_bedrock_cce_mem_msg_s mmio_resp_lo;
  assign mmio_resp_lo = '{header: mmio_header_lo, data: mmio_resp_data_lo};

  logic [mc_data_width_p-1:0] store_payload;
  logic [bsg_manycore_reg_id_width_gp-1:0] store_reg_id;
  bsg_manycore_packet_op_e store_op;
  bsg_manycore_reg_id_encode
   #(.data_width_p(mc_data_width_p))
   reg_id_encode
    (.data_i(io_cmd_li.data[0+:mc_data_width_p])
     ,.mask_i(store_mask)
     ,.reg_id_i(bsg_manycore_reg_id_width_gp'(trans_id_lo))

     ,.data_o(store_payload)
     ,.reg_id_o(store_reg_id)
     ,.op_o(store_op)
     );

  bsg_manycore_packet_s mmio_out_packet_li;
  always_comb
    begin
      mmio_out_packet_li = '0;
      mmio_out_packet_li.src_y_cord = my_y_i;
      mmio_out_packet_li.src_x_cord = my_x_i;
      if (io_cmd_eva_li.remote == '0)
        begin
          mmio_out_packet_li.addr                             = io_cmd_eva_li.addr;
          mmio_out_packet_li.y_cord                           = io_cmd_eva_li.y_cord;
          mmio_out_packet_li.x_cord                           = io_cmd_eva_li.x_cord;
        end
      else
        begin
          mmio_out_packet_li.addr = {
            1'b0,
            {(mc_addr_width_p-1-vcache_word_offset_width_lp-hash_bank_index_width_lp){1'b0}},
            hash_bank_index_lo,
            io_cmd_eva_li[2+:vcache_word_offset_width_lp]
          };
          mmio_out_packet_li.y_cord = hash_bank_lo[mc_x_subcord_width_p]
            ? (mc_y_cord_width_p)'(mc_num_tiles_y_p+1) // DRAM ports are directly below the manycore tiles.
            : {mc_y_cord_width_p{1'b0}};
          mmio_out_packet_li.x_cord = hash_bank_lo[0+:mc_x_subcord_width_p];
        end

        case (io_cmd_li.header.msg_type)
          e_bedrock_mem_uc_rd, e_bedrock_mem_rd:
            begin
              mmio_out_packet_li.op_v2                                    = e_remote_load;
              mmio_out_packet_li.payload.load_info_s.load_info.is_byte_op = (io_cmd_li.header.size == e_bedrock_msg_size_1);
              mmio_out_packet_li.payload.load_info_s.load_info.is_hex_op  = (io_cmd_li.header.size == e_bedrock_msg_size_2);
              mmio_out_packet_li.payload.load_info_s.load_info.part_sel   = io_cmd_eva_li.low_bits;
              mmio_out_packet_li.reg_id                                   = bsg_manycore_reg_id_width_gp'(trans_id_lo);
            end
          default: // e_bedrock_mem_uc_wr, e_bedrock_mem_wr:
            begin
              mmio_out_packet_li.op_v2                                    = store_op;
              mmio_out_packet_li.payload.data                             = store_payload;
              mmio_out_packet_li.reg_id                                   = store_reg_id;
            end
        endcase
    end

  //////////////////////////////////////////////
  // Host Interface
  //////////////////////////////////////////////
  logic bp_to_mc_v_li, bp_to_mc_ready_lo;
  host_request_packet_s bp_to_mc_lo;
  logic bp_to_mc_v_lo, bp_to_mc_yumi_li;
  bsg_manycore_load_info_s bp_to_mc_load_info;

  host_response_packet_s mc_to_bp_response_li;
  logic mc_to_bp_response_v_li, mc_to_bp_response_ready_lo;
  logic [word_width_gp-1:0] mc_to_bp_response_data_lo;
  logic mc_to_bp_response_v_lo, mc_to_bp_response_yumi_li;

  host_request_packet_s mc_to_bp_request_li;
  logic mc_to_bp_request_v_li, mc_to_bp_request_ready_lo;
  logic [word_width_gp-1:0] mc_to_bp_request_data_lo;
  logic mc_to_bp_request_v_lo, mc_to_bp_request_yumi_li;

  bsg_manycore_packet_s bp_to_mc_out_packet_li;

  if (host_enable_p)
    begin : host
      wire [word_width_gp-1:0] bp_to_mc_data_li = io_cmd_li.data[0+:word_width_gp];
      bsg_serial_in_parallel_out_full
       #(.width_p(word_width_gp), .els_p($bits(host_request_packet_s)/word_width_gp))
       bp_to_mc_request_sipo
        (.clk_i(clk_i)
         ,.reset_i(reset_i)

         ,.data_i(bp_to_mc_data_li)
         ,.v_i(bp_to_mc_v_li)
         ,.ready_o(bp_to_mc_ready_lo)

         ,.data_o(bp_to_mc_lo)
         ,.v_o(bp_to_mc_v_lo)
         ,.yumi_i(bp_to_mc_yumi_li)
         );
      assign bp_to_mc_load_info = '{part_sel : io_cmd_li.header.addr[0+:2], default: '0};
      assign bp_to_mc_out_packet_li = '{addr       : bp_to_mc_lo.addr[2+:mc_addr_width_p]
                                        ,op_v2     : bsg_manycore_packet_op_e'(bp_to_mc_lo.op_v2)
                                        ,reg_id    : bp_to_mc_lo.reg_id
                                        ,payload   : (bp_to_mc_lo.op_v2 inside {e_remote_store, e_remote_sw})
                                                     ? bp_to_mc_lo.payload
                                                     : bp_to_mc_load_info
                                        ,src_y_cord: bp_to_mc_lo.y_src
                                        ,src_x_cord: bp_to_mc_lo.x_src
                                        ,y_cord    : bp_to_mc_lo.y_dst
                                        ,x_cord    : bp_to_mc_lo.x_dst
                                        };

      bsg_parallel_in_serial_out
       #(.width_p(word_width_gp), .els_p($bits(host_response_packet_s)/word_width_gp))
       mc_to_bp_response_piso
        (.clk_i(clk_i)
         ,.reset_i(reset_i)

         ,.data_i(mc_to_bp_response_li)
         ,.valid_i(mc_to_bp_response_v_li)
         ,.ready_and_o(mc_to_bp_response_ready_lo)

         ,.data_o(mc_to_bp_response_data_lo)
         ,.valid_o(mc_to_bp_response_v_lo)
         ,.yumi_i(mc_to_bp_response_yumi_li)
         );
      // We ignore the x dst and y dst of return packets
      // TODO: Support remote SW
      assign mc_to_bp_response_li = '{x_dst   : my_x_i
                                      ,y_dst  : my_y_i
                                      ,reg_id : returned_reg_id_r_lo
                                      ,payload: returned_data_r_lo
                                      ,op_v2  : returned_pkt_type_r_lo
                                      ,default: '0
                                      };

      bsg_parallel_in_serial_out
       #(.width_p(word_width_gp), .els_p($bits(host_request_packet_s)/word_width_gp))
       mc_to_bp_request_piso
        (.clk_i(clk_i)
         ,.reset_i(reset_i)

         ,.data_i(mc_to_bp_request_li)
         ,.valid_i(mc_to_bp_request_v_li)
         ,.ready_and_o(mc_to_bp_request_ready_lo)

         ,.data_o(mc_to_bp_request_data_lo)
         ,.valid_o(mc_to_bp_request_v_lo)
         ,.yumi_i(mc_to_bp_request_yumi_li)
         );
      assign mc_to_bp_request_li = '{x_dst    : my_x_i
                                     ,y_dst   : my_y_i
                                     ,x_src   : in_src_x_cord_lo
                                     ,y_src   : in_src_y_cord_lo
                                     ,payload : in_data_lo
                                     ,reg_id  : in_mask_lo
                                     ,op_v2   : e_remote_store
                                     ,addr    : in_addr_lo
                                     ,default : '0
                                     };
    end

  //////////////////////////////////////////////
  // Outgoing Request
  //////////////////////////////////////////////
  always_comb
    begin
      io_resp_cast_o = '0;
      io_resp_v_o = '0;
      io_cmd_yumi_lo = '0;

      bp_to_mc_v_li = '0;
      bp_to_mc_yumi_li = '0;
      mc_to_bp_request_yumi_li = '0;

      trans_id_yumi_li = '0;

      out_packet_li = '0;
      out_v_li = '0;

      mmio_returned_v_li = '0;
      mc_to_bp_response_v_li = '0;
      returned_yumi_li = '0;
      mmio_resp_yumi_li = '0;

      if (io_cmd_v_li & (io_cmd_eva_li == mc_link_bp_req_fifo_addr_gp) & host_enable_p)
        begin
          io_resp_cast_o = '{header: io_cmd_li.header, data: '0};
          io_resp_v_o    = bp_to_mc_ready_lo;
          io_cmd_yumi_lo = io_resp_yumi_i;

          bp_to_mc_v_li  = io_cmd_yumi_lo;
        end
      else if (io_cmd_v_li & (io_cmd_eva_li == mc_link_bp_req_credits_addr_gp) & host_enable_p)
        begin
          io_resp_cast_o = '{header: io_cmd_li.header, data: out_credits_lo};
          io_resp_v_o    = 1'b1;
          io_cmd_yumi_lo = io_resp_yumi_i;
        end
      else if (io_cmd_v_li & (io_cmd_eva_li == mc_link_bp_resp_fifo_addr_gp) & host_enable_p)
        begin
          io_resp_cast_o = '{header: io_cmd_li.header, data: mc_to_bp_response_data_lo};
          io_resp_v_o    = mc_to_bp_response_v_lo;
          io_cmd_yumi_lo = io_resp_yumi_i;

          mc_to_bp_response_yumi_li = io_cmd_yumi_lo;
        end
      else if (io_cmd_v_li & (io_cmd_eva_li == mc_link_bp_resp_entries_addr_gp) & host_enable_p)
        begin
          io_resp_cast_o = '{header: io_cmd_li.header, data: mc_to_bp_response_v_lo};
          io_resp_v_o    = 1'b1;
          io_cmd_yumi_lo = io_resp_yumi_i;
        end
      else if (io_cmd_v_li & (io_cmd_eva_li == mc_link_mc_req_fifo_addr_gp) & host_enable_p)
        begin
          io_resp_cast_o = '{header: io_cmd_li.header, data: mc_to_bp_request_data_lo};
          io_resp_v_o    = 1'b1;
          io_cmd_yumi_lo = io_resp_yumi_i;

          mc_to_bp_request_yumi_li = io_cmd_yumi_lo;
        end
      else if (io_cmd_v_li & (io_cmd_eva_li == mc_link_mc_req_entries_addr_gp) & host_enable_p)
        begin
          io_resp_cast_o = '{header: io_cmd_li.header, data: mc_to_bp_request_v_lo};
          io_resp_v_o    = 1'b1;
          io_cmd_yumi_lo = io_resp_yumi_i;
        end
      else // Not an incoming fifo request
        begin
          if (io_cmd_v_li)
            begin
              io_cmd_yumi_lo = trans_id_v_lo & out_ready_lo;
              trans_id_yumi_li = io_cmd_yumi_lo;
              out_v_li = trans_id_yumi_li;
              out_packet_li = mmio_out_packet_li;
            end

          if ((returned_v_r_lo & (returned_credit_reg_id_r_lo > mc_max_outstanding_p)))
            begin
              mc_to_bp_response_v_li = mc_to_bp_response_ready_lo;
              returned_yumi_li = mc_to_bp_response_v_li;
            end
          else
            begin
              // We can always ack mmio requests, because we've allocated space in the reorder fifo
              mmio_returned_v_li = returned_credit_v_r_lo;
              returned_yumi_li = returned_v_r_lo;
            end
        end

      // Send out host request opportunistically
      if (~trans_id_yumi_li)
        begin
          bp_to_mc_yumi_li = out_ready_lo & bp_to_mc_v_lo;
          out_v_li = bp_to_mc_yumi_li;
          out_packet_li = bp_to_mc_out_packet_li;
        end

      // Send out mmio response opportunistically
      if (~io_resp_v_o)
        begin
          io_resp_v_o = mmio_resp_v_lo;
          io_resp_cast_o = mmio_resp_lo;
          mmio_resp_yumi_li = io_resp_yumi_i;
        end
    end

  //////////////////////////////////////////////
  // Incoming packet
  //////////////////////////////////////////////
  bp_epa_s in_epa_li;
  assign in_epa_li = in_addr_lo;
  bp_bedrock_cce_mem_payload_s io_payload_lo;
  always_comb
    begin
      io_payload_lo = '0;
      io_payload_lo.lce_id = 2'b10;

      io_cmd_cast_o = '0;
      io_cmd_cast_o.header.msg_type = in_we_lo ? e_bedrock_mem_uc_wr : e_bedrock_mem_uc_rd;

      if (in_epa_li.dev == 1)
        io_cmd_cast_o.header.addr = clint_dev_base_addr_gp + in_epa_li.addr;
      else // if (in_epa_li.dev == 0)
        io_cmd_cast_o.header.addr = cfg_dev_base_addr_gp + in_epa_li.addr;

      // TODO: we only support 32-bit loads and stores to BP configuration addresses
      io_cmd_cast_o.header.size = e_bedrock_msg_size_4;
      io_cmd_cast_o.header.payload = io_payload_lo;
      io_cmd_cast_o.data = in_data_lo;

      io_cmd_v_o = in_v_lo;
      in_yumi_li = io_cmd_yumi_i;
    end

  //////////////////////////////////////////////
  // Return to incoming packet
  //////////////////////////////////////////////
  always_comb
    begin
      returning_data_li = (io_resp_cast_i.header.size == e_bedrock_msg_size_4)
                          ? io_resp_cast_i.data[0+:32]
                          : (io_resp_cast_i.header.size == e_bedrock_msg_size_2)
                            ? io_resp_cast_i.data[0+:16]
                            : io_resp_cast_i.data[0+:8];
      returning_v_li = io_resp_v_i;

      // Returning data is always "ready" (but please don't randomly respond)
      io_resp_ready_o = 1'b1;
    end

  ////synopsys translate_off
  //always_ff @(negedge clk_i)
  //  begin
  //    if (bp_to_mc_yumi_li)
  //      $display("[BP-LINK] Outgoing command: %p", bp_to_mc_lo);
  //    if (mc_to_bp_response_v_li)
  //      $display("[BP-LINK] Incoming response: %p", mc_to_bp_response_li);
  //  end
  ////synopsys translate_on

  ////synopsys translate_off
  //always_ff @(negedge clk_i)
  //  begin
  //    if (io_cmd_v_i)
  //      begin
  //        $display("[BP-LINK] Outgoing command: %p", io_cmd_li);
  //        $display("[      EVA] Outgoing EVA: %p", io_cmd_eva_li);
  //        $display("[   PACKET] Outgoing packet: %p", out_packet_li);
  //      end
  //    if (io_resp_yumi_i)
  //      $display("[BP-LINK] Incoming response: %p", io_resp_cast_o);
  //  end
  ////synopsys translate_on

  //`declare_bp_bedrock_mem_if(paddr_width_p, word_width_gp, lce_id_width_p, lce_assoc_p, cce);

endmodule

