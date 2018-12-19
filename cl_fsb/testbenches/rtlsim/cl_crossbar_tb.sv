/**
 *  cl_crossbar_tb.sv
 *
 *  testbench for crossbar functionality
 */

module cl_crossbar_tb();

  import tb_type_defines_pkg::*;

  // AXI ID
  parameter [5:0] AXI_ID = 6'h0;
  parameter CROSSBAR_M0  = 32'h0000_0000;
  parameter CROSSBAR_M1  = 32'h0000_1000;
  parameter CROSSBAR_M2  = 32'h0000_2000;
  parameter CROSSBAR_M3  = 32'h0000_3000;

  // parameter for FSB master write adapteer
  parameter CFG_REG      = 32'h00;
  parameter CNTL_REG     = 32'h08; // WR
  parameter RESET_REG    = 32'h0c; // W

  parameter WR_ADDR_LOW  = 32'h20;
  parameter WR_ADDR_HIGH = 32'h24;
  parameter WR_READ_HEAD = 32'h28;
  parameter WR_LEN       = 32'h2c;
  parameter WR_BUF_SIZE  = 32'h30;

  parameter WR_START = 32'h01;
  parameter WR_STOP =  32'h00;

  // parameter for axil read/write adapter
  parameter IST_REG    = 32'h0000_0000;
  parameter IER_REG    = 32'h0000_0004;

  parameter AXIL_SEND  = 32'h0000_0014;
  parameter AXIL_FETCH = 32'h0000_0024;

  parameter AXIL_WRITE = 32'h0000_0010;
  parameter AXIL_READ  = 32'h0000_0020;

  // parameter for axi4 read/write adapter
  parameter AXI4_SEND  = 32'h0000_0014;
  parameter AXI4_FETCH = 32'h0000_0024;

  parameter AXI4_WRITE = 32'h8000_1000;
  parameter AXI4_READ  = 32'h8000_2000;


  parameter BUFF1_SIZE = 32'd640;
  parameter BUFF2_SIZE = 32'd1024;

  parameter READ_CYCLES = 2;

  int timeout_count;
  int error_count = 0;

  initial begin

    tb.power_up();

    tb.nsec_delay(2000);
    poke_ddr_stat();
    
    $display ("No.1A ===> FSB to AXI-4 write test:");
    pcim_DMA_write_buffer(.pcim_addr(64'h0000_0000_0000_0000), 
      .BURST_LEN(0),
      .WR_BUFF_SIZE(BUFF1_SIZE),
      .CFG_BASE_ADDR(CROSSBAR_M1));

    $display ("No.1B ===> FSB to AXI-4 write test:");
    pcim_DMA_write_buffer(.pcim_addr(64'h0000_0000_1000_0000), 
      .BURST_LEN(1),
      .WR_BUFF_SIZE(BUFF2_SIZE),
      .CFG_BASE_ADDR(CROSSBAR_M1+12'h500));

    pcim_reads_buffer1(.pcim_addr(64'h0000_0000_0000_0000), .CFG_BASE_ADDR(CROSSBAR_M1));
    pcim_reads_buffer2(.pcim_addr(64'h0000_0000_1000_0000), .CFG_BASE_ADDR(CROSSBAR_M1+12'h500));

    $display ("No.2 (concurrent test) ===> AXI-L to FSB slave test:");
    ocl_FSB_poke_peek_test(.CFG_BASE_ADDR(CROSSBAR_M0));

    $display ("N0.3 ===> AXI4 to FSB slave test:");
    pcis_FSB_poke_peek_test(.CFG_BASE_ADDR(CROSSBAR_M2));

    tb.kernel_reset();
    tb.power_down();

    $finish;

  end


  task compare_data(logic [511:0] act_data, exp_data);
    if(act_data !== exp_data) begin
        $display($time,,,"***ERROR*** : Data Mismatch!!!");
        $display("Expected Data: %0h", exp_data);
        $display("Actual   Data: %0h", act_data);
       error_count ++;
    end
    else begin
        $display("~~~PASS~~~ : Data is Matched.");
        $display("Expected Data: %0h", exp_data);
        $display("Actual   Data: %0h", act_data);
    end
  endtask


  task disp_err(input string s);
    $display($time,,,"***ERROR*** : %s", s);
    error_count ++;
  endtask


  task poke_ddr_stat();
    $display("[%t] : Start poking ddr stats", $realtime);
    tb.poke_stat(.addr(8'h0c), .ddr_idx(0), .data(32'h0000_0000));
    tb.poke_stat(.addr(8'h0c), .ddr_idx(1), .data(32'h0000_0000));
    tb.poke_stat(.addr(8'h0c), .ddr_idx(2), .data(32'h0000_0000));
  endtask


  task DMA_pcim_config(logic [63:0] start_addr, logic [31:0] burst_len, buffer_size, CFG_BASE_ADDR);
    $display("[%t] : Configuring adapter for DMA write", $realtime);
    tb.poke_ocl(.addr(CFG_BASE_ADDR+WR_ADDR_LOW), .data(start_addr[31:0]));   // write address low
    tb.poke_ocl(.addr(CFG_BASE_ADDR+WR_ADDR_HIGH), .data(start_addr[63:32])); // write address high
    tb.poke_ocl(.addr(CFG_BASE_ADDR+WR_LEN), .data(burst_len));               // write burst size, 0=512bits
    tb.poke_ocl(.addr(CFG_BASE_ADDR+WR_BUF_SIZE), .data(buffer_size));        // buffer size, tail offset
  endtask


  task DMA_transfer_start(logic [31:0] adapter_wr_start, CFG_BASE_ADDR);
    tb.poke_ocl(.addr(CFG_BASE_ADDR+CNTL_REG), .data(adapter_wr_start));      // Start write
  endtask


  task DMA_master_wvalid_mask(logic [31:0] fsb_wvalid_mask, CFG_BASE_ADDR);
    tb.poke_ocl(.addr(CFG_BASE_ADDR+CFG_REG), .data(fsb_wvalid_mask));        // set mask to enable fsb_wvalid
  endtask


  task DMA_write_stat_check(int timeout_count, logic [31:0] CFG_BASE_ADDR);
    int cnt = 0;
    logic [31:0] cfg_rdata;
    do begin
      # 10ns;
      // check if is still writting
      tb.peek_ocl(.addr(CFG_BASE_ADDR+CNTL_REG), .data(cfg_rdata));
      $display("[%t] : Waiting for 1st write activity to complete", $realtime);
      cnt ++;
    end while((cfg_rdata[0] !== 1'b0) && cnt < timeout_count);

    if ((cfg_rdata[0] !== 1'b0) && (cnt == timeout_count))
      $display("[%t] : *** ERROR *** Timeout waiting for 1st writes to complete.", $realtime);
    else
      $display("[%t] : PASS ~~~ axi4 1st writes complete.", $realtime);
  endtask


  task DMA_adapter_error_check(logic [31:0] CFG_BASE_ADDR);
    logic [31:0] cfg_rdata;
    tb.peek_ocl(.addr(CFG_BASE_ADDR+CNTL_REG), .data(cfg_rdata));
    if (cfg_rdata[3])
      $display("[%t] : *** ERROR *** axi4 1st writes RESP is NOT OKAY.", $realtime);
    else
      $display("[%t] : ~~~ OKAY ~~~ axi4 1st writes RESP[1] is 0.", $realtime);
  endtask


  task DMA_buffer_full_check(logic [63:0] buffer_address, int timeout_count);
    int cnt = 0;
    int tail_change;
    logic [63:0] hm_tail_old = 0;
    logic [63:0] hm_tail;
    do begin
      # 300ns
        for (int i=0; i<8; i++) begin
          hm_tail[(i*8)+:8] = tb.hm_get_byte(.addr(buffer_address+i));
        end
      $display("[%t] : tail word : %h ", $realtime, hm_tail);
      tail_change = (hm_tail!=hm_tail_old);
      hm_tail_old = hm_tail;
      cnt++;
    end while(tail_change && (cnt < timeout_count));

    if (cnt != timeout_count)
      $display("[%t] : PASS~~~ buffer is full at %h.", $realtime, hm_tail);
    else
      $display("[%t] : *** ERROR *** Timeout to fill the buffer.", $realtime);

    // check the write tail word chunk, it should not overwirte elsewhere in buffer
    for (int i=0; i<8; i++) begin
      for (int j=0; j<8; j++) begin
        hm_tail[(j*8)+:8] = tb.hm_get_byte(.addr(buffer_address+i*8+j));
      end
      $display("[%t] : Readback the write tail word @ %h : %h", $realtime, buffer_address+i*8, hm_tail);
    end
    $display("[%t] : Readback 64 Bytes Done!\n", $realtime);
  endtask


  task ocl_FSB_poke_peek_test(logic [31:0] CFG_BASE_ADDR);
    logic [31:0] rdata_num_B;
    logic [31:0] rdata_word1;
    logic [31:0] rdata_word2;
    logic [31:0] rdata_word3;
    logic [31:0] rdata_word4;

    // initialize, using poke() for fun ";)
    // Note: address bit range: only s_axi_awaddr(5:2) and s_axi_araddr(5:2) are decoded
    tb.poke(.addr(CFG_BASE_ADDR+IST_REG), .data(32'hFFFFFFFF), // Clear Interrupt Status Register (ISR)
            .id(AXI_ID), .size(DataSize::UINT16), .intf(AxiPort::PORT_OCL));
    tb.poke(.addr(CFG_BASE_ADDR+IER_REG), .data(32'hFFFFFFFF), // Set Interrupt Enable Register (IER)
            .id(AXI_ID), .size(DataSize::UINT16), .intf(AxiPort::PORT_OCL));

    // write
    for (int i=0; i<4; i++) begin
      tb.poke_ocl(.addr(CFG_BASE_ADDR+AXIL_WRITE), .data(32'h1 + 4*i));
      tb.poke_ocl(.addr(CFG_BASE_ADDR+AXIL_WRITE), .data(32'h2 + 4*i));
      tb.poke_ocl(.addr(CFG_BASE_ADDR+AXIL_WRITE), .data(32'h3 + 4*i));
      tb.poke_ocl(.addr(CFG_BASE_ADDR+AXIL_WRITE), .data(32'h4 + 4*i));
      tb.poke_ocl(.addr(CFG_BASE_ADDR+AXIL_SEND), .data(32'h00000010));
      // 128 bits are then truncated to 80-bits fsb packet
    end

    // read
    for (int i=0; i<4; i++) begin
      tb.peek_ocl(.addr(AXIL_FETCH), .data(rdata_num_B));
      if (rdata_num_B == 32'h00000010) begin
        $display ("READBACK LEN IS 16 BYTES, PASSED~");
      end else begin
        $display ("READBACK LEN FAILED!");
      end
      tb.peek_ocl(.addr(CFG_BASE_ADDR+AXIL_READ), .data(rdata_word1));
      tb.peek_ocl(.addr(CFG_BASE_ADDR+AXIL_READ), .data(rdata_word2));
      tb.peek_ocl(.addr(CFG_BASE_ADDR+AXIL_READ), .data(rdata_word3));
      tb.peek_ocl(.addr(CFG_BASE_ADDR+AXIL_READ), .data(rdata_word4));
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
  endtask


  task pcis_FSB_poke_peek_test(logic [31:0] CFG_BASE_ADDR);
    // axi4 pcis write/read
    logic [511:0] pcis_wr_data;
    logic [511:0] pcis_rd_data;
    bit   [511:0] pcis_exp_data;
    bit   [511:0] pcis_act_data;
    int pcis_data = 0;
    tb.poke_ocl(.addr(CFG_BASE_ADDR+IST_REG), .data(32'hFFFFFFFF));
    tb.poke_ocl(.addr(CFG_BASE_ADDR+IER_REG), .data(32'hFFFFFFFF));
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
        tb.poke_ocl(.addr(CFG_BASE_ADDR+AXI4_SEND), .data(32'h040));  // write 64 bytes
        $display("[%t] : axi4 send data = %0h", $realtime, pcis_wr_data);

        tb.poke_ocl(.addr(CFG_BASE_ADDR+AXI4_FETCH), .data(32'h040)); // readback 64 bytes
        tb.peek(.addr(AXI4_READ), .data(pcis_rd_data), .size(DataSize::DATA_SIZE'(6)));
        $display("[%t] : axi4 readback data = %0h", $realtime, pcis_rd_data);
        for (int num_bytes=0; num_bytes<64; num_bytes++) begin           
             pcis_exp_data[((addr+num_bytes)*8)+:8] = pcis_wr_data[(num_bytes*8)+:8];
             pcis_act_data[((addr+num_bytes)*8)+:8] = pcis_rd_data[(num_bytes*8)+:8];
        end
      end
      compare_data(.act_data(pcis_act_data), .exp_data(pcis_exp_data));
    end
  endtask


  task pcim_DMA_write_buffer (logic [63:0] pcim_addr, logic [31:0] BURST_LEN, WR_BUFF_SIZE, CFG_BASE_ADDR);

    logic [ 63:0] hm_tail;
    logic [127:0] hm_fsb_pkt;

    DMA_pcim_config(.start_addr(pcim_addr), .burst_len(BURST_LEN), .buffer_size(WR_BUFF_SIZE), .CFG_BASE_ADDR(CFG_BASE_ADDR));

    DMA_master_wvalid_mask(.fsb_wvalid_mask(32'h0000_0010), .CFG_BASE_ADDR(CFG_BASE_ADDR));

    DMA_transfer_start(.adapter_wr_start(WR_START), .CFG_BASE_ADDR(CFG_BASE_ADDR));
    
    // // test fsb wvalid pauses
    // // --------------------------------------------
    // DMA_master_wvalid_mask(.fsb_wvalid_mask(32'h0000_0000), .CFG_BASE_ADDR(CFG_BASE_ADDR));
    // # 100ns;
    // DMA_master_wvalid_mask(.fsb_wvalid_mask(32'h0000_0010), .CFG_BASE_ADDR(CFG_BASE_ADDR));
    // # 100ns;
    // DMA_master_wvalid_mask(.fsb_wvalid_mask(32'h0000_0000), .CFG_BASE_ADDR(CFG_BASE_ADDR));
    // # 100ns;
    // DMA_master_wvalid_mask(.fsb_wvalid_mask(32'h0000_0010), .CFG_BASE_ADDR(CFG_BASE_ADDR));
    // // --------------------------------------------


    DMA_write_stat_check(.timeout_count(10), .CFG_BASE_ADDR(CFG_BASE_ADDR));
    DMA_adapter_error_check(.CFG_BASE_ADDR(CFG_BASE_ADDR));

    DMA_buffer_full_check(.buffer_address(pcim_addr+WR_BUFF_SIZE), .timeout_count(50));

  endtask


  task pcim_reads_buffer1(logic [63:0] pcim_addr, logic [31:0] CFG_BASE_ADDR);

    logic [READ_CYCLES*BUFF1_SIZE/16-1:0] [127:0] hm_fsb_pkts;
    logic [ 63:0] hm_tail;
    logic [127:0] hm_fsb_pkt;
    for (int n=0; n<READ_CYCLES; n++) begin
      for (int i=0; i<BUFF1_SIZE/64; i++)  // read in steps of 64 bytes
        begin
          for (int j=0; j<8; j++) begin
            hm_tail[(j*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+BUFF1_SIZE+j)); // check the write tail
          end
          $display("[%t] : Readback the write tail word @ %h : %h", $realtime, pcim_addr+BUFF1_SIZE, hm_tail[31:0]);
          # 100ns // Delay enough time to ensure that we have data in buffer to read
            if (hm_tail[31:0]!=(i*32'h40)) begin  // check current read head is not equal to tail
              for (int j=0; j<4; j++) begin
                for (int k=0; k<16; k++) begin
                  hm_fsb_pkts[BUFF1_SIZE/16*n+4*i+j][(k*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+64*i+16*j+k));
                end
              end
              tb.poke_ocl(.addr(CFG_BASE_ADDR+WR_READ_HEAD), .data(32'h0000_0000 + i*32'h40));  // update the head
            end
        end
      end

    // bfm backdoor check
    // --------------------------------------------
    // 1. print all read data in sequence
    for (int i=0; i<BUFF1_SIZE/16*READ_CYCLES; i++) begin
      $display("read data @No. %d: %h", i, hm_fsb_pkts[i]);
    end
    // 2. print data left in memory buff
    for (int i=0; i<BUFF1_SIZE/16; i++) begin
      for(int k=0; k<16; k++) begin
        hm_fsb_pkt[(k*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+16*i+k));
      end
      $display("host memory @%d +:16: %h", pcim_addr+16*i, hm_fsb_pkt);
    end
  endtask


  task pcim_reads_buffer2(logic [63:0] pcim_addr, logic [31:0] CFG_BASE_ADDR);

    logic [READ_CYCLES*BUFF2_SIZE/64-1:0] [511:0] hm_fsb_pkts;
    logic [ 63:0] hm_tail;
    logic [511:0] hm_fsb_pkt;
    for (int n=0; n<READ_CYCLES; n++) begin
      for (int i=0; i<BUFF2_SIZE/64; i++)  // read in steps of 64 bytes
        begin
          for (int j=0; j<8; j++) begin
            hm_tail[(j*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+BUFF2_SIZE+j)); // check the write tail
          end
          $display("[%t] : Readback the write tail word @ %h : %h", $realtime, pcim_addr+BUFF2_SIZE, hm_tail[31:0]);
          # 200ns // Delay enough time to ensure that we have data in buffer to read
            if (hm_tail[31:0]!=(i*32'd64)) begin  // check current read head is not equal to tail
              for (int j=0; j<64; j++) begin
                  hm_fsb_pkts[BUFF2_SIZE/64*n+i][(j*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+64*i+j));
              end
              tb.poke_ocl(.addr(CFG_BASE_ADDR+WR_READ_HEAD), .data(32'h0000_0000 + i*32'd64));  // update the head
            end
        end
      end

    // bfm backdoor check
    // --------------------------------------------
    // 1. print all read data in sequence
    for (int i=0; i<BUFF2_SIZE/64*READ_CYCLES; i++) begin
      $display("read data @No. %d: %h", i, hm_fsb_pkts[i]);
    end
    // 2. print data left in memory buff
    for (int i=0; i<BUFF2_SIZE/64; i++) begin
      for(int k=0; k<64; k++) begin
        hm_fsb_pkt[(k*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+64*i+k));
      end
      $display("host memory @%d +:64: %h", pcim_addr+64*i, hm_fsb_pkt);
    end
  endtask

endmodule
