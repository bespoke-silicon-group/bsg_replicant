
`include "bp_common_defines.svh"

module bp_cce_serializer
 import bp_common_pkg::*;
 import bsg_manycore_pkg::*;
 #(parameter bp_params_e bp_params_p = e_bp_default_cfg
   `declare_bp_proc_params(bp_params_p)
   `declare_bp_bedrock_mem_if_widths(paddr_width_p, dword_width_gp, lce_id_width_p, lce_assoc_p, cce)
   `declare_bp_bedrock_mem_if_widths(paddr_width_p, word_width_gp, lce_id_width_p, lce_assoc_p, split)
   )
  (input                                            clk_i
   , input                                          reset_i

   , input [cce_mem_msg_width_lp-1:0]               io_cmd_i
   , input                                          io_cmd_v_i
   , output logic                                   io_cmd_ready_o

   , output logic [cce_mem_msg_width_lp-1:0]        io_resp_o
   , output logic                                   io_resp_v_o
   , input                                          io_resp_yumi_i

   , output logic [split_mem_msg_width_lp-1:0]      io_cmd_o
   , output logic                                   io_cmd_v_o
   , input                                          io_cmd_ready_i

   , input [split_mem_msg_width_lp-1:0]             io_resp_i
   , input                                          io_resp_v_i
   , output logic                                   io_resp_yumi_o
   );

  `declare_bp_bedrock_mem_if(paddr_width_p, dword_width_gp, lce_id_width_p, lce_assoc_p, cce);
  `declare_bp_bedrock_mem_if(paddr_width_p, word_width_gp, lce_id_width_p, lce_assoc_p, split);
  bp_bedrock_cce_mem_msg_s io_cmd_cast_i;
  bp_bedrock_cce_mem_msg_s io_resp_cast_o;
  bp_bedrock_split_mem_msg_s io_cmd_cast_o;
  bp_bedrock_split_mem_msg_s io_resp_cast_i;

  assign io_cmd_cast_i = io_cmd_i;
  assign io_resp_o = io_resp_cast_o;

  assign io_cmd_o = io_cmd_cast_o;
  assign io_resp_cast_i = io_resp_i;

  logic [word_width_gp-1:0] io_cmd_data_li;
  bsg_parallel_in_serial_out_passthrough
   #(.width_p(word_width_gp), .els_p(2))
   pisop
    (.clk_i(clk_i)
     ,.reset_i(reset_i)

     ,.data_i(io_cmd_cast_i.data[0+:64])
     ,.v_i(io_cmd_v_i)
     ,.ready_and_o(io_cmd_ready_o)

     ,.data_o(io_cmd_data_li)
     ,.v_o(io_cmd_v_o)
     ,.ready_and_i(io_cmd_ready_i)
     );

  bp_bedrock_cce_mem_msg_header_s io_cmd_header_li;
  bsg_dff_en_bypass
   #(.width_p($bits(bp_bedrock_cce_mem_msg_header_s)))
   wide_header_reg
    (.clk_i(clk_i)
     ,.en_i(io_cmd_ready_o & io_cmd_v_i)
     ,.data_i(io_cmd_cast_i.header)
     ,.data_o(io_cmd_header_li)
     );

  logic toggle_n, toggle_r;
  bsg_dff_reset
   #(.width_p(1))
   toggle_reg
    (.clk_i(clk_i)
     ,.reset_i(reset_i)
     ,.data_i(toggle_n)
     ,.data_o(toggle_r)
     );
  assign toggle_n = toggle_r ^ (io_cmd_ready_i & io_cmd_v_o);

  // Rebase changes
  always_comb
    begin
      io_cmd_cast_o.header = io_cmd_header_li;
      io_cmd_cast_o.header.size = e_bedrock_msg_size_4;
      io_cmd_cast_o.header.addr = io_cmd_header_li + (toggle_r << 2);
      io_cmd_cast_o.data = io_cmd_data_li;
    end

  logic [dword_width_gp-1:0] io_resp_data_li;
  logic io_resp_ready_lo;
  bsg_serial_in_parallel_out_passthrough
   #(.width_p(word_width_gp), .els_p(2))
   sipop
    (.clk_i(clk_i)
     ,.reset_i(reset_i)

     ,.data_i(io_resp_cast_i.data)
     ,.v_i(io_resp_v_i)
     ,.ready_and_o(io_resp_ready_lo)

     ,.data_o(io_resp_data_li)
     ,.v_o(io_resp_v_o)
     ,.ready_and_i(io_resp_yumi_i)
     );
  assign io_resp_yumi_o = io_resp_ready_lo & io_resp_v_i;

  bp_bedrock_cce_mem_msg_header_s io_resp_header_li;
  bsg_dff_en_bypass
   #(.width_p($bits(bp_bedrock_cce_mem_msg_header_s)))
   narrow_header_reg
    (.clk_i(clk_i)
     ,.en_i(io_resp_v_i & io_resp_ready_lo & ~io_resp_v_o)
     ,.data_i(io_resp_cast_i.header)
     ,.data_o(io_resp_header_li)
     );

  // Combine responses
  always_comb
    begin
      io_resp_cast_o.header = io_resp_header_li;
      io_resp_cast_o.header.size = e_bedrock_msg_size_8;
      io_resp_cast_o.data = io_resp_data_li;
    end

  //synopsys translate_off
  always_ff @(negedge clk_i)
    begin
      assert (~io_cmd_v_i | (io_cmd_cast_i.header.size == e_bedrock_msg_size_8))
        else $error("Only 64-bit split is supported");
    end
  //synopsys translate_on

endmodule

