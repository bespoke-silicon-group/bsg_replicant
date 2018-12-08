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


module cl_crossbar_tb();

  import tb_type_defines_pkg::*;

  // AXI ID
  parameter [5:0] AXI_ID = 6'h0;
  parameter CROSSBAR_M0 = 32'h0000_0000;
  parameter CROSSBAR_M1 = 32'h0000_1000;

  // parameter for axil read/write adapter
  parameter AXIL_WRITE = CROSSBAR_M0+32'h0000_0010;
  parameter AXIL_SEND  = CROSSBAR_M0+32'h0000_0014;

  parameter AXIL_READ  = CROSSBAR_M0+32'h0000_0020;
  parameter AXIL_FETCH = CROSSBAR_M0+32'h0000_0024;

  // parameter for FSB master write adapteer
  parameter CFG_REG    = CROSSBAR_M1+32'h00;
  parameter CNTL_START = CROSSBAR_M1+32'h08; // WR
  parameter CNTL_RESET = CROSSBAR_M1+32'h0c; // W

  parameter WR_ADDR_LOW  = CROSSBAR_M1+32'h20;
  parameter WR_ADDR_HIGH = CROSSBAR_M1+32'h24;
  parameter WR_READ_HEAD = CROSSBAR_M1+32'h28;
  parameter WR_LEN       = CROSSBAR_M1+32'h2c;
  parameter WR_OFFSET    = CROSSBAR_M1+32'h30;

  parameter RD_ADDR_LOW  = CROSSBAR_M1+32'h40;
  parameter RD_ADDR_HIGH = CROSSBAR_M1+32'h44;
  parameter RD_DATA      = CROSSBAR_M1+32'h48;
  parameter RD_LEN       = CROSSBAR_M1+32'h4c;

  parameter WR_DST_SEL = CROSSBAR_M1+32'he0;

  parameter WR_SEL = CROSSBAR_M1+32'h01;
  parameter ED_SEL = CROSSBAR_M1+32'h02;

  parameter WR_BUFF_SIZE   = 32'h280; // 640 Bytes
  parameter WR_TAIL_OFFSET = 64'h280;

  localparam num_pkt_lp = WR_BUFF_SIZE/64; // number of axi pkts in buffer


  logic [ 31:0] rdata_byte_num;
  logic [ 31:0] rdata_word1   ;
  logic [ 31:0] rdata_word2   ;
  logic [ 31:0] rdata_word3   ;
  logic [ 31:0] rdata_word4   ;
  logic [128:0] rdata_readback;


// axi4 write
  logic [63:0] pcim_addr;
  logic [31:0] pcim_data;

  logic [31:0] cfg_rdata;

  logic [num_pkt_lp*8-1:0] [127:0] hm_fsb_pkts;// store 2*num_pkt_lp x 4*16 Byts for 2 reads
  logic [127:0] hm_fsb_pkt    ;
  logic [ 63:0] hm_tail_old   ;
  logic [ 63:0] hm_tail       ;
  logic [  7:0] hm_byte       ;
  bit           tail_change   ;
  bit           fsb_valid_mask;

  int timeout_count;
  int read_cycle_num = 2;

  initial begin

    tb.power_up();

    // ----------------------------------------------------------------------
    // $display ("AXI-L to FSB slave test:");
    tb.nsec_delay(2000);
    // ----------------------------------------------------------------------

    // initialize, using poke() for fun ";)
    // Note: address bit range: only s_axi_awaddr(5:2) and s_axi_araddr(5:2) are decoded
    tb.poke(.addr(CROSSBAR_M0+32'h0000_0000), .data(32'hFFFFFFFF), // Transmit Data FIFO Reset (TDFR)
            .id(AXI_ID), .size(DataSize::UINT16), .intf(AxiPort::PORT_OCL));
    tb.poke(.addr(CROSSBAR_M0+32'h0000_0004), .data(32'hFFFFFFFF), // Interrupt Enable Register (IER)
            .id(AXI_ID), .size(DataSize::UINT16), .intf(AxiPort::PORT_OCL));

    // write
    for (int i=0; i<4; i++) begin
      tb.poke_ocl(.addr(AXIL_WRITE), .data(32'h1 + 4*i));
      tb.poke_ocl(.addr(AXIL_WRITE), .data(32'h2 + 4*i));
      tb.poke_ocl(.addr(AXIL_WRITE), .data(32'h3 + 4*i));
      tb.poke_ocl(.addr(AXIL_WRITE), .data(32'h4 + 4*i));
      tb.poke_ocl(.addr(AXIL_SEND), .data(32'h00000010));
      // 128 bits are then truncated to 80-bits fsb packet
    end

    // read
    for (int i=0; i<4; i++) begin
      tb.peek_ocl(.addr(AXIL_FETCH), .data(rdata_byte_num));
      if (rdata_byte_num == 32'h00000010) begin
        $display ("READBACK LEN IS 16 BYTES, PASSED~");
      end else begin
        $display ("READBACK LEN FAILED!");
      end
      tb.peek_ocl(.addr(AXIL_READ), .data(rdata_word1));
      tb.peek_ocl(.addr(AXIL_READ), .data(rdata_word2));
      tb.peek_ocl(.addr(AXIL_READ), .data(rdata_word3));
      tb.peek_ocl(.addr(AXIL_READ), .data(rdata_word4));
      $display ("FSB READBACK DATA %h %h %h %h", rdata_word1, rdata_word2, rdata_word3, rdata_word4);
      if (rdata_word1 == (32'h00000001+4*i) && rdata_word2 == (32'h00000002+4*i)
        && rdata_word3 == ((((32'h3+4*i)&32'h0000000F)<<12) 
                        + (((32'h3+4*i)>>12)&32'h0000000F) 
                        + ((32'h3 + 4*i)&32'h00000FF0))
        && rdata_word4 == 32'h00000000) begin
        $display ("FSB READBACK DATA PASSED~");
      end else begin
        $display ("FSB READBACK DATA FAILED!");
      end
    end


    // ----------------------------------------------------------------------
    $display ("FSB to AXI-4 write test:");
    // ----------------------------------------------------------------------
    
    // base configure bus address: 0x0000_0000, 20 bit address
    pcim_addr = 64'h0000_0000_1234_0000;
    pcim_data = 32'h6c93_af50;

    tb.nsec_delay(2000);
    tb.poke_stat(.addr(CROSSBAR_M1+8'h0c), .ddr_idx(0), .data(32'h0000_0000));
    tb.poke_stat(.addr(CROSSBAR_M1+8'h0c), .ddr_idx(1), .data(32'h0000_0000));
    tb.poke_stat(.addr(CROSSBAR_M1+8'h0c), .ddr_idx(2), .data(32'h0000_0000));


    $display("[%t] : Configuring cl_tst registers for DMA write", $realtime);
    tb.poke_ocl(.addr(CFG_REG), .data(32'h0000_0010));          // set mask to enable fsb_wvalid
    tb.poke_ocl(.addr(WR_ADDR_LOW), .data(pcim_addr[31:0]));    // write address low
    tb.poke_ocl(.addr(WR_ADDR_HIGH), .data(pcim_addr[63:32]));  // write address high
    tb.poke_ocl(.addr(WR_LEN), .data(32'h0000_0000));           // write 64 bytes, 512bits
    tb.poke_ocl(.addr(WR_OFFSET), .data(WR_BUFF_SIZE-1));       // 320 bytes, 32 fsb pkts
    // Start write
    tb.poke_ocl(.addr(CNTL_START), .data(WR_SEL));


    // test fsb wvalid pauses
    // --------------------------------------------
    fsb_valid_mask = 0;
    for (int i=0; i<3; i++) begin
      fsb_valid_mask = ~fsb_valid_mask;
      tb.poke_ocl(.addr(CFG_REG), .data(32'h0000_0000 | (fsb_valid_mask << 4)));
      # 100ns;
    end


    // wait until not writing, buffer is full
    // --------------------------------------------
    timeout_count = 0;
    do begin
      # 10ns;
      // check if is still writting
      tb.peek_ocl(.addr(CNTL_START), .data(cfg_rdata));
      $display("[%t] : Waiting for 1st write activity to complete", $realtime);
      timeout_count ++;
    end while((cfg_rdata[0] !== 1'b0) && timeout_count < 10);

    if ((cfg_rdata[0] !== 1'b0) && (timeout_count == 10))
      $display("[%t] : *** ERROR *** Timeout waiting for 1st writes to complete.", $realtime);
    else
      $display("[%t] : PASS ~~~ axi4 1st writes complete.", $realtime);


    // check if the axi-4 bus is free of error
    // --------------------------------------------
    tb.peek_ocl(.addr(CNTL_START), .data(cfg_rdata));
    if (cfg_rdata[3])
      $display("[%t] : *** ERROR *** axi4 1st writes RESP is NOT OKAY.", $realtime);


    // read x2, update the head, so more data from fsb should come
    // --------------------------------------------
    // Note: only test in steps of 64 bytes for simplity...

    for (int n=0; n<read_cycle_num; n++) begin
      for (int i=0; i<num_pkt_lp; i++)
        begin
          for (int j=0; j<8; j++) begin
            hm_tail[(j*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+WR_TAIL_OFFSET+j)); // check the write tail
          end
          $display("[%t] : Readback the write tail word @ %h : %h", $realtime, pcim_addr+WR_TAIL_OFFSET, hm_tail[31:0]);
          # 100ns // Delay enough time to ensure that we have data in buffer to read
            if (hm_tail[31:0]!=(i*32'h40)) begin  // check current read head is not equal to tail
              for (int j=0; j<4; j++) begin
                for (int k=0; k<16; k++) begin
                  hm_fsb_pkts[4*num_pkt_lp*n+4*i+j][(k*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+64*i+16*j+k));
                end
              end
              tb.poke_ocl(.addr(WR_READ_HEAD), .data(32'h0000_0000 + i*32'h40));
            end
        end
      end


    // check tail is not updating any more
    // --------------------------------------------
    timeout_count = 0;
    hm_tail_old = 0;
    do begin
      # 100ns
        for (int i=0; i<8; i++) begin
          hm_tail[(i*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+WR_TAIL_OFFSET+i));
        end
      $display("[%t] : tail word @ %h : %h \n", $realtime, pcim_addr+WR_TAIL_OFFSET, hm_tail);
      tail_change = (hm_tail!=hm_tail_old);
      hm_tail_old = hm_tail;
    end while(tail_change && (timeout_count < 50));

    if ((hm_tail[31:0] == (pcim_addr[15] + WR_BUFF_SIZE - 32'h80)) && (timeout_count != 50))
      $display("[%t] : PASS~~~ buffer is full.", $realtime);
    else
      $display("[%t] : *** ERROR *** Timeout to fill the buffer.", $realtime);

    // check the write tail word (it should not overwirte elsewhere in buffer)
    for (int i=0; i<8; i++) begin
      for (int j=0; j<8; j++) begin
        hm_tail[(j*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+WR_TAIL_OFFSET+i*8+j));
      end
      $display("[%t] : Readback the write tail word @ %h : %h", $realtime, pcim_addr+WR_TAIL_OFFSET+i*8, hm_tail);
    end
    $display("[%t] : Readback 64 Bytes Done!\n", $realtime);

 
    // bfm backdoor check
    // --------------------------------------------
    // 1. print all read data in sequence
    for (int i=0; i<num_pkt_lp*4*read_cycle_num; i++) begin
      $display("read data @No. %h: %h", i, hm_fsb_pkts[i]);
    end
    // 2. print data left in memory buff
    for (int i=0; i<WR_BUFF_SIZE/16; i++) begin
      for(int k=0; k<16; k++) begin
        hm_fsb_pkt[(k*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+16*i+k));
      end
      $display("host memory @%h +:16: %h", pcim_addr+16*i, hm_fsb_pkt);
    end

    tb.kernel_reset();
    tb.power_down();

    $finish;
  end

endmodule
