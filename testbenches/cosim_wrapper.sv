// Copyright (c) 2019, University of Washington All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list
// of conditions and the following disclaimer.
// 
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
// 
// Neither the name of the copyright holder nor the names of its contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

module cosim_wrapper();
   import cl_manycore_pkg::*;
   import bsg_bladerunner_mem_cfg_pkg::*;
   initial begin
      int exit_code;
      string args;
      longint t;
      $value$plusargs("c_args=%s", args);

      $display("==================== BSG MACHINE SETTINGS: ====================");
      $display("[INFO][TESTBENCH] BSG_MACHINE_GLOBAL_X                 = %d", num_tiles_x_p);
      $display("[INFO][TESTBENCH] BSG_MACHINE_GLOBAL_Y                 = %d", num_tiles_y_p);
      $display("[INFO][TESTBENCH] BSG_MACHINE_VCACHE_SET               = %d", sets_p);
      $display("[INFO][TESTBENCH] BSG_MACHINE_VCACHE_WAY               = %d", ways_p);
      $display("[INFO][TESTBENCH] BSG_MACHINE_VCACHE_BLOCK_SIZE_WORDS  = %d", block_size_in_words_p);
      $display("[INFO][TESTBENCH] BSG_MACHINE_MAX_EPA_WIDTH            = %d", addr_width_p);
      $display("[INFO][TESTBENCH] BSG_MACHINE_MEM_CFG                  = %s", mem_cfg_p.name());

      tb.power_up();

      if (mem_cfg_p == e_vcache_blocking_axi4_f1_ddrx4 ||
          mem_cfg_p == e_vcache_blocking_axi4_f1_ddr_modelx4) begin
         // initialize the DDR
         $display("BSG COSIM: Starting DDR init...\n");
         tb.nsec_delay(500);
         tb.poke_stat(.addr(8'h0c), .ddr_idx(0), .data(32'h0000_0000));
         tb.poke_stat(.addr(8'h0c), .ddr_idx(1), .data(32'h0000_0000));
         tb.poke_stat(.addr(8'h0c), .ddr_idx(2), .data(32'h0000_0000));
         // allow memory to initialize
         tb.nsec_delay(27000);
         $display("BSG COSIM: Done DDR init...\n");
      end

      tb.cosim_main(exit_code, args);
      
      #50ns;
       
      tb.power_down();
      if(exit_code < 0) 
          $display("BSG COSIM FAIL: Test failed with exit code: %d", exit_code);
      else 
          $display("BSG COSIM PASS: Test passed!");
          
      $finish;
   end

endmodule // cosim_wrapper


