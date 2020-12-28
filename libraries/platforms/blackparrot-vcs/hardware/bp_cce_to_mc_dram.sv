
module bp_cce_to_mc_dram
 import bp_common_aviary_pkg::*;
 import bp_common_pkg::*;
 import bsg_manycore_pkg::*;
 #(parameter bp_params_e bp_params_p = e_bp_default_cfg
   `declare_bp_proc_params(bp_params_p)
   `declare_bp_bedrock_mem_if_widths(paddr_width_p, cce_block_width_p, lce_id_width_p, lce_assoc_p, cce)

   , parameter mc_max_outstanding_p     = 32
   , parameter mc_x_cord_width_p        = "inv"
   , parameter mc_y_cord_width_p        = "inv"
   , parameter mc_data_width_p          = "inv"
   , parameter mc_addr_width_p          = "inv"
   , localparam mc_packet_width_lp      = `bsg_manycore_packet_width(mc_addr_width_p, mc_data_width_p, mc_x_cord_width_p, mc_y_cord_width_p)
   , localparam mc_link_sif_width_lp    = `bsg_manycore_link_sif_width(mc_addr_width_p, mc_data_width_p, mc_x_cord_width_p, mc_y_cord_width_p)
   )
  (input                                      clk_i
   , input                                    reset_i

   , input [cce_mem_msg_width_lp-1:0]         mem_cmd_i
   , input                                    mem_cmd_v_i
   , output logic                             mem_cmd_ready_o

   , output logic [cce_mem_msg_width_lp-1:0]  mem_resp_o
   , output logic                             mem_resp_v_o
   , input                                    mem_resp_yumi_i

   , input [mc_link_sif_width_lp-1:0]         link_sif_i
   , output logic [mc_link_sif_width_lp-1:0]  link_sif_o

   , input [mc_x_cord_width_p-1:0]            my_x_i
   , input [mc_y_cord_width_p-1:0]            my_y_i
   );

  `declare_bp_bedrock_mem_if(paddr_width_p, cce_block_width_p, lce_id_width_p, lce_assoc_p, cce);
  `declare_bsg_manycore_packet_s(mc_addr_width_p, mc_data_width_p, mc_x_cord_width_p, mc_y_cord_width_p);
  `bp_cast_i(bp_bedrock_cce_mem_msg_s, mem_cmd);
  `bp_cast_o(bp_bedrock_cce_mem_msg_s, mem_resp);

  //
  // TX
  //
  logic [`BSG_SAFE_CLOG2(mc_max_outstanding_p)-1:0] trans_id_lo;
  logic trans_id_v_lo, trans_id_yumi_li;
  logic [`BSG_SAFE_CLOG2(mc_max_outstanding_p)-1:0] mem_returned_id_li;
  logic [mc_data_width_p-1:0] mem_returned_data_li;
  logic mem_returned_v_li;
  logic [mc_data_width_p-1:0] mem_resp_data_lo;
  logic mem_resp_v_lo, mem_resp_yumi_li;
  bsg_fifo_reorder
   #(.width_p(mc_data_width_p), .els_p(mc_max_outstanding_p))
   return_data_fifo
    (.clk_i(clk_i)
     ,.reset_i(reset_i)

     ,.fifo_alloc_id_o(trans_id_lo)
     ,.fifo_alloc_v_o(trans_id_v_lo)
     ,.fifo_alloc_yumi_i(trans_id_yumi_li)

     // We write an entry on credit return in order to determine when to send
     //   back a store response.  A little inefficent, but allocating storage for
     //   worst case (all loads) isn't unreasonable
     ,.write_id_i(mem_returned_id_li)
     ,.write_data_i(mem_returned_data_li)
     ,.write_v_i(mem_returned_v_li)

     ,.fifo_deq_data_o(mem_resp_data_lo)
     ,.fifo_deq_v_o(mem_resp_v_lo)
     ,.fifo_deq_yumi_i(mem_resp_yumi_li)

     ,.empty_o()
     );

  bp_bedrock_cce_mem_msg_header_s mem_resp_header_lo;
  logic mem_resp_header_v_lo, mem_resp_header_yumi_li;
  logic header_ready_lo;
  bsg_fifo_1r1w_small
   #(.width_p($bits(mem_cmd_cast_i.header)), .els_p(mc_max_outstanding_p))
   return_header_fifo
    (.clk_i(clk_i)
     ,.reset_i(reset_i)

     ,.data_i(mem_cmd_cast_i.header)
     ,.v_i(mem_cmd_v_i)
     ,.ready_o(header_ready_lo)

     ,.data_o(mem_resp_header_lo)
     ,.v_o(mem_resp_header_v_lo)
     ,.yumi_i(mem_resp_header_yumi_li)
     );
  assign mem_resp_header_yumi_li = mem_resp_yumi_i;

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
    ,.fifo_els_p(16)
    ,.data_width_p(mc_data_width_p)
    ,.addr_width_p(mc_addr_width_p)

    ,.max_out_credits_p(16)
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
    ,.in_v_o()
    ,.in_data_o()
    ,.in_mask_o()
    ,.in_addr_o()
    ,.in_we_o()
    ,.in_load_info_o()
    ,.in_src_x_cord_o()
    ,.in_src_y_cord_o()
    ,.in_yumi_i(1'b0)

    //--------------------------------------------------------
    // 2. out_response signal group
    //    responses that will send back to the network
    ,.returning_data_i()
    ,.returning_v_i(1'b0)

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
    // We allocate data in the return fifo, so we can immediately accept, always
    ,.returned_yumi_i(returned_yumi_li)
    ,.returned_fifo_full_o()

    ,.returned_credit_v_r_o(returned_credit_v_r_lo)
    ,.returned_credit_reg_id_r_o(returned_credit_reg_id_r_lo)

    ,.out_credits_o(out_credits_lo)

    ,.my_x_i(my_x_i)
    ,.my_y_i(my_y_i)
    );

  typedef struct packed
  {
    logic dram_not_tile;
    logic tile_not_dram;
    union packed
    {
      struct packed
      {
        logic [29:0] dram_addr;
      } dram_eva;
      struct packed
      {
        logic [max_y_cord_width_gp-1:0]    y_cord;
        logic [max_x_cord_width_gp-1:0]    x_cord;
        logic [epa_word_addr_width_gp-1:0] epa;
        logic [1:0]                        low_bits;
      } tile_eva;
    } a;
  } bp_eva_s;

  bp_eva_s mem_cmd_eva_li;
  assign mem_cmd_eva_li = mem_cmd_cast_i.header.addr;

  // Command packet formatmemn
  always_comb
    begin
      mem_cmd_ready_o = trans_id_v_lo & header_ready_lo & out_ready_lo;
      trans_id_yumi_li = mem_cmd_v_i;
      out_v_li = trans_id_yumi_li;
      out_packet_li = '0;

      case (mem_cmd_cast_i.header.msg_type)
        e_bedrock_mem_uc_rd:
          begin
            // Word-aligned address
            out_packet_li.addr                             = mem_cmd_eva_li.a.tile_eva.epa;
            out_packet_li.op                               = e_remote_load;
            // Ignored for remote loads
            out_packet_li.op_ex                            = '0;
            // Overload reg_id with the trans id of the request
            out_packet_li.reg_id                           = bsg_manycore_reg_id_width_gp'(trans_id_lo);
            // Irrelevant for bp loads
            out_packet_li.payload.load_info_s.load_info.float_wb       = '0;
            out_packet_li.payload.load_info_s.load_info.icache_fetch   = '0;
            out_packet_li.payload.load_info_s.load_info.is_unsigned_op = '0;
            // 64-bit+ packets are not supported
            out_packet_li.payload.load_info_s.load_info.is_byte_op     = (mem_cmd_cast_i.header.size == e_bedrock_msg_size_1);
            out_packet_li.payload.load_info_s.load_info.is_hex_op      = (mem_cmd_cast_i.header.size == e_bedrock_msg_size_2);
            // Assume aligned for now
            out_packet_li.payload.load_info_s.load_info.part_sel       = mem_cmd_eva_li.a.tile_eva.low_bits;
            out_packet_li.src_y_cord                       = my_y_i;
            out_packet_li.src_x_cord                       = my_x_i;
            out_packet_li.y_cord                           = mem_cmd_eva_li.a.tile_eva.y_cord;
            out_packet_li.x_cord                           = mem_cmd_eva_li.a.tile_eva.x_cord;
          end
        e_bedrock_mem_uc_wr:
          begin
            // Word-aligned address
            out_packet_li.addr                             = mem_cmd_eva_li.a.tile_eva.epa;
            out_packet_li.op                               = e_remote_store;
            // Set store data and mask (assume aligned)
            case (mem_cmd_cast_i.header.size)
              e_bedrock_msg_size_1:
                begin
                  out_packet_li.payload.data               = {4{mem_cmd_cast_i.data[0+:8]}};
                  out_packet_li.op_ex.store_mask           = 4'h1 << mem_cmd_eva_li.a.tile_eva.low_bits;
                end
              e_bedrock_msg_size_2:
                begin
                  out_packet_li.payload.data               = {2{mem_cmd_cast_i.data[0+:16]}};
                  out_packet_li.op_ex.store_mask           = 4'h3 << mem_cmd_eva_li.a.tile_eva.low_bits;
                end
              e_bedrock_msg_size_4:
                begin
                  out_packet_li.payload.data               = {1{mem_cmd_cast_i.data[0+:32]}};
                  out_packet_li.op_ex.store_mask           = 4'hf << mem_cmd_eva_li.a.tile_eva.low_bits;
                end
              default:
                begin
                  // Should not happen
                  out_packet_li.payload.data               = '0;
                  out_packet_li.op_ex.store_mask           = '0;
                end
            endcase
            out_packet_li.reg_id                           = bsg_manycore_reg_id_width_gp'(trans_id_lo);
            out_packet_li.src_y_cord                       = my_y_i;
            out_packet_li.src_x_cord                       = my_x_i;
            out_packet_li.y_cord                           = mem_cmd_eva_li.a.tile_eva.y_cord;
            out_packet_li.x_cord                           = mem_cmd_eva_li.a.tile_eva.x_cord;
          end
        // Unsupported
        e_bedrock_mem_pre
        ,e_bedrock_mem_rd
        ,e_bedrock_mem_wr: out_packet_li = '0;
        default      : out_packet_li = '0;
      endcase
    end

  // Response packet formatmemn
  always_comb
    begin
      mem_returned_id_li   = returned_v_r_lo ? returned_reg_id_r_lo : returned_credit_reg_id_r_lo;
      mem_returned_data_li = returned_data_r_lo;
      mem_returned_v_li    = returned_v_r_lo | returned_credit_v_r_lo;
      returned_yumi_li      = returned_v_r_lo & mem_returned_v_li;

      mem_resp_v_o           = mem_resp_header_v_lo & mem_resp_v_lo;
      mem_resp_cast_o.header = mem_resp_header_lo;
      mem_resp_cast_o.data   = mem_resp_data_lo;

      mem_resp_yumi_li = mem_resp_yumi_i;
    end

  always_ff @(negedge clk_i)
    begin
      if (mem_cmd_v_i)
        $display("[BP-LINK] Outgoing command: %p", mem_cmd_cast_i);
      if (mem_resp_yumi_i)
        $display("[BP-LINK] Incoming response: %p", mem_resp_cast_o);
    end

endmodule

