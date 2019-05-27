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
`include "cl_manycore_pkg.v"
import cl_manycore_pkg::*;

module cl_tb();

  import tb_type_defines_pkg::*;
  import cl_mcl_pkg::*;
  // AXI ID
  parameter [5:0] AXI_ID = 6'h0;

  parameter ISR_REG = 32'h0000_0000;

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
	parameter DMEM_DATA = 32'hCDEF_0123;
  parameter LOAD_ID 	=	32'h4567_89AB;

  parameter axil_fifo_els_lp = 256;

	logic [7:0] pkt_type, y_cord, x_cord;
  logic [31:0] rdata;
  logic [31:0] load_id;
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
			$display("");
			$display("======= AXI-Lite Address Monitor test =======");	
			$display("");
      tb.peek_ocl(.addr(MON_BASE_ADDR + (HOST_RCV_VACANCY_MC_REQ)), .data(rdata));
      $display($time,,,"Readback the rcv FIFO vacancy: %d", rdata);

      tb.peek_ocl(.addr(MON_BASE_ADDR + (HOST_REQ_CREDITS)), .data(rdata));
      $display($time,,,"Readback the credits: %d", rdata);
      
      // TODO: remove magic number for rom address.
      tb.peek_ocl(.addr(MON_BASE_ADDR + (4<<2)), .data(rdata));
      $display($time,,,"Readback the x demension: %d", rdata);

      tb.peek_ocl(.addr(MON_BASE_ADDR + (5<<2)), .data(rdata));
      $display($time,,,"Readback the y demension: %d", rdata);

      // -------------------------------------
      // let's move more manycore-related test to cosim
      // -------------------------------------
			$display("");
			$display("======= Manycore NPA Read and Write Test =======");	
			$display("");
			ocl_power_up_init(.FIFO_BASE_ADDR(MST_BASE_ADDR), .debug_fifo(1));
			ocl_power_up_init(.FIFO_BASE_ADDR(SLV_BASE_ADDR), .debug_fifo(1));
			// write to dmem
      ocl_poke_request(.addr(DMEM_BASE>>2), .op(2'b01), .op_ex(4'b1111), .payload(DMEM_DATA), .src_y_cord(0), .src_x_cord(0), .y_cord(1), .x_cord(0));
			// read from dmem
      ocl_poke_request(.addr(DMEM_BASE>>2), .op(2'b00), .op_ex(4'b1111), .payload(LOAD_ID), .src_y_cord(0), .src_x_cord(0), .y_cord(1), .x_cord(0));
			#500ns
			// read from receive fifo
			ocl_peek_response(.pkt_type(pkt_type), .data(rdata), .load_id(load_id), .y_cord(y_cord), .x_cord(x_cord));
			// compare result
			compare_dword(rdata, data_width_p'(DMEM_DATA));
			compare_dword(load_id, load_id_width_p'(LOAD_ID));
			tb.kernel_reset();

      tb.power_down();

      $finish;
   end


  task ocl_power_up_init(logic [31:0] FIFO_BASE_ADDR, bit debug_fifo);

    logic [31:0] rd_reg;
    tb.poke_ocl(.addr(FIFO_BASE_ADDR+ISR_REG), .data(32'h0));
		if (debug_fifo)
			$display($time,,,"Clear ISR");

    tb.peek_ocl(.addr(FIFO_BASE_ADDR+ISR_REG), .data(rd_reg));
   	if (debug_fifo) begin 
			$display($time,,,"Read ISR: %0h", rd_reg);
  	 	compare_dword(rd_reg, 32'h0000_0000);
		end

    tb.peek_ocl(.addr(FIFO_BASE_ADDR+TDFV_REG), .data(rd_reg));
		if (debug_fifo) begin
	    $display($time,,,"Read TDFV: %0d", rd_reg);
		end

    tb.peek_ocl(.addr(FIFO_BASE_ADDR+RDFO_REG), .data(rd_reg));
   	if (debug_fifo) 
			$display($time,,,"Read RDFO: %0h", rd_reg);
  endtask

  task ocl_poke_request(input logic [31:0] addr, input logic [7:0] op, op_ex, input logic [31:0] payload, input logic [7:0] src_y_cord, src_x_cord, y_cord, x_cord);
    logic [127:0] packet;
    packet = 128'b0;
		request_packet.padding = '0;
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
    ocl_peek_fifo(.FIFO_BASE_ADDR(MST_BASE_ADDR), .packet(packet), .debug_fifo(1));
		response_packet = packet;
    pkt_type = response_packet.pkt_type;
    data = response_packet.data;
    load_id = response_packet.load_id;
    y_cord = response_packet.y_cord;
    x_cord = response_packet.x_cord;
  endtask

  task ocl_poke_fifo(input logic [31:0] FIFO_BASE_ADDR, input logic [127:0] packet, bit debug_fifo);
      logic [31:0] rd_reg;

      // read TDFV in store-and-forward mode
      $display($time,,,"Write %h to manycore", packet);
      for (int i = 0; i < 4; i++) begin
        tb.poke_ocl(.addr(FIFO_BASE_ADDR+TDFD_REG), .data(packet[32*i+:32]));
      end
   
      // enqueue tx fifo 
      #200ns
      tb.peek_ocl(.addr(FIFO_BASE_ADDR+TDFV_REG), .data(rd_reg));
			if(debug_fifo) begin
        $display($time,,,"Read TDFV: %h", rd_reg);
        compare_dword(rd_reg, axil_fifo_els_lp-4);
			end
  
      // dequeue tx fifo, check ISR 
      tb.poke_ocl(.addr(FIFO_BASE_ADDR+TLR_REG), .data(32'h00000010));
      tb.peek_ocl(.addr(FIFO_BASE_ADDR+ISR_REG), .data(rd_reg));
			if(debug_fifo) begin
        $display($time,,,"Read ISR: %h", rd_reg);
        compare_dword(rd_reg, 32'h0800_0000);
			end

      // check vacancy  
      #200ns
      tb.peek_ocl(.addr(FIFO_BASE_ADDR+TDFV_REG), .data(rd_reg));
      if(debug_fifo)
        $display($time,,,"Transmit FIFO Vacancy is: %h", rd_reg);
  endtask

  task ocl_peek_fifo(input logic [31:0] FIFO_BASE_ADDR, output logic [127:0] packet, input bit debug_fifo);
    logic [31:0] rd_reg;
    logic [31:0] rev_num;

    // read RDFO in store-and-forward mode
    tb.peek_ocl(.addr(FIFO_BASE_ADDR+RDFO_REG), .data(rd_reg));
    if(debug_fifo)
      $display($time,,,"Receive FIFO Occupancy is: %h", rd_reg);
	  
		// read RLR
    tb.peek_ocl(.addr(FIFO_BASE_ADDR+RLR_REG), .data(rev_num));
    if(debug_fifo)
      $display($time,,,"Read RLR : %h", rev_num);
    for (int k=0; k<rev_num/16; k++) begin
      for (int i=0; i<4; i++) begin
        tb.peek_ocl(.addr(FIFO_BASE_ADDR+RDFD_REG), .data(rd_reg));
        packet[32*i+:32] = rd_reg;
      end
    $display($time,,,"Read %h from manycore", packet);
    end
		
		// check occupancy again
    tb.peek_ocl(.addr(FIFO_BASE_ADDR+RDFO_REG), .data(rd_reg));
    if(debug_fifo)
      $display($time,,,"Receive FIFO Occupancy is: %h", rd_reg);
  endtask

  task compare_dword(logic [32:0] act_data, exp_data);
    if(act_data !== exp_data) 
        $display("***ERROR*** : Data Mismatch!!! Expected Data: %0h, Actual   Data: %0h", exp_data, act_data);
    else 
        $display("~~~PASS~~~ : Actual Data %0h is Matched with Expected Data %0h.", act_data, exp_data);
		$display("");
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
