// Amazon FPGA Hardware Development Kit
//
// Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Amazon Software License (the "License"). You may not use
// this file except in compliance with the License. A copy of the License is
// located at
//
//    http://aws.amazon.com/asl/
//
// or in the "license" file accompanying this file. This file is distributed on
// an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, express or
// implied. See the License for the specific language governing permissions and
// limitations under the License.


module cl_m_axi4_fsb_tb();

`define CFG_REG           32'h00
`define CNTL_START        32'h08  // WR
`define CNTL_RESET        32'h0c  // W

`define WR_ADDR_LOW       32'h20
`define WR_ADDR_HIGH      32'h24
`define WR_DATA           32'h28
`define WR_LEN            32'h2c
`define WR_OFFSET         32'h30

`define RD_ADDR_LOW       32'h40
`define RD_ADDR_HIGH      32'h44
`define RD_DATA           32'h48
`define RD_LEN            32'h4c

`define WR_DST_SEL        32'he0

`define WR_SEL            32'h01
`define ED_SEL            32'h02

   import tb_type_defines_pkg::*;

   logic [63:0]  pcim_addr;
   logic [31:0]  pcim_data;

   logic [31:0]  read_data;

   logic [79:0]  hm_data;

   int           timeout_count;

   initial begin

         pcim_addr = 64'h0000_0000_1234_0000;
         pcim_data = 32'h6c93_af50;

         tb.power_up();

         tb.nsec_delay(2000);
         tb.poke_stat(.addr(8'h0c), .ddr_idx(0), .data(32'h0000_0000));
         tb.poke_stat(.addr(8'h0c), .ddr_idx(1), .data(32'h0000_0000));
         tb.poke_stat(.addr(8'h0c), .ddr_idx(2), .data(32'h0000_0000));

         
         $display("[%t] : Programming cl_tst registers for PCIe", $realtime);

         // Read compare
         tb.poke_ocl(.addr(`CFG_REG), .data(32'h0000_0008));
         tb.poke_ocl(.addr(`WR_ADDR_LOW), .data(pcim_addr[31:0]));    // write address low
         tb.poke_ocl(.addr(`WR_ADDR_HIGH), .data(pcim_addr[63:32]));  // write address high
         tb.poke_ocl(.addr(`WR_DATA), .data(pcim_data[31:0]));        // write data, not used
         tb.poke_ocl(.addr(`WR_LEN), .data(32'h0000_0000));           // write 64 bytes, 512bits
         tb.poke_ocl(.addr(`WR_OFFSET), .data(32'h0000_0140-1));      // 320 bytes, 32 fsb pkts
         // Start write
         tb.poke_ocl(.addr(`CNTL_START), .data(`WR_SEL));
         // # 900ns;
         timeout_count = 0;
         do begin
            # 100ns;
            $display("[%t] : Waiting for 1st write activity to complete", $realtime);
            tb.peek_ocl(.addr(`CNTL_START), .data(read_data));
            timeout_count ++;
         end while((read_data[0] !== 1'b0) && timeout_count < 10);

         if ((read_data[0] !== 1'b0) && (timeout_count == 10))
            $display("[%t] : *** ERROR *** Timeout waiting for 1st writes to complete.", $realtime);
         else
            $display("[%t] : PASS~~~ axi4 1st writes complete.", $realtime);

         tb.peek_ocl(.addr(`CNTL_START), .data(read_data));
         if (read_data[3])
            $display("[%t] : axi4 1st writes RESP ERROR.", $realtime);


         # 1000ns;  // ensure that axi packets are written into host memory
         for (int i=0; i<32; i++) begin
            for(int k=0; k<10; k++) begin
               hm_data[(k*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+10*i+k));
            end
            $display("read host @ memory %h ~ %h: %h", pcim_addr+10*i, pcim_addr+10*i+9, hm_data);
         end

         tb.power_down();

         $finish;
   end

endmodule // cl_m_axi4_fsb_tb
