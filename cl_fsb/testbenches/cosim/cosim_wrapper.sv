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



module cosim_wrapper();
	import "DPI-C" context task print_mem();
	import "DPI-C" context task check_mem();
	import "DPI-C" context task pop_data();
	import "DPI-C" context task disable_datagen();
	import "DPI-C" context task enable_datagen();
  	
  	parameter CROSSBAR_M1 = 32'h0000_1000;
	parameter CFG_REG    = CROSSBAR_M1+32'h00;
	parameter CNTL_START = CROSSBAR_M1+32'h8; 			// WR
	parameter WR_SEL = CROSSBAR_M1+32'h1;
	int num_pops, pop_size;
	
	initial begin
      int exit_code, pop_fail;
      
      tb.power_up();

       
      tb.test_main(exit_code); /* initiates device write */
      #10us;				   
      
      check_mem();			   /* check buffer data against pattern */
	  
	$display("Now start popping off data.\n");
	while (num_pops <= 20) 
	begin
		pop_size = (num_pops % 2) ? 64 : 128;
		tb.pop_data(pop_size, pop_fail);
		if(!pop_fail) begin
			$display("0x%h successful pops", num_pops);
		end
		if (num_pops == 4) begin
    		     //tb.poke_ocl(.addr(CFG_REG), .data(32'h0000_0000));          // set mask to enable fsb_wvalid
		     //$display("disabled datagen.\n");
			tb.disable_datagen();
		end
		else if (num_pops == 13) begin
    		tb.enable_datagen();	
	//		tb.poke_ocl(.addr(CFG_REG), .data(32'h0000_0010));          // set mask to enable fsb_wvalid
	//		//tb.poke_ocl(.addr(WR_ADDR_LOW), .data(low32)); // write address low
	//		//rc = fpga_pci_poke(pci_bar_handle, WR_ADDR_HIGH, high32); // write address high
   	//		//rc = fpga_pci_poke(pci_bar_handle, WR_HEAD, head);     
   	//		//rc = fpga_pci_poke(pci_bar_handle, WR_LEN, 0x0); // write 64 bytes, 512bits
    	//		// rc = fpga_pci_poke(pci_bar_handle, WR_OFFSET, (uint32_t) (buffer_size-1)); 
	//		// tb.poke_ocl(.addr(CNTL_START), .data(WR_SEL));
	//		$display("enabled datagen.\n");
		end
		num_pops++;
		#1us;
	end
	tb.power_down();		
	$finish;
   end

endmodule // cosim_wrapper
