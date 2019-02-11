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
`define AXI_MEMORY_MODEL
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

  parameter in_addr_width=27;
  parameter in_data_width=32;
  parameter in_x_cord_width=2;
  parameter in_y_cord_width=3;
  parameter load_id_width_p=11;

  `declare_bsg_manycore_packet_s(in_addr_width,in_data_width,in_x_cord_width,in_y_cord_width,load_id_width_p);
  parameter packet_width_lp = `bsg_manycore_packet_width(in_addr_width,in_data_width,in_x_cord_width,in_y_cord_width,load_id_width_p);
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
      // ocl_poke_cmd(.fifo_num(0), .addr(1<<22), .op(2'b01), .op_ex(4'b1111), .payload(32'b0), .src_y_cord(3'b100), .src_x_cord(2'b11), .y_cord(3'b101), .x_cord(2'b00));

      tb.peek_ocl(.addr(32'h0000_0200), .data(rdata));
      $display($time,,,"Readback the Vacancy%h", rdata);

      tb.peek_ocl(.addr(32'h0000_0210), .data(rdata));
      $display($time,,,"Readback the Credits%h", rdata);

      tb.peek_ocl(.addr(32'h0000_0220), .data(rdata));
      $display($time,,,"Readback the x Demension%h", rdata);
      tb.peek_ocl(.addr(32'h0000_0220), .data(rdata));
      $display($time,,,"Readback the y Demension%h", rdata);

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

  task ocl_poke_cmd( 
    int fifo_num,
    logic [in_addr_width-1:0] addr, logic [1:0] op, logic [in_data_width>>3-1:0] op_ex, logic [in_data_width-1:0] payload,
    logic [in_y_cord_width-1:0] src_y_cord, y_cord, logic [in_x_cord_width-1:0] src_x_cord, x_cord);
    bsg_manycore_packet_s packet_cast;
    logic [127:0] packet;
    packet = 128'b0;
    packet_cast.addr = addr;
    packet_cast.op = op;
    packet_cast.op_ex = op_ex;
    packet_cast.payload = payload;
    packet_cast.src_y_cord =src_y_cord;
    packet_cast.src_x_cord = src_x_cord;    
    packet_cast.y_cord = y_cord;
    packet_cast.x_cord = x_cord;
    packet[0+:packet_width_lp] = packet_cast;
    if (fifo_num==0) begin
      ocl_poke_test(.CFG_BASE_ADDR(32'h0000_0000), .packet(packet), .debug_fifo(0));
    end
    else if(fifo_num==1) begin
      ocl_poke_test(.CFG_BASE_ADDR(32'h0000_0100), .packet(packet), .debug_fifo(0));
    end
  endtask

  task ocl_poke_test(logic [31:0] CFG_BASE_ADDR, logic [127:0] packet, bit debug_fifo);

      logic [31:0] rd_reg;

      // write
      $display($time,,,"Write %h to manycore", packet);
      tb.poke_ocl(.addr(CFG_BASE_ADDR+IER_REG), .data(32'h0C00_0000));
      if(debug_fifo)
        $display($time,,,"Enable Transmit and Receive Complete interrupts");
      tb.poke_ocl(.addr(CFG_BASE_ADDR+TDR_REG), .data(32'h0000_0000));
      if(debug_fifo)
        $display($time,,,"Transmit Destination Address 0x0");

      for (int i = 0; i < 4; i++) begin
        tb.poke_ocl(.addr(CFG_BASE_ADDR+TDFD_REG), .data(packet[32*i+:32]));
      end

      // read TDFV in store-and-forward mode
      #500ns
      tb.peek_ocl(.addr(CFG_BASE_ADDR+TDFV_REG), .data(rd_reg));
      if(debug_fifo)
        compare_dword(rd_reg, 32'd506);

      tb.poke_ocl(.addr(CFG_BASE_ADDR+TLR_REG), .data(32'h00000010));
      tb.peek_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(rd_reg));
      if(debug_fifo)
        $display($time,,,"Read ISR: %h", rd_reg);
      // 0800_0000 is for write complete, 0020_0000 is for write FIFO empty
      if(debug_fifo)
        compare_dword(rd_reg, 32'h0820_0000);
      tb.poke_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(32'hFFFF_FFFF));
      if(debug_fifo)
        $display($time,,,"Clear ISR");
      tb.peek_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(rd_reg));
      if(debug_fifo)
        $display($time,,,"Read ISR: %h", rd_reg);
      tb.peek_ocl(.addr(CFG_BASE_ADDR+TDFV_REG), .data(rd_reg));
      if(debug_fifo)
        $display($time,,,"Transmit FIFO Vacancy is: %h", rd_reg);

  endtask

  task ocl_peek_test(logic [31:0] CFG_BASE_ADDR, logic [127:0] cmp_pkt, bit debug_fifo);

    logic [31:0] rd_reg;
    logic [31:0] rev_num;
    logic [127:0] packet;

    tb.peek_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(rd_reg));
    if(debug_fifo)
      $display($time,,,"Read ISR: %h", rd_reg);
    tb.poke_ocl(.addr(CFG_BASE_ADDR+IER_REG), .data(32'hFFFF_FFFF));
    if(debug_fifo)
      $display($time,,,"Clear ISR");
    tb.peek_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(rd_reg));
    if(debug_fifo)
      $display($time,,,"Read ISR: %h", rd_reg);

    // read RDFO in store-and-forward mode

    tb.peek_ocl(.addr(CFG_BASE_ADDR+RDFO_REG), .data(rd_reg));
    // if(debug_fifo)  
      $display($time,,,"Receive FIFO Occupancy is: %h", rd_reg);
    // compare_dword(rd_reg, 32'h00000004*(1));

    tb.peek_ocl(.addr(CFG_BASE_ADDR+RLR_REG), .data(rev_num));
    // if(debug_fifo)  
      $display($time,,,"Read RLR : %h", rev_num);
    tb.peek_ocl(.addr(CFG_BASE_ADDR+RDR_REG), .data(rd_reg));
    if(debug_fifo)  
      $display($time,,,"Read RDR : %h", rev_num);
    for (int k=0; k<rev_num/16; k++) begin
      for (int i=0; i<4; i++) begin
        tb.peek_ocl(.addr(CFG_BASE_ADDR+RDFD_REG), .data(rd_reg));
        packet[32*i+:32] = rd_reg;
      end
    $display($time,,,"Read %h from manycore", packet);
    end

    // tb.peek_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(rd_reg));
    if(debug_fifo)  //  
      $display($time,,,"Read ISR: %h", rd_reg);
      // // 0008_0000 is for write FIFO empty
      // compare_dword(rd_reg, 32'h0408_0000);

    tb.peek_ocl(.addr(CFG_BASE_ADDR+RDFO_REG), .data(rd_reg));
    if(debug_fifo)  
      $display($time,,,"Receive FIFO Occupancy is: %h", rd_reg);

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
    tb.nsec_delay(100);
  endtask


endmodule // cl_tb
