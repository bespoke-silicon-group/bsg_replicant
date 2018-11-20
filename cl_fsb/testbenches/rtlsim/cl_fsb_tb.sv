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


module cl_fsb_tb();

import tb_type_defines_pkg::*;
`include "cl_common_defines.vh" // CL Defines with register addresses

// AXI ID
parameter [5:0] AXI_ID = 6'h0;


logic [31:0] rdata_byte_num;
logic [31:0] rdata_word1;
logic [31:0] rdata_word2;
logic [31:0] rdata_word3;
logic [31:0] rdata_word4;
logic [128:0] rdata_readback;


   initial begin

      tb.power_up();
      $display ("AXI-L to FSB slave test:");

      // initialize
      // Note: only the address signals s_axi_awaddr(5:2) and s_axi_araddr(5:2) are decoded, so base address 32'h80000000 is not mandatory
      tb.poke(.addr(32'h80000000), .data(32'hFFFFFFFF), .id(AXI_ID), .size(DataSize::UINT16), .intf(AxiPort::PORT_OCL));  // Transmit Data FIFO Reset (TDFR) 
      tb.poke(.addr(32'h80000004), .data(32'hFFFFFFFF), .id(AXI_ID), .size(DataSize::UINT16), .intf(AxiPort::PORT_OCL));  // Interrupt Enable Register (IER)

      // write
      for (int i=0; i<4; i++) begin
        tb.poke(.addr(32'h80000010), .data(32'h1 + 4*i), .id(AXI_ID), .size(DataSize::UINT16), .intf(AxiPort::PORT_OCL));
        tb.poke(.addr(32'h80000010), .data(32'h2 + 4*i), .id(AXI_ID), .size(DataSize::UINT16), .intf(AxiPort::PORT_OCL));
        tb.poke(.addr(32'h80000010), .data(32'h3 + 4*i), .id(AXI_ID), .size(DataSize::UINT16), .intf(AxiPort::PORT_OCL));
        tb.poke(.addr(32'h80000010), .data(32'h4 + 4*i), .id(AXI_ID), .size(DataSize::UINT16), .intf(AxiPort::PORT_OCL));
        tb.poke(.addr(32'h80000014), .data(32'h00000010), .id(AXI_ID), .size(DataSize::UINT16), .intf(AxiPort::PORT_OCL));
        // 128 bits are then truncated to 80-bits fsb packet
      end

      // read
      for (int i=0; i<4; i++) begin
        tb.peek_ocl(.addr(32'h80000024), .data(rdata_byte_num));
        if (rdata_byte_num == 32'h00000010) begin
          $display ("READBACK LEN IS 16 BYTES, PASSED~");
        end else begin
          $display ("READBACK LEN FAILED!");
        end
        tb.peek_ocl(.addr(32'h80000020), .data(rdata_word1));
        tb.peek_ocl(.addr(32'h80000020), .data(rdata_word2));
        tb.peek_ocl(.addr(32'h80000020), .data(rdata_word3));
        tb.peek_ocl(.addr(32'h80000020), .data(rdata_word4));
        $display ("FSB READBACK DATA %h %h %h %h", rdata_word1, rdata_word2, rdata_word3, rdata_word4);
        if (rdata_word1 == (32'h00000001+4*i) && rdata_word2 == (32'h00000002+4*i) 
        && rdata_word3 == ((((32'h3+4*i)&32'h0000000F)<<12) + (((32'h3+4*i)>>12)&32'h0000000F))
        && rdata_word4 == 32'h00000000) begin
          $display ("FSB READBACK DATA PASSED~");
        end else begin
          $display ("FSB READBACK DATA FAILED!");
        end
      end

      tb.kernel_reset();
      tb.power_down();

      $finish;
   end

endmodule // test_cl_fsb
