
module bp_cce_splitter
 import bp_common_aviary_pkg::*;
 import bp_common_pkg::*;
 import bsg_manycore_pkg::*;
 #(parameter bp_params_e bp_params_p = e_bp_default_cfg
   `declare_bp_proc_params(bp_params_p)
   `declare_bp_bedrock_mem_if_widths(paddr_width_p, dword_width_p, lce_id_width_p, lce_assoc_p, cce)
   `declare_bp_bedrock_mem_if_widths(paddr_width_p, word_width_p, lce_id_width_p, lce_assoc_p, split)
   )
  (input                                            clk_i
   , input                                          reset_i

   , input [cce_mem_msg_width_lp-1:0]               io_cmd_i
   , input                                          io_cmd_v_i
   , output logic                                   io_cmd_ready_o

   , output logic [cce_mem_msg_width_lp-1:0]        io_resp_o
   , output logic                                   io_resp_v_o
   , input                                          io_resp_yumi_i

   , output logic [1:0][split_mem_msg_width_lp-1:0] io_cmd_o
   , output logic [1:0]                             io_cmd_v_o
   , input [1:0]                                    io_cmd_ready_i

   , input [1:0][split_mem_msg_width_lp-1:0]        io_resp_i
   , input [1:0]                                    io_resp_v_i
   , output logic [1:0]                             io_resp_yumi_o
   );

  `declare_bp_bedrock_mem_if(paddr_width_p, dword_width_p, lce_id_width_p, lce_assoc_p, cce);
  `declare_bp_bedrock_mem_if(paddr_width_p, word_width_p, lce_id_width_p, lce_assoc_p, split);
  bp_bedrock_cce_mem_msg_s io_cmd_cast_i;
  bp_bedrock_cce_mem_msg_s io_resp_cast_o;
  bp_bedrock_split_mem_msg_s [1:0] io_cmd_cast_o;
  bp_bedrock_split_mem_msg_s [1:0] io_resp_cast_i;

  assign io_cmd_cast_i = io_cmd_i;
  assign io_resp_o = io_resp_cast_o;

  assign io_cmd_o = io_cmd_cast_o;
  assign io_resp_cast_i = io_resp_i;

  // We only support 64-bit DRAM access i.e. no uncached access to DRAM
  assign io_cmd_ready_o = &io_cmd_ready_i;
  assign io_cmd_v_o[0] = io_cmd_v_i;
  assign io_cmd_v_o[1] = io_cmd_v_i;
  always_comb
    begin
      io_cmd_cast_o[0] = io_cmd_cast_i;
      io_cmd_cast_o[0].header.size = e_bedrock_msg_size_4;
      io_cmd_cast_o[0].header.addr = io_cmd_cast_i.header.addr + 4'd0;
      io_cmd_cast_o[0].data = io_cmd_cast_i.data[0+:32];

      io_cmd_cast_o[1] = io_cmd_cast_i;
      io_cmd_cast_o[1].header.size = e_bedrock_msg_size_4;
      io_cmd_cast_o[1].header.addr = io_cmd_cast_i.header.addr + 4'd4;
      io_cmd_cast_o[1].data = io_cmd_cast_i.data[32+:32];
    end

  assign io_resp_v_o = &io_resp_v_i;
  assign io_resp_yumi_o[0] = io_resp_yumi_i;
  assign io_resp_yumi_o[1] = io_resp_yumi_i;
  always_comb
    begin
      io_resp_cast_o = io_resp_cast_i[0];
      io_resp_cast_o.header.size = e_bedrock_msg_size_8;
      io_resp_cast_o.data = {io_resp_cast_i[1].data[0+:32], io_resp_cast_i[0].data[0+:32]};
    end

  //synopsys translate_off
  always_ff @(negedge clk_i)
    begin
      assert (~io_cmd_v_i | (io_cmd_cast_i.header.size == e_bedrock_msg_size_8))
        else $error("Only 64-bit split is supported");
    end
  //synopsys translate_on

endmodule

