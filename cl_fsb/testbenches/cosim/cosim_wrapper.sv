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
   initial begin
      int exit_code;
      
      tb.power_up();

       $display("Hello\n");
       
      tb.test_main(exit_code);
      
      #10us;
      print_mem();

      tb.power_down();
      
      	 
      $finish;
   end

endmodule // cosim_wrapper
