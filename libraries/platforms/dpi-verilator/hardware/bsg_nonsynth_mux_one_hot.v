// This module is a non-synthesizable replacement for bsg_mux_one_hot
`include "bsg_defines.v"

module bsg_mux_one_hot #(parameter width_p="inv"
                         , els_p=1
                         , harden_p=1)
   (input [els_p-1:0][width_p-1:0] data_i
    ,input [els_p-1:0] sel_one_hot_i
    ,output [width_p-1:0] data_o
    );

   initial begin
      $display("  __/|___/|___/|| |     / /___ __________  (_)___  ____ ___/|___/|___/|_");
      $display(" |    /    /    / | /| / / __ `/ ___/ __ \/ / __ \/ __ `/    /    /    /");
      $display("/_ __/_ __/_ __|| |/ |/ / /_/ / /  / / / / / / / / /_/ /_ __/_ __/_ __| ");
      $display(" |/   |/   |/   |__/|__/\__,_/_/  /_/ /_/_/_/ /_/\__, / |/   |/   |/    ");
      $display("                                                /____/                  ");
      $display("  *** This instantiation of bsg_mux_one_hot is non-synthesizable  ***   ");
      $display("  *** Instantiation path: %m ***         ");
   end

   // synopsys translate_off
   logic [width_p-1:0]            data_lo;
   assign data_o = data_lo;

   always_comb begin
     data_lo = '0;

     for (int i = 0; i < els_p; i++)
       begin
          if(sel_one_hot_i[i] === 1'b1)
            data_lo |= data_i[i];
       end
   end
   // synopsys translate_on
endmodule

