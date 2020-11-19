// This module is a non-synthesizable replacement for bsg_encode_one_hot
`include "bsg_defines.v"

module bsg_encode_one_hot #(parameter width_p=8, parameter lo_to_hi_p=1, parameter debug_p=0)
(input [width_p-1:0] i
 ,output [`BSG_SAFE_CLOG2(width_p)-1:0] addr_o
 ,output v_o // whether any bits are set
);
   logic v_lo;
   logic [`BSG_SAFE_CLOG2(width_p)-1:0] addr_lo;
   
   always_comb begin
      v_lo = $onehot(i);
      addr_lo = '0;
      if($onehot(i))
        if(lo_to_hi_p)
          for(addr_lo = '0; ~i[addr_lo]; addr_lo++);
        else
          for(addr_lo = `BSG_SAFE_CLOG2(width_p)-1; ~i[addr_lo]; addr_lo--);
      else
        addr_lo = '0;
   end

   assign v_o = v_lo;
   assign addr_o = addr_lo;
endmodule
