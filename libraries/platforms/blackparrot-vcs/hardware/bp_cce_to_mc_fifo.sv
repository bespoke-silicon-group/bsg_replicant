
module bp_cce_to_mc_fifo
 import bp_common_aviary_pkg::*;
 import bp_common_pkg::*;
 import bsg_manycore_pkg::*;
 #(parameter bp_params_e bp_params_p = e_bp_default_cfg
   `declare_bp_proc_params(bp_params_p)
   `declare_bp_bedrock_mem_if_widths(paddr_width_p, cce_block_width_p, lce_id_width_p, lce_assoc_p, cce)

   , parameter mc_x_cord_width_p        = "inv"
   , parameter mc_y_cord_width_p        = "inv"
   , parameter mc_data_width_p          = "inv"
   , parameter mc_addr_width_p          = "inv"
   , localparam mc_packet_width_lp      = `bsg_manycore_packet_width(mc_addr_width_p, mc_data_width_p, mc_x_cord_width_p, mc_y_cord_width_p)
   , localparam mc_link_sif_width_lp    = `bsg_manycore_link_sif_width(mc_addr_width_p, mc_data_width_p, mc_x_cord_width_p, mc_y_cord_width_p)
   , localparam mc_max_outstanding_lp   = 32
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
  `bp_cast_o(bp_bedrock_cce_mem_msg_s, io_cmd);
  `bp_cast_i(bp_bedrock_cce_mem_msg_s, io_resp);
  `bp_cast_i(bp_bedrock_cce_mem_msg_s, io_cmd);
  `bp_cast_o(bp_bedrock_cce_mem_msg_s, io_resp);

  bp_bedrock_cce_mem_msg_s io_cmd_li;
  logic io_cmd_v_li, io_cmd_yumi_lo;
  bsg_two_fifo
   #(.width_p(cce_mem_msg_width_lp))
   small_fifo
    (.clk_i(clk_i)
     ,.reset_i(reset_i)

     ,.data_i(io_cmd_cast_i)
     ,.v_i(io_cmd_v_i)
     ,.ready_o(io_cmd_ready_o)

     ,.data_o(io_cmd_li)
     ,.v_o(io_cmd_v_li)
     ,.yumi_i(io_cmd_yumi_lo)
     );

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
  logic                              returned_v_r_lo;
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
    ,.returned_yumi_i(returned_v_r_lo)
    ,.returned_fifo_full_o()

    // TODO: When ci_bigblade is merged, this will become relevant
    //,.returned_credit_v_r_o(returned_credit_v_r_lo)
    //,.returned_credit_reg_id_r_o(returned_credit_reg_id_r_lo)

    ,.out_credits_o(out_credits_lo)

    ,.my_x_i(my_x_i)
    ,.my_y_i(my_y_i)
    );

  //
  // MC loads from BP are disabled, so we stub this
  //
  assign returning_data_li = '0;
  assign returning_v_li = '0;

  // TODO: This should be set in bsg_replicant
  typedef struct packed
  {
    logic [7:0]  reserved;
    logic [31:0] addr;
    logic [7:0]  op;
    logic [7:0]  op_ex;
    logic [7:0]  reg_id;
    logic [31:0] data;
    logic [7:0]  y_src;
    logic [7:0]  x_src;
    logic [7:0]  y_dst;
    logic [7:0]  x_dst;
  }  host_request_packet_s;

  typedef struct packed
  {
    logic [63:0] reserved;
    logic [7:0]  op;
    logic [31:0] data;
    logic [7:0]  load_id;
    logic [7:0]  y_dst;
    logic [7:0]  x_dst;
  }  host_response_packet_s;

  logic [dword_width_p-1:0] bp_to_mc_data_li;
  logic bp_to_mc_v_li, bp_to_mc_ready_lo;
  host_request_packet_s bp_to_mc_lo;
  logic bp_to_mc_v_lo, bp_to_mc_yumi_li;
  bsg_manycore_load_info_s bp_to_mc_load_info;
  bsg_serial_in_parallel_out_full
   #(.width_p(dword_width_p), .els_p($bits(host_request_packet_s)/dword_width_p))
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
  assign out_packet_li = '{addr       : bp_to_mc_lo.addr[2+:mc_addr_width_p]
                           ,op        : bp_to_mc_lo.op
                           ,op_ex     : bp_to_mc_lo.op_ex
                           ,reg_id    : bp_to_mc_lo.reg_id
                           ,payload   : (bp_to_mc_lo.op == e_remote_store)
                                        ? bp_to_mc_lo.data
                                        : bp_to_mc_load_info
                           ,src_y_cord: bp_to_mc_lo.y_src
                           ,src_x_cord: bp_to_mc_lo.x_src
                           ,y_cord    : bp_to_mc_lo.y_dst
                           ,x_cord    : bp_to_mc_lo.x_dst
                           };
  assign out_v_li = out_ready_lo & bp_to_mc_v_lo;
  assign bp_to_mc_yumi_li = out_v_li;

  host_response_packet_s mc_to_bp_response_li;
  logic mc_to_bp_response_v_li, mc_to_bp_response_ready_lo;
  logic [dword_width_p-1:0] mc_to_bp_response_data_lo;
  logic mc_to_bp_response_v_lo, mc_to_bp_response_yumi_li;
  bsg_parallel_in_serial_out
   #(.width_p(dword_width_p), .els_p($bits(host_response_packet_s)/dword_width_p))
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
  assign mc_to_bp_response_li = '{x_dst   : my_x_i
                                  ,y_dst  : my_y_i
                                  ,load_id: returned_reg_id_r_lo
                                  ,data   : returned_data_r_lo
                                  // Possibly unused by host?
                                  ,op     : returned_pkt_type_r_lo
                                  ,default: '0
                                  };
  assign mc_to_bp_response_v_li = mc_to_bp_response_ready_lo & returned_v_r_lo;
  assign returned_yumi_li = mc_to_bp_response_v_li;

  host_request_packet_s mc_to_bp_request_li;
  logic mc_to_bp_request_v_li, mc_to_bp_request_ready_lo;
  logic [dword_width_p-1:0] mc_to_bp_request_data_lo;
  logic mc_to_bp_request_v_lo, mc_to_bp_request_yumi_li;
  bsg_parallel_in_serial_out
   #(.width_p(dword_width_p), .els_p($bits(host_request_packet_s)/dword_width_p))
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
                                 ,data    : in_data_lo
                                 // We only support remote stores from MC
                                 ,reg_id  : '0
                                 ,op_ex   : in_mask_lo
                                 ,op      : e_remote_store
                                 // Else these fields would be dynamic
                                 ,addr    : in_addr_lo
                                 ,default : '0
                                 };
  assign mc_to_bp_request_v_li = mc_to_bp_request_ready_lo & in_v_lo;
  assign in_yumi_li = mc_to_bp_request_v_li;

  //typedef struct packed
  //{
  //  logic [8:0]  nonlocal;
  //  logic [6:0]  cce;
  //  logic [3:0]  dev;
  //  logic [19:0] addr;
  //}  bp_local_addr_s;

  bp_local_addr_s local_addr;
  assign local_addr = io_cmd_li.header.addr;

  logic bp_req_fifo_cmd_v;
  logic bp_req_credits_cmd_v;
  logic bp_resp_fifo_cmd_v;
  logic bp_resp_entries_cmd_v;
  logic mc_req_fifo_cmd_v;
  logic mc_req_entries_cmd_v;
  
  logic wr_not_rd;

  localparam mc_link_bp_req_fifo_addr_gp     = 20'h0_1000;
  localparam mc_link_bp_req_credits_addr_gp  = 20'h0_2000;
  localparam mc_link_bp_resp_fifo_addr_gp    = 20'h0_3000;
  localparam mc_link_bp_resp_entries_addr_gp = 20'h0_4000;
  localparam mc_link_mc_req_fifo_addr_gp     = 20'h0_5000;
  localparam mc_link_mc_req_entries_addr_gp  = 20'h0_6000;

  always_comb
    begin
      bp_req_fifo_cmd_v = 1'b0;
      bp_req_credits_cmd_v = 1'b0;
      bp_resp_fifo_cmd_v = 1'b0;
      bp_resp_entries_cmd_v = 1'b0;
      mc_req_fifo_cmd_v = 1'b0;
      mc_req_entries_cmd_v = 1'b0;

      // TODO: We don't actually check that
      wr_not_rd = io_cmd_li.header.msg_type inside {e_bedrock_mem_wr, e_bedrock_mem_uc_wr};
  
      case ({local_addr.dev, local_addr.addr})
        mc_link_bp_req_fifo_addr_gp     : bp_req_fifo_cmd_v     = io_cmd_v_li;
        mc_link_bp_req_credits_addr_gp  : bp_req_credits_cmd_v  = io_cmd_v_li;
        mc_link_bp_resp_fifo_addr_gp    : bp_resp_fifo_cmd_v    = io_cmd_v_li;
        mc_link_bp_resp_entries_addr_gp : bp_resp_entries_cmd_v = io_cmd_v_li;
        mc_link_mc_req_fifo_addr_gp     : mc_req_fifo_cmd_v     = io_cmd_v_li;
        mc_link_mc_req_entries_addr_gp  : mc_req_entries_cmd_v  = io_cmd_v_li;
        default: begin end
      endcase
    end

  // I/O Interfacing
  //
  always_comb
    begin
      bp_to_mc_data_li = '0;
      bp_to_mc_v_li    = '0;

      mc_to_bp_response_yumi_li = '0;

      io_resp_cast_o = '0;
      io_resp_v_o    = '0;
      io_cmd_yumi_lo = '0;

      // Need to connect for MC->BP connection
      io_cmd_v_o = 1'b0;
      io_resp_ready_o = 1'b1;

      if (bp_req_fifo_cmd_v)
        begin
          io_resp_cast_o   = '{header: io_cmd_li.header, data: '0};
          io_resp_v_o      = bp_to_mc_ready_lo;
          io_cmd_yumi_lo   = io_resp_yumi_i;

          bp_to_mc_data_li = io_cmd_li.data[0+:64];
          bp_to_mc_v_li    = io_cmd_yumi_lo;
        end
      else if (bp_req_credits_cmd_v)
        begin
          io_resp_cast_o = '{header: io_cmd_li.header, data: out_credits_lo};
          io_resp_v_o    = 1'b1;
          io_cmd_yumi_lo = io_resp_yumi_i;
        end
      else if (bp_resp_fifo_cmd_v)
        begin
          io_resp_cast_o = '{header: io_cmd_li.header, data: mc_to_bp_response_data_lo};
          io_resp_v_o    = mc_to_bp_response_v_lo;
          io_cmd_yumi_lo = io_resp_yumi_i;

          mc_to_bp_response_yumi_li = io_cmd_yumi_lo;
        end
      else if (bp_resp_entries_cmd_v)
        begin
          io_resp_cast_o = '{header: io_cmd_li.header, data: mc_to_bp_response_v_lo};
          io_resp_v_o    = 1'b1;
          io_cmd_yumi_lo = io_resp_yumi_i;
        end
    end

  always_ff @(negedge clk_i)
    begin
      if (io_cmd_v_li)
        $display("[BP] Incoming command: %p", io_cmd_li);
      if (out_v_li)
        $display("[LINK] Outgoing mc_pkt: %p", out_packet_li);
      if (io_resp_yumi_i)
        $display("[BP] Outgoing response: %p", io_resp_cast_o);
    end

endmodule

