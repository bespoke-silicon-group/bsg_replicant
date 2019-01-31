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

`include "bsg_manycore_packet.vh"

module cl_tb();

import tb_type_defines_pkg::*;

// AXI ID
parameter [5:0] AXI_ID = 6'h0;

  parameter ISR_REG = 32'h0000_0000;
  parameter IER_REG = 32'h0000_0004;

  parameter TDR_REG = 32'h0000_002C;
  parameter RDR_REG = 32'h0000_0030;

  parameter TDFV_REG = 32'h0000_000C;
  parameter RDFO_REG = 32'h0000_001C;

  parameter TLR_REG = 32'h0000_0014;
  parameter RLR_REG = 32'h0000_0024;

  parameter TDFD_REG = 32'h0000_0010;
  parameter RDFD_REG = 32'h0000_0020;

  logic [31:0] rdata;
  logic [15:0] vdip_value;
  logic [15:0] vled_value;
  logic [127:0] packet;
  `declare_bsg_manycore_packet_s(26,32,2,3,11);
  parameter packet_width_lp = `bsg_manycore_packet_width(26,32,2,3,11);
  bsg_manycore_packet_s packet_cast;
  
  
   initial begin

      tb.power_up(
          .clk_recipe_a(ClockRecipe::A1),
          .clk_recipe_b(ClockRecipe::B0),
          .clk_recipe_c(ClockRecipe::C0)
      );
      poke_ddr_stat();
      tb.set_virtual_dip_switch(.dip(0));

      vdip_value = tb.get_virtual_dip_switch();

      $display ("value of vdip:%0x", vdip_value);

      vled_value = tb.get_virtual_led();

      $display ("value of vled:%0x", vled_value);

      // begin here
      ocl_power_up_init(.CFG_BASE_ADDR('0));
      
      packet = 128'b0;
      packet_cast.addr = (1 << 25);
      packet_cast.op = 2'b01;
      packet_cast.op_ex = 4'b1111;
      packet_cast.payload = 32'b0;
      packet_cast.src_y_cord = 3'b111;
      packet_cast.src_x_cord = 2'b11;    
      packet_cast.y_cord = 3'b111;
      packet_cast.x_cord = 2'b00;
      packet[0+:packet_width_lp] = packet_cast;
  
      ocl_FSB_poke_test(.CFG_BASE_ADDR(0), .packet(packet));

      packet = 128'b0;
      packet_cast.addr = (1 << 25) + (1<<9);
      packet_cast.op = 2'b01;
      packet_cast.op_ex = 4'b1111;
      packet_cast.payload = 32'b0;
      packet_cast.src_y_cord = 3'b111;
      packet_cast.src_x_cord = 2'b11;    
      packet_cast.y_cord = 3'b111;
      packet_cast.x_cord = 2'b00;
      packet[0+:packet_width_lp] = packet_cast;
  
      ocl_FSB_poke_test(.CFG_BASE_ADDR(0), .packet(packet));

      packet = 128'b0;
      packet_cast.addr = (1 << 25) + (1<<9);
      packet_cast.op = 2'b01;
      packet_cast.op_ex = 4'b1111;
      packet_cast.payload = 32'b0;
      packet_cast.src_y_cord = 3'b111;
      packet_cast.src_x_cord = 2'b11;    
      packet_cast.y_cord = 3'b111;
      packet_cast.x_cord = 2'b00;
      packet[0+:packet_width_lp] = packet_cast;
      
      ocl_FSB_poke_test(.CFG_BASE_ADDR(0), .packet(packet));

      packet = 128'b0;
      packet_cast.addr = (1 << 25) + (1<<9);
      packet_cast.op = 2'b01;
      packet_cast.op_ex = 4'b1111;
      packet_cast.payload = 32'b0;
      packet_cast.src_y_cord = 3'b111;
      packet_cast.src_x_cord = 2'b11;    
      packet_cast.y_cord = 3'b111;
      packet_cast.x_cord = 2'b00;
      packet[0+:packet_width_lp] = packet_cast;
      
      ocl_FSB_poke_test(.CFG_BASE_ADDR(0), .packet(packet));

      packet = 128'b0;
      packet_cast.addr = 0;
      packet_cast.op = 2'b01;
      packet_cast.op_ex = 4'b1111;
      packet_cast.payload = 32'hdeadbeef;
      packet_cast.src_y_cord = 3'b111;
      packet_cast.src_x_cord = 2'b11;    
      packet_cast.y_cord = 3'b111;
      packet_cast.x_cord = 2'b00;
      packet[0+:packet_width_lp] = packet_cast;
  
      ocl_FSB_poke_test(.CFG_BASE_ADDR(0), .packet(packet));

      #10000ns;
      tb.kernel_reset();

      tb.power_down();
      
      $finish;
   end


  task ocl_power_up_init(logic [31:0] CFG_BASE_ADDR);

    logic [31:0] rd_reg;
    tb.peek_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(rd_reg));
    $display($time,,,"Read ISR: %0h", rd_reg);
    compare_dword(rd_reg, 32'h01d0_0000);



    tb.poke_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(32'hFFFF_FFFF));
    $display($time,,,"Clear ISR");



    tb.peek_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(rd_reg));
    $display($time,,,"Read ISR: %0h", rd_reg);



    tb.peek_ocl(.addr(CFG_BASE_ADDR+IER_REG), .data(rd_reg));
    $display($time,,,"Read IER: %0h", rd_reg);



    tb.peek_ocl(.addr(CFG_BASE_ADDR+TDFV_REG), .data(rd_reg));
    $display($time,,,"Read TDFV: %0h", rd_reg);



    tb.peek_ocl(.addr(CFG_BASE_ADDR+RDFO_REG), .data(rd_reg));
    $display($time,,,"Read RDFO: %0h", rd_reg);

  endtask

task ocl_FSB_poke_test(logic [31:0] CFG_BASE_ADDR, logic [127:0] packet);

    logic [31:0] rd_reg;

    // write

      tb.poke_ocl(.addr(CFG_BASE_ADDR+IER_REG), .data(32'h0C00_0000));
      $display($time,,,"Enable Transmit and Receive Complete interrupts");



      tb.poke_ocl(.addr(CFG_BASE_ADDR+TDR_REG), .data(32'h0000_0000));
      $display($time,,,"Transmit Destination Address 0x0");

      for (int i = 0; i < 4; i++) begin
        tb.poke_ocl(.addr(CFG_BASE_ADDR+TDFD_REG), .data(packet[32*i+:32]));
      end

      // read TDFV in store-and-forward mode

      #500ns
      tb.peek_ocl(.addr(CFG_BASE_ADDR+TDFV_REG), .data(rd_reg));
      compare_dword(rd_reg, 32'd506);

      tb.poke_ocl(.addr(CFG_BASE_ADDR+TLR_REG), .data(32'h00000010));

      tb.peek_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(rd_reg));
      $display($time,,,"Read ISR: %h", rd_reg);

      // 0800_0000 is for write complete, 0020_0000 is for write FIFO empty

      compare_dword(rd_reg, 32'h0820_0000);

      tb.poke_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(32'hFFFF_FFFF));
      $display($time,,,"Clear ISR");

      tb.peek_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(rd_reg));
      $display($time,,,"Read ISR: %h", rd_reg);

      tb.peek_ocl(.addr(CFG_BASE_ADDR+TDFV_REG), .data(rd_reg));
      $display($time,,,"Transmit FIFO Vacancy is: %h", rd_reg);

  endtask


  task compare_dword(logic [32:0] act_data, exp_data);

    if(act_data !== exp_data) begin
        $display("***ERROR*** : Data Mismatch!!! Expected Data: %0h, Actual   Data: %0h", exp_data, act_data);
    end
    else begin
        $display("~~~PASS~~~ : Actual Data %0h is Matched with Expected Data %0h.", act_data, exp_data);
    end

  endtask

  task poke_ddr_stat();
    $display("[%t] : Start poking ddr stats", $realtime);
    tb.nsec_delay(100);
    tb.poke_stat(.addr(8'h0c), .ddr_idx(0), .data(32'h0000_0000));
    tb.poke_stat(.addr(8'h0c), .ddr_idx(1), .data(32'h0000_0000));
    tb.poke_stat(.addr(8'h0c), .ddr_idx(2), .data(32'h0000_0000));
    tb.nsec_delay(27000);
  endtask


endmodule // cl_tb
