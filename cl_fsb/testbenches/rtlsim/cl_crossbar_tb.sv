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
  parameter CROSSBAR_M2 = 32'h0000_2000;
  parameter CROSSBAR_M3 = 32'h0000_3000;

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
  localparam num_pkt_lp = WR_BUFF_SIZE/64; // number of axi pkts in buffer

  // parameter for axi4 read/write adapter
  parameter AXI4_WRITE = 32'h8000_1000;
  parameter AXI4_SEND  = CROSSBAR_M2+32'h0000_0014;

  parameter AXI4_READ  = 32'h8000_2000;;
  parameter AXI4_FETCH = CROSSBAR_M2+32'h0000_0024;



  logic [ 31:0] rdata_byte_num;
  logic [ 31:0] rdata_word1   ;
  logic [ 31:0] rdata_word2   ;
  logic [ 31:0] rdata_word3   ;
  logic [ 31:0] rdata_word4   ;
  logic [128:0] rdata_readback;


  // axi4 pcim write
  logic [63:0] pcim_addr;

  logic [31:0] cfg_rdata;

  logic [num_pkt_lp*8-1:0] [127:0] hm_fsb_pkts;// store 2*num_pkt_lp x 4*16 Byts for 2 reads
  logic [127:0] hm_fsb_pkt    ;
  logic [ 63:0] hm_tail_old   ;
  logic [ 63:0] hm_tail       ;
  logic [  7:0] hm_byte       ;
  bit           tail_change   ;
  bit           fsb_valid_mask;


  // axi4 pcis write/read
  logic [63:0]  pcis_addr;
  logic [511:0] pcis_wr_data;
  logic [511:0] pcis_rd_data;
  bit   [511:0] pcis_exp_data;
  bit   [511:0] pcis_act_data;


  int read_cycle_num = 2;

  int pcis_data=0;
  int timeout_count;
  int error_count;

  initial begin

    tb.power_up();

    // ----------------------------------------------------------------------
    $display ("No.1 ===> FSB to AXI-4 write test:");
    // ----------------------------------------------------------------------
    
    // base configure bus address: 0x0000_0000, 20 bit address
    pcim_addr = 64'h0000_0000_1234_0000;
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


    // concurrent test:
    // ----------------------------------------------------------------------
    $display ("No.2 ===> AXI-L to FSB slave test:");
    // ----------------------------------------------------------------------
    // tb.nsec_delay(2000);
    // initialize, using poke() for fun ";)
    // Note: address bit range: only s_axi_awaddr(5:2) and s_axi_araddr(5:2) are decoded
    tb.poke(.addr(CROSSBAR_M0+32'h0000_0000), .data(32'hFFFFFFFF), // Clear Interrupt Status Register (ISR)
            .id(AXI_ID), .size(DataSize::UINT16), .intf(AxiPort::PORT_OCL));
    tb.poke(.addr(CROSSBAR_M0+32'h0000_0004), .data(32'hFFFFFFFF), // Set Interrupt Enable Register (IER)
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


    // read in steps of 64 bytes, 2 times from 0x00 to offset
    // --------------------------------------------
    for (int n=0; n<read_cycle_num; n++) begin
      for (int i=0; i<num_pkt_lp; i++)
        begin
          for (int j=0; j<8; j++) begin
            hm_tail[(j*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+WR_BUFF_SIZE+j)); // check the write tail
          end
          $display("[%t] : Readback the write tail word @ %h : %h", $realtime, pcim_addr+WR_BUFF_SIZE, hm_tail[31:0]);
          # 100ns // Delay enough time to ensure that we have data in buffer to read
            if (hm_tail[31:0]!=(i*32'h40)) begin  // check current read head is not equal to tail
              for (int j=0; j<4; j++) begin
                for (int k=0; k<16; k++) begin
                  hm_fsb_pkts[4*num_pkt_lp*n+4*i+j][(k*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+64*i+16*j+k));
                end
              end
              tb.poke_ocl(.addr(WR_READ_HEAD), .data(32'h0000_0000 + i*32'h40));  // update the head
            end
        end
      end


    // read greedily, do not record the read data
    // --------------------------------------------
    # 500ns
    for (int j=0; j<8; j++) begin
      hm_tail[(j*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+WR_BUFF_SIZE+j)); // check the write tail
    end
    $display("[%t] : Tail before gready read : %h", $realtime, hm_tail[31:0]);

    if (hm_tail[31:0] == 32'h0) begin
      tb.poke_ocl(.addr(WR_READ_HEAD), .data(WR_BUFF_SIZE - 32'h40));  // update the head
    end
    else begin
      tb.poke_ocl(.addr(WR_READ_HEAD), .data(hm_tail[31:0] - 32'h40));  // update the head
    end

    for (int j=0; j<8; j++) begin
      hm_tail[(j*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+WR_BUFF_SIZE+j)); // check the write tail
    end
    $display("[%t] : Tail after gready read : %h", $realtime, hm_tail[31:0]);


    // check tail is not updating any more
    // --------------------------------------------
    timeout_count = 0;
    hm_tail_old = 0;
    do begin
      # 100ns
        for (int i=0; i<8; i++) begin
          hm_tail[(i*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+WR_BUFF_SIZE+i));
        end
      $display("[%t] : tail word : %h ", $realtime, hm_tail);
      tail_change = (hm_tail!=hm_tail_old);
      hm_tail_old = hm_tail;
    end while(tail_change && (timeout_count < 50));

    if (timeout_count != 50)
      $display("[%t] : PASS~~~ buffer is full at %h.", $realtime, hm_tail);
    else
      $display("[%t] : *** ERROR *** Timeout to fill the buffer.", $realtime);

    // check the write tail word (it should not overwirte elsewhere in buffer)
    for (int i=0; i<8; i++) begin
      for (int j=0; j<8; j++) begin
        hm_tail[(j*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+WR_BUFF_SIZE+i*8+j));
      end
      $display("[%t] : Readback the write tail word @ %h : %h", $realtime, pcim_addr+WR_BUFF_SIZE+i*8, hm_tail);
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



    // ----------------------------------------------------------------------
    $display ("N0.3 ===> AXI4 to FSB slave test:");
    // ----------------------------------------------------------------------
    // axi4 read/write test:
    // tb.nsec_delay(2000);
    // initialize, using poke() for fun ";)
    // Note: address bit range: only s_axi_awaddr(5:2) and s_axi_araddr(5:2) are decoded
    tb.poke(.addr(CROSSBAR_M2+32'h0000_0000), .data(32'hFFFFFFFF), // Transmit Data FIFO Reset (TDFR)
            .id(AXI_ID), .size(DataSize::UINT16), .intf(AxiPort::PORT_OCL));
    tb.poke(.addr(CROSSBAR_M2+32'h0000_0004), .data(32'hFFFFFFFF), // Interrupt Enable Register (IER)
            .id(AXI_ID), .size(DataSize::UINT16), .intf(AxiPort::PORT_OCL));
    $display("[%t] : RESET axi_fifo_mm_s.", $realtime);

    // axi4 write 64 bytes
    for(int i=0; i<4; i++) begin
      for(int addr=0; addr<64; addr=addr+64) begin
        for (int num_bytes=0; num_bytes<64; num_bytes++) begin  // prepare data
              pcis_wr_data[(num_bytes*8)+:8] = pcis_data;
              pcis_data++;
        end

        // TODO: check the interrupt register.
        // tb.peek_ocl(.addr(CROSSBAR_M2+32'h0000_0000), .data(cfg_rdata));
        // compare_data(.act_data(cfg_rdata), .exp_data(32'h00000000)); // Assert no interrupt

        tb.poke(.addr(AXI4_WRITE), .data(pcis_wr_data), .size(DataSize::DATA_SIZE'(6)));
        tb.poke_ocl(.addr(AXI4_SEND), .data(32'h040));  // write 64 bytes
        $display("[%t] : axi4 send data = %0h", $realtime, pcis_wr_data);

        tb.poke_ocl(.addr(AXI4_FETCH), .data(32'h040)); // readback 64 bytes
        tb.peek(.addr(AXI4_READ), .data(pcis_rd_data), .size(DataSize::DATA_SIZE'(6)));
        $display("[%t] : axi4 readback data = %0h", $realtime, pcis_rd_data);
        for (int num_bytes=0; num_bytes<64; num_bytes++) begin           
             pcis_exp_data[((addr+num_bytes)*8)+:8] = pcis_wr_data[(num_bytes*8)+:8];
             pcis_act_data[((addr+num_bytes)*8)+:8] = pcis_rd_data[(num_bytes*8)+:8];
        end
      end
      compare_data(.act_data(pcis_act_data), .exp_data(pcis_exp_data));
    end
    // ----------------------------------------------------------------------

    tb.kernel_reset();
    tb.power_down();

    $finish;
  end


  task compare_data(logic [511:0] act_data, exp_data);
    if(act_data !== exp_data) begin
       $display($time,,,"***ERROR*** : Data Mismatch. Actual Data:%0h <==> Expected Data: %0h",
                          act_data, exp_data);
       error_count ++;
    end
    else begin
       $display("      PASSED! : Data Matched. Actual Data:%0h <==> Expected Data: %0h", act_data, exp_data);
    end
  endtask

  task disp_err (input string s);
    $display($time,,,"***ERROR*** : %s", s);
    error_count ++;
  endtask

endmodule
