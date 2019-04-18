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

`include "axil_to_mcl.vh"

module cl_tb();

  import tb_type_defines_pkg::*;
  import cl_mcl_pkg::*;
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

  parameter MST_BASE_ADDR = 32'h0000_0000;
  parameter SLV_BASE_ADDR = 32'h0000_1000;
  parameter MON_BASE_ADDR = 32'h0000_2000;

  parameter DMEM_BASE = 16'h1000;

  logic [31:0] rdata;
  logic [15:0] vdip_value;
  logic [15:0] vled_value;
  logic [127:0] packet;

  `declare_bsg_mcl_request_s;
  `declare_bsg_mcl_response_s;

  bsg_mcl_request_s request_packet;
  bsg_mcl_response_s response_packet;
  
  
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

      // -------------------------------------
      // mcl monitor AXI space test:
      // -------------------------------------
      $display ($time,,,"AXIL monitor space test");

      tb.peek_ocl(.addr(MON_BASE_ADDR + (HOST_RCV_VACANCY_MC_REQ<<2)), .data(rdata));
      $display($time,,,"Readback the rcv FIFO vacancy: %10h", rdata);

      tb.peek_ocl(.addr(MON_BASE_ADDR + (HOST_REQ_CREDITS<<2)), .data(rdata));
      $display($time,,,"Readback the credits: %10h", rdata);

      tb.peek_ocl(.addr(MON_BASE_ADDR + (MC_NUM_X<<2)), .data(rdata));
      $display($time,,,"Readback the x demension: %10h", rdata);

      tb.peek_ocl(.addr(MON_BASE_ADDR + (MC_NUM_Y<<2)), .data(rdata));
      $display($time,,,"Readback the y demension: %10h", rdata);

      // -------------------------------------
      // let's move more manycore-related test to cosim
      // -------------------------------------
      ocl_power_up_init(.FIFO_BASE_ADDR(MST_BASE_ADDR));
      ocl_power_up_init(.FIFO_BASE_ADDR(SLV_BASE_ADDR));
      // ocl_poke_request(.addr(DMEM_BASE>>2), .op(2'b01), .op_ex(4'b1111), .payload(32'hABCD), .src_y_cord(0), .src_x_cord(3), .y_cord(1), .x_cord(0));

      tb.kernel_reset();

      tb.power_down();
      
      $finish;
   end


  task ocl_power_up_init(logic [31:0] FIFO_BASE_ADDR);

    logic [31:0] rd_reg;
    tb.peek_ocl(.addr(FIFO_BASE_ADDR+ISR_REG), .data(rd_reg));
    $display($time,,,"Read ISR: %0h", rd_reg);
    compare_dword(rd_reg, 32'h01d0_0000);

    tb.poke_ocl(.addr(FIFO_BASE_ADDR+ISR_REG), .data(32'hFFFF_FFFF));
    $display($time,,,"Clear ISR");

    tb.peek_ocl(.addr(FIFO_BASE_ADDR+ISR_REG), .data(rd_reg));
    $display($time,,,"Read ISR: %0h", rd_reg);

    tb.peek_ocl(.addr(FIFO_BASE_ADDR+IER_REG), .data(rd_reg));
    $display($time,,,"Read IER: %0h", rd_reg);

    tb.peek_ocl(.addr(FIFO_BASE_ADDR+TDFV_REG), .data(rd_reg));
    $display($time,,,"Read TDFV: %0d", rd_reg);

    tb.peek_ocl(.addr(FIFO_BASE_ADDR+RDFO_REG), .data(rd_reg));
    $display($time,,,"Read RDFO: %0h", rd_reg);
  endtask

  task ocl_poke_request(input logic [31:0] addr, input logic [7:0] op, op_ex, input logic [31:0] payload, input logic [7:0] src_y_cord, src_x_cord, y_cord, x_cord);
    logic [127:0] packet;
    packet = 128'b0;
    request_packet.addr = addr;
    request_packet.op = op;
    request_packet.op_ex = op_ex;
    request_packet.payload = payload;
    request_packet.src_y_cord =src_y_cord;
    request_packet.src_x_cord = src_x_cord;    
    request_packet.y_cord = y_cord;
    request_packet.x_cord = x_cord;
    packet = request_packet;
    ocl_poke_fifo(.FIFO_BASE_ADDR(MST_BASE_ADDR), .packet(packet), .debug_fifo(0));
  endtask

  task ocl_peek_response(output logic [7:0] pkt_type, output logic [31:0] data, load_id, output logic [7:0] y_cord, x_cord);
    logic [127:0] packet;
    ocl_poke_fifo(.FIFO_BASE_ADDR(MST_BASE_ADDR), .packet(packet), .debug_fifo(0));
    pkt_type = response_packet.pkt_type;
    data = response_packet.data;
    load_id = response_packet.load_id;
    y_cord = response_packet.y_cord;
    x_cord = response_packet.x_cord;
  endtask

  task ocl_poke_fifo(logic [31:0] FIFO_BASE_ADDR, input logic [127:0] packet, bit debug_fifo);
      logic [31:0] rd_reg;

      $display($time,,,"Write %h to manycore", packet);
      tb.poke_ocl(.addr(FIFO_BASE_ADDR+IER_REG), .data(32'h0C00_0000));
      if(debug_fifo)
        $display($time,,,"Enable Transmit and Receive Complete interrupts");
      tb.poke_ocl(.addr(FIFO_BASE_ADDR+TDR_REG), .data(32'h0000_0000));
      if(debug_fifo)
        $display($time,,,"Transmit Destination Address 0x0");

      for (int i = 0; i < 4; i++) begin
        tb.poke_ocl(.addr(FIFO_BASE_ADDR+TDFD_REG), .data(packet[32*i+:32]));
      end

      // read TDFV in store-and-forward mode
      #500ns
      tb.peek_ocl(.addr(FIFO_BASE_ADDR+TDFV_REG), .data(rd_reg));
      if(debug_fifo)
        compare_dword(rd_reg, 32'd506);

      tb.poke_ocl(.addr(FIFO_BASE_ADDR+TLR_REG), .data(32'h00000010));
      tb.peek_ocl(.addr(FIFO_BASE_ADDR+ISR_REG), .data(rd_reg));
      if(debug_fifo)
        $display($time,,,"Read ISR: %h", rd_reg);
      // 0800_0000 is for write complete, 0020_0000 is for write FIFO empty
      if(debug_fifo)
        compare_dword(rd_reg, 32'h0820_0000);
      tb.poke_ocl(.addr(FIFO_BASE_ADDR+ISR_REG), .data(32'hFFFF_FFFF));
      if(debug_fifo)
        $display($time,,,"Clear ISR");
      tb.peek_ocl(.addr(FIFO_BASE_ADDR+ISR_REG), .data(rd_reg));
      if(debug_fifo)
        $display($time,,,"Read ISR: %h", rd_reg);
      tb.peek_ocl(.addr(FIFO_BASE_ADDR+TDFV_REG), .data(rd_reg));
      if(debug_fifo)
        $display($time,,,"Transmit FIFO Vacancy is: %h", rd_reg);
  endtask

  task ocl_peek_fifo(logic [31:0] FIFO_BASE_ADDR, output logic [127:0] packet, bit debug_fifo);
    logic [31:0] rd_reg;
    logic [31:0] rev_num;

    tb.peek_ocl(.addr(FIFO_BASE_ADDR+ISR_REG), .data(rd_reg));
    if(debug_fifo)
      $display($time,,,"Read ISR: %h", rd_reg);
    tb.poke_ocl(.addr(FIFO_BASE_ADDR+IER_REG), .data(32'hFFFF_FFFF));
    if(debug_fifo)
      $display($time,,,"Clear ISR");
    tb.peek_ocl(.addr(FIFO_BASE_ADDR+ISR_REG), .data(rd_reg));
    if(debug_fifo)
      $display($time,,,"Read ISR: %h", rd_reg);

    // read RDFO in store-and-forward mode

    tb.peek_ocl(.addr(FIFO_BASE_ADDR+RDFO_REG), .data(rd_reg));
    // if(debug_fifo)  
      $display($time,,,"Receive FIFO Occupancy is: %h", rd_reg);
    // compare_dword(rd_reg, 32'h00000004*(1));

    tb.peek_ocl(.addr(FIFO_BASE_ADDR+RLR_REG), .data(rev_num));
    // if(debug_fifo)  
      $display($time,,,"Read RLR : %h", rev_num);
    tb.peek_ocl(.addr(FIFO_BASE_ADDR+RDR_REG), .data(rd_reg));
    if(debug_fifo)  
      $display($time,,,"Read RDR : %h", rev_num);
    for (int k=0; k<rev_num/16; k++) begin
      for (int i=0; i<4; i++) begin
        tb.peek_ocl(.addr(FIFO_BASE_ADDR+RDFD_REG), .data(rd_reg));
        packet[32*i+:32] = rd_reg;
      end
    $display($time,,,"Read %h from manycore", packet);
    end

    // tb.peek_ocl(.addr(FIFO_BASE_ADDR+ISR_REG), .data(rd_reg));
    if(debug_fifo)  //  
      $display($time,,,"Read ISR: %h", rd_reg);
      // // 0008_0000 is for write FIFO empty
      // compare_dword(rd_reg, 32'h0408_0000);

    tb.peek_ocl(.addr(FIFO_BASE_ADDR+RDFO_REG), .data(rd_reg));
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
