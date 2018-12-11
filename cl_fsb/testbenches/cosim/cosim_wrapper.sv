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
   
	int num_pops, pop_size;
	
	initial begin
      int exit_code, pop_fail;
      
      tb.power_up();

       
      tb.test_main(exit_code); /* initiates device write */
      #10us;				   
      
      check_mem();			   /* check buffer data against pattern */
	  
	$display("Now start popping off data.\n");
	while (num_pops <= 15) 
	begin	
		pop_size = (num_pops % 2) ? 64 : 128;
		tb.pop_data(pop_size, pop_fail);
		if(!pop_fail)
			num_pops++;
		else
			num_pops++;
		#1us;
	end
	tb.power_down();		
	$finish;
   end

endmodule // cosim_wrapper
