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
  `define WR_READ_HEAD      32'h28
  `define WR_LEN            32'h2c
  `define WR_OFFSET         32'h30

  `define RD_ADDR_LOW       32'h40
  `define RD_ADDR_HIGH      32'h44
  `define RD_DATA           32'h48
  `define RD_LEN            32'h4c

  `define WR_DST_SEL        32'he0

  `define WR_SEL            32'h01
  `define ED_SEL            32'h02

  `define WR_BUFF_SIZE      32'h280
  `define WR_TAIL_OFFSET    64'h280

  import tb_type_defines_pkg::*;

  logic [63:0] pcim_addr;
  logic [31:0] pcim_data;

  logic [31:0] cfg_rdata;

  logic [79:0] hm_fsb_pkt    ;
  logic [63:0] hm_tail_old   ;
  logic [63:0] hm_tail       ;
  logic [ 7:0] hm_byte       ;
  bit          tail_change   ;
  bit          fsb_valid_mask;

  int timeout_count;
  initial begin

    pcim_addr = 64'h0000_0000_1234_0000;
    pcim_data = 32'h6c93_af50;

    tb.power_up();

    tb.nsec_delay(2000);
    tb.poke_stat(.addr(8'h0c), .ddr_idx(0), .data(32'h0000_0000));
    tb.poke_stat(.addr(8'h0c), .ddr_idx(1), .data(32'h0000_0000));
    tb.poke_stat(.addr(8'h0c), .ddr_idx(2), .data(32'h0000_0000));


    $display("[%t] : Programming cl_tst registers for PCIe", $realtime);

    tb.poke_ocl(.addr(`CFG_REG), .data(32'h0000_0010));          // set mask to enable fsb_wvalid
    tb.poke_ocl(.addr(`WR_ADDR_LOW), .data(pcim_addr[31:0]));    // write address low
    tb.poke_ocl(.addr(`WR_ADDR_HIGH), .data(pcim_addr[63:32]));  // write address high
    tb.poke_ocl(.addr(`WR_LEN), .data(32'h0000_0000));           // write 64 bytes, 512bits
    tb.poke_ocl(.addr(`WR_OFFSET), .data(`WR_BUFF_SIZE-1));      // 320 bytes, 32 fsb pkts
    // Start write
    tb.poke_ocl(.addr(`CNTL_START), .data(`WR_SEL));


    // test fsb wvalid pauses
    // --------------------------------
    fsb_valid_mask = 0;
    for (int i=0; i<3; i++) begin
      fsb_valid_mask = ~fsb_valid_mask;
      tb.poke_ocl(.addr(`CFG_REG), .data(32'h0000_0000 | (fsb_valid_mask << 4)));
      # 100ns;
    end

    // wait until the buffer is full
    // --------------------------------
    timeout_count = 0;
    do begin
      # 50ns;
      // check if is still writting
      tb.peek_ocl(.addr(`CNTL_START), .data(cfg_rdata));
      $display("[%t] : Waiting for 1st write activity to complete", $realtime);
      timeout_count ++;
    end while((cfg_rdata[0] !== 1'b0) && timeout_count < 10);

    if ((cfg_rdata[0] !== 1'b0) && (timeout_count == 10))
      $display("[%t] : *** ERROR *** Timeout waiting for 1st writes to complete.", $realtime);
    else
      $display("[%t] : PASS ~~~ axi4 1st writes complete.", $realtime);


    // check if the axi-4 bus is free of error
    // --------------------------------
    tb.peek_ocl(.addr(`CNTL_START), .data(cfg_rdata));
    if (cfg_rdata[3])
      $display("[%t] : *** ERROR *** axi4 1st writes RESP is NOT OKAY.", $realtime);
    // --------------------------------


    // read x2, update the head, so more data from fsb should come
    // --------------------------------

    // read from 0 to WR_BUFF_SIZE-0x40
    for (int i=0; i<`WR_BUFF_SIZE/64; i++)
      begin
        // check the write tail
        for (int j=0; j<8; j++) begin
          hm_tail[(j*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+`WR_TAIL_OFFSET+j));
        end
        $display("[%t] : Readback the write tail word @ %h : %h", $realtime, pcim_addr+`WR_TAIL_OFFSET, hm_tail[31:0]);
        # 500ns
          if ((i*32'h40)!=hm_tail[31:0]) begin  // check current read head is not equal to tail
            tb.poke_ocl(.addr(`WR_READ_HEAD), .data(32'h0000_0000 + i*32'h40));
          end
      end

    // read again, from 0 to WR_BUFF_SIZE-0x40
    for (int i=0; i<`WR_BUFF_SIZE/64; i++)
      begin
        // check the write tail
        for (int j=0; j<8; j++) begin
          hm_tail[(j*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+`WR_TAIL_OFFSET+j));
        end
        $display("[%t] : Readback the write tail word @ %h : %h", $realtime, pcim_addr+`WR_TAIL_OFFSET, hm_tail[31:0]);
        # 500ns
          if ((i*32'h40)!=hm_tail[31:0]) begin  // check current read head is not equal to tail
            tb.poke_ocl(.addr(`WR_READ_HEAD), .data(32'h0000_0000 + i*32'h40));
          end
      end
    // --------------------------------


    // --------------------------------
    // bfm backdoor check
    // --------------------------------

    // check if the tail is not updating any more
    timeout_count = 0;
    hm_tail_old = 0;
    do begin
      # 100ns
        for (int i=0; i<8; i++) begin
          hm_tail[(i*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+`WR_TAIL_OFFSET+i));
        end
      $display("[%t] : Readback the write tail word @ %h : %h \n", $realtime, pcim_addr+`WR_TAIL_OFFSET, hm_tail);
      tail_change = (hm_tail!=hm_tail_old);
      hm_tail_old = hm_tail;
    end while(tail_change && (timeout_count < 50));

    if ((hm_tail != pcim_addr + `WR_BUFF_SIZE) && (timeout_count == 50))
      $display("[%t] : *** ERROR *** Timeout to full the buffer.", $realtime);
    else
      $display("[%t] : PASS~~~ buffer is full.", $realtime);

    // check the write tail word does not overwirte elsewhere in buffer
    for (int i=0; i<8; i++) begin
      for (int j=0; j<8; j++) begin
        hm_tail[(j*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+`WR_TAIL_OFFSET+i*8+j));
      end
      $display("[%t] : Readback the write tail word @ %h : %h", $realtime, pcim_addr+`WR_TAIL_OFFSET+i*8, hm_tail);
    end
    $display("[%t] : Readback 64 Bytes Done!\n", $realtime);


    // put the bytes into host memory using backdoor function
    // tb.hm_put_byte and tb.hm_get_byte are backdoor functions to access memory on HBM.

    // for (int i=0; i<`WR_BUFF_SIZE/10; i++) begin
    //    for(int k=0; k<10; k++) begin
    //       tb.hm_put_byte(.addr(pcim_addr+10*i+k), .d(8'hFF & i));
    //    end
    //    hm_byte = 8'hFF & i;
    //    $display("set host @ memory %h ~ %h: %h", pcim_addr+10*i, pcim_addr+10*i+9, {10{hm_byte}});
    // end

    // check the data left in the memory buff
    for (int i=0; i<`WR_BUFF_SIZE/10; i++) begin
      for(int k=0; k<10; k++) begin
        hm_fsb_pkt[(k*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+10*i+k));
      end
      $display("read host @ memory %h ~ %h: %h", pcim_addr+10*i, pcim_addr+10*i+9, hm_fsb_pkt);
    end

    tb.power_down();

    $finish;
  end

endmodule // cl_m_axi4_fsb_tb
