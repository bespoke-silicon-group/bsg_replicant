
module bp_cce_to_mc_mmio
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

  `declare_bp_bedrock_mem_if(paddr_width_p, cce_block_width_p, lce_id_width_p, lce_assoc_p, cce);
  `declare_bsg_manycore_packet_s(mc_addr_width_p, mc_data_width_p, mc_x_cord_width_p, mc_y_cord_width_p);
  `bp_cast_i(bp_bedrock_cce_mem_msg_s, io_cmd);
  `bp_cast_o(bp_bedrock_cce_mem_msg_s, io_resp);
  `bp_cast_o(bp_bedrock_cce_mem_msg_s, io_cmd);
  `bp_cast_i(bp_bedrock_cce_mem_msg_s, io_resp);

  //
  // TX
  //
  logic [`BSG_SAFE_CLOG2(mc_max_outstanding_p)-1:0] trans_id_lo;
  logic trans_id_v_lo, trans_id_yumi_li;
  logic [`BSG_SAFE_CLOG2(mc_max_outstanding_p)-1:0] mmio_returned_id_li;
  logic [mc_data_width_p-1:0] mmio_returned_data_li;
  logic mmio_returned_v_li;
  logic [mc_data_width_p-1:0] mmio_resp_data_lo;
  logic mmio_resp_v_lo, mmio_resp_yumi_li;
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
     ,.write_id_i(mmio_returned_id_li)
     ,.write_data_i(mmio_returned_data_li)
     ,.write_v_i(mmio_returned_v_li)

     ,.fifo_deq_data_o(mmio_resp_data_lo)
     ,.fifo_deq_v_o(mmio_resp_v_lo)
     ,.fifo_deq_yumi_i(mmio_resp_yumi_li)

     ,.empty_o()
     );

  bp_bedrock_cce_mem_msg_header_s io_resp_header_lo;
  logic io_resp_header_v_lo, io_resp_header_yumi_li;
  logic header_ready_lo;
  bsg_fifo_1r1w_small
   #(.width_p($bits(io_cmd_cast_i.header)), .els_p(mc_max_outstanding_p))
   return_header_fifo
    (.clk_i(clk_i)
     ,.reset_i(reset_i)

     ,.data_i(io_cmd_cast_i.header)
     ,.v_i(io_cmd_v_i)
     ,.ready_o(header_ready_lo)

     ,.data_o(io_resp_header_lo)
     ,.v_o(io_resp_header_v_lo)
     ,.yumi_i(io_resp_header_yumi_li)
     );
  assign io_resp_header_yumi_li = io_resp_yumi_i;

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
    // 1x indicates DRAM address
    // 01x indicates global manycore address
    // 00x indicates BlackParrot address
    logic [1:0] _type;
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

  bp_eva_s io_cmd_eva_li;
  assign io_cmd_eva_li = io_cmd_cast_i.header.addr;

  // Command packet formation
  always_comb
    begin
      io_cmd_ready_o = trans_id_v_lo & header_ready_lo & out_ready_lo;
      trans_id_yumi_li = io_cmd_v_i;
      out_v_li = trans_id_yumi_li;
      out_packet_li = '0;

      case (io_cmd_cast_i.header.msg_type)
        e_bedrock_mem_uc_rd:
          begin
            // Word-aligned address
            out_packet_li.addr                             = io_cmd_eva_li.a.tile_eva.epa;
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
            out_packet_li.payload.load_info_s.load_info.is_byte_op     = (io_cmd_cast_i.header.size == e_bedrock_msg_size_1);
            out_packet_li.payload.load_info_s.load_info.is_hex_op      = (io_cmd_cast_i.header.size == e_bedrock_msg_size_2);
            // Assume aligned for now

            out_packet_li.payload.load_info_s.load_info.part_sel       = io_cmd_eva_li.a.tile_eva.low_bits;
            out_packet_li.src_y_cord                       = my_y_i;
            out_packet_li.src_x_cord                       = my_x_i;
            out_packet_li.y_cord                           = io_cmd_eva_li.a.tile_eva.y_cord;
            out_packet_li.x_cord                           = io_cmd_eva_li.a.tile_eva.x_cord;
          end
        e_bedrock_mem_uc_wr:
          begin
            // Word-aligned address
            out_packet_li.addr                             = io_cmd_eva_li.a.tile_eva.epa;
            out_packet_li.op                               = e_remote_store;
            // Set store data and mask (assume aligned)
            case (io_cmd_cast_i.header.size)
              e_bedrock_msg_size_1:
                begin
                  out_packet_li.payload.data               = {4{io_cmd_cast_i.data[0+:8]}};
                  out_packet_li.op_ex.store_mask           = 4'h1 << io_cmd_eva_li.a.tile_eva.low_bits;
                end
              e_bedrock_msg_size_2:
                begin
                  out_packet_li.payload.data               = {2{io_cmd_cast_i.data[0+:16]}};
                  out_packet_li.op_ex.store_mask           = 4'h3 << io_cmd_eva_li.a.tile_eva.low_bits;
                end
              e_bedrock_msg_size_4:
                begin
                  out_packet_li.payload.data               = {1{io_cmd_cast_i.data[0+:32]}};
                  out_packet_li.op_ex.store_mask           = 4'hf << io_cmd_eva_li.a.tile_eva.low_bits;
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
            out_packet_li.y_cord                           = io_cmd_eva_li.a.tile_eva.y_cord;
            out_packet_li.x_cord                           = io_cmd_eva_li.a.tile_eva.x_cord;
          end
        // Unsupported
        e_bedrock_mem_pre
        ,e_bedrock_mem_rd
        ,e_bedrock_mem_wr: out_packet_li = '0;
        default      : out_packet_li = '0;
      endcase
    end

  // Response packet formation
  always_comb
    begin
      mmio_returned_id_li   = returned_v_r_lo ? returned_reg_id_r_lo : returned_credit_reg_id_r_lo;
      mmio_returned_data_li = returned_data_r_lo;
      mmio_returned_v_li    = returned_v_r_lo | returned_credit_v_r_lo;
      returned_yumi_li      = returned_v_r_lo;

      io_resp_v_o           = io_resp_header_v_lo & mmio_resp_v_lo;
      io_resp_cast_o.header = io_resp_header_lo;
      io_resp_cast_o.data   = mmio_resp_data_lo;

      mmio_resp_yumi_li = io_resp_yumi_i;
    end


  // BP EPA Map
  // dev: 0 -- CFG
  //      1 -- CCE ucode
  //      2 -- CLINT
  typedef struct packed
  {
    logic [3:0]  dev;
    logic [11:0] addr;
  } bp_epa_s;
  bp_epa_s in_epa_li;
  assign in_epa_li = in_addr_lo;

  bp_bedrock_cce_mem_payload_s io_payload_lo;
  assign io_payload_lo = '{lce_id: 2'b10, default: '0};
  // Incoming packet
  always_comb
    begin
      io_cmd_cast_o.header.msg_type = in_we_lo ? e_bedrock_mem_uc_wr : e_bedrock_mem_uc_rd;

      if (in_epa_li.dev == 2)
        io_cmd_cast_o.header.addr = clint_dev_base_addr_gp + in_epa_li.addr;
      else if (in_epa_li.dev == 1)
        io_cmd_cast_o.header.addr = cfg_dev_base_addr_gp + bp_cfg_mem_base_cce_ucode_gp + in_epa_li.addr;
      else // if (in_epa_li.dev == cfg_dev_gp)
        io_cmd_cast_o.header.addr = cfg_dev_base_addr_gp + in_epa_li.addr;

      // TODO: we only support 64-bit loads and stores to BP configuration addresses
      io_cmd_cast_o.header.size = e_bedrock_msg_size_8;
      io_cmd_cast_o.header.payload = io_payload_lo;
      io_cmd_cast_o.data = in_data_lo;

      io_cmd_v_o = in_v_lo;
      in_yumi_li = io_cmd_yumi_i;
    end

  // Returning data
  always_comb
    begin
      returning_data_li = (io_resp_cast_i.header.size == e_bedrock_msg_size_4)
                          ? io_resp_cast_i.data[0+:32]
                          : (io_resp_cast_i.header.size == e_bedrock_msg_size_2)
                            ? io_resp_cast_i.data[0+:16]
                            : io_resp_cast_i.data[0+:8];
      returning_v_li = io_resp_v_i;

      // Returning data is always ready
      io_resp_ready_o = 1'b1;
    end

  always_ff @(negedge clk_i)
    begin
      if (io_cmd_v_i)
        begin
          $display("[BP-LINK] Outgoing command: %p", io_cmd_cast_i);
          $display("[      EVA] Outgoing EVA: %p", io_cmd_eva_li);
          $display("[   PACKET] Outgoing packet: %p", out_packet_li);
        end
      if (io_resp_yumi_i)
        $display("[BP-LINK] Incoming response: %p", io_resp_cast_o);
    end

endmodule

