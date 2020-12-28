
module bp_lite_gearbox
 import bp_common_aviary_pkg::*;
 import bp_common_pkg::*;
 #(parameter bp_params_e bp_params_p = e_bp_default_cfg
   `declare_bp_proc_params(bp_params_p)

   , parameter in_data_width_p  = "inv"
   , parameter out_data_width_p = "inv"

   // Bitmask which etermines which message types have a data payload
   // Constructed as (1 << e_payload_msg1 | 1 << e_payload_msg2)
   , parameter payload_mask_p = 0

   `declare_bp_bedrock_mem_if_widths(paddr_width_p, in_data_width_p, lce_id_width_p, lce_assoc_p, in)
   `declare_bp_bedrock_mem_if_widths(paddr_width_p, out_data_width_p, lce_id_width_p, lce_assoc_p, out)
   )
  (input clk_i
   , input reset_i

   , input [in_mem_msg_width_lp-1:0]          mem_i
   , input                                    mem_v_i
   , output logic                             mem_ready_and_o

   , output logic [out_mem_msg_width_lp-1:0]  mem_o
   , output logic                             mem_v_o
   , input                                    mem_ready_and_i
   );

  `declare_bp_bedrock_mem_if(paddr_width_p, cce_block_width_p, lce_id_width_p, lce_assoc_p, in);
  `declare_bp_bedrock_mem_if(paddr_width_p, cce_block_width_p, lce_id_width_p, lce_assoc_p, out);
  `bp_cast_i(bp_bedrock_in_mem_msg_s, mem);
  `bp_cast_o(bp_bedrock_out_mem_msg_s, mem);

  localparam downshift_lp  = (in_data_width_p > out_data_width_p);
  localparam max_size_lp   = `BSG_MIN($clog2(in_data_width_p/8), $clog2(out_data_width_p/8));
  localparam bus_ratio_lp  = `BSG_MAX(in_data_width_p/out_data_width_p, out_data_width_p/in_data_width_p);
  localparam size_delta_lp = $clog2(bus_ratio_lp);

  if (downshift_lp)
    begin : downshift
      logic [in_data_width_p-1:0] pisop_data_li;
      logic pisop_v_li, pisop_ready_and_lo;
      logic [out_data_width_p-1:0] pisop_data_lo;
      logic pisop_v_lo, pisop_ready_and_li;
      bsg_parallel_in_serial_out_passthrough
       #(.width_p(out_data_width_p), .els_p(bus_ratio_lp))
       pisop
        (.clk_i(clk_i)
         ,.reset_i(reset_i)

         ,.data_i(pisop_data_li)
         ,.v_i(pisop_v_li)
         ,.ready_and_o(pisop_ready_and_lo)

         ,.data_o(pisop_data_lo)
         ,.v_o(pisop_v_lo)
         ,.ready_and_i(pisop_ready_and_li)
         );

      assign multibeat_li = (mem_cast_i.header.size > max_size_lp) & payload_mask_p[mem_cast_i.msg_type];
      always_comb
        begin
          pisop_data_li = mem_cast_i.data;
          pisop_v_li = mem_v_i & multibeat_li;
          mem_ready_and_o = pisop_ready_and_lo & mem_ready_and_i;

          // Override size 
          mem_cast_o.header = multibeat_li ? (mem_cast_i.header.size - size_delta_lp) : mem_cast_i.header;
          // TODO: could probably just use pisop_data_lo here
          mem_cast_o.data = multibeat_li ? pisop_data_lo : mem_cast_i.data;
          mem_v_o = multibeat_li ? pisop_v_lo : mem_v_i;
          pisop_ready_and_li = mem_ready_and_i;
        end
    end
  else
    begin : upshift
      logic [in_data_width_p-1:0] sipop_data_li;
      logic sipop_v_li, sipop_ready_and_lo;
      logic [out_data_width_p-1:0] sipop_data_lo;
      logic sipop_v_lo, sipop_ready_and_li;
      bsg_serial_in_parallel_out_passthrough
       #(.width_p(in_data_width_p), .els_p(bus_ratio_lp))
       sipop
        (.clk_i(clk_i)
         ,.reset_i(reset_i)

         ,.data_i(sipop_data_li)
         ,.v_i(sipop_v_li)
         ,.ready_and_o(sipop_ready_and_lo)

         ,.data_o(sipop_data_lo)
         ,.v_o(sipop_v_lo)
         ,.ready_and_i(sipop_ready_and_li)
         );

      assign multibeat_li = (mem_cast_i.header.size > max_size_lp) & payload_mask_p[mem_cast_i.msg_type];
      always_comb
        begin
          sipop_data_li = mem_cast_i.data;
          sipop_v_li = mem_v_i & multibeat_li;
          mem_ready_and_o = sipop_ready_and_lo & mem_ready_and_i;

          // Override size
          mem_cast_o.header = multibeat_li ? (mem_cast_i.header.size + size_delta_lp) : mem_cast_i.header;
          // TODO: Could probably just use sipop_data_lo here
          mem_cast_o.data = multibeat_li ? sipop_data_lo : mem_cast_i.data;
          mem_v_o = multibeat_li ? sipop_v_lo : mem_v_i;
          sipop_ready_and_li = mem_ready_and_i;
        end
    end

  //synopsys translate_off
  if (in_data_width_p < out_data_width_p)
    $error("Gearbox currently aonly supports large to small");
  //synopsys translate_on

endmodule

