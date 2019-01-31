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

  parameter WR_PHASE_NUM = 32'h60;

  parameter WR_START = 32'h01;
  parameter WR_STOP =  32'h00;
  parameter WR_RESET = 32'h01;

  // parameter for axil read/write adapter
  parameter ISR_REG = 32'h0000_0000;
  parameter IER_REG = 32'h0000_0004;

  parameter TDFV_REG = 32'h0000_000C;
  parameter RDFO_REG = 32'h0000_001C;

  parameter AXIL_TLR = 32'h0000_0014;
  parameter AXIL_RLR = 32'h0000_0024;

  parameter TDFD_REG = 32'h0000_0010;
  parameter RDFD_REG = 32'h0000_0020;

  // parameter for axi4 read/write adapter
  parameter AXI4_TLR = 32'h0000_0014;
  parameter AXI4_RLR = 32'h0000_0024;

  parameter AXI4_TDFD = 32'h8000_1000;
  parameter AXI4_RDFD = 32'h8000_2000;


  parameter BUFF1_SIZE = 32'd512;
  parameter BUFF2_SIZE = 32'd5120;

  parameter READ_CYCLES = 0;

  int timeout_count;
  int fail=0;
  int error_count = 0;

  logic [ 31:0] hm_head = 0;

  initial begin

    tb.power_up();

    tb.nsec_delay(2000);
    poke_ddr_stat();

    // ========================================================================
    // peek and poke test
    // ========================================================================

    $display ("No.1A (concurrent test) ===> AXI-L write to FSB slave:");
    for (int i=0; i<1; i++)
    begin
      $display("initiate FIFO IP No. %d", i);
      ocl_power_up_init(.CFG_BASE_ADDR(CROSSBAR_M0 + 32'h100 * i));
      $display("write to fsb_client No. %d", i);
      ocl_FSB_poke_test(.CFG_BASE_ADDR(CROSSBAR_M0 + 32'h100 * i), .num(2));
      $display("read from fsb_client No. %d", i);
      ocl_FSB_peek_test(.CFG_BASE_ADDR(CROSSBAR_M0 + 32'h100 * i), .num(1));
    end
    // $display ("No.1B (concurrent test) ===> AXI-L read from FSB slave:");    
    // for (int i=0; i<10; i++)
    // begin
    //   $display("read from fsb_client No. %d", i);
    //   ocl_FSB_peek_test(.CFG_BASE_ADDR(CROSSBAR_M0 + 32'h100 + 32'h100 * i), .num(1));
    // end

    // $display ("N0.3 ===> AXI4 to FSB slave test:");
    // pcis_FSB_poke_peek_test(.CFG_BASE_ADDR(CROSSBAR_M2));


    // ========================================================================
    // DMA driver test
    // ========================================================================

    // $display ("No.2A ===> FSB data write to Host:");

    // DMA_pcim_config(.start_addr(64'h0000_0000_0000_0000), .burst_len(0), .buffer_size(BUFF1_SIZE), .CFG_BASE_ADDR(CROSSBAR_M1));
    // DMA_master_wvalid_mask(.fsb_wvalid_mask(32'h0000_0010), .CFG_BASE_ADDR(CROSSBAR_M1));
    // DMA_transfer_start(.start_stop_reg(WR_START), .CFG_BASE_ADDR(CROSSBAR_M1));
    
    // // test master wvalid pause feature
    // // --------------------------------------------
    // # 500ns; // writting

    // DMA_master_wvalid_mask(.fsb_wvalid_mask(32'h0000_0000), .CFG_BASE_ADDR(CROSSBAR_M1));
    // pcim_pop_buffer(.pcim_addr(64'h0000_0000_0000_0000), .CFG_BASE_ADDR(CROSSBAR_M1), .BUFF_SIZE(BUFF1_SIZE), .pop_size(-1));
    // # 100ns
    // DMA_master_wvalid_mask(.fsb_wvalid_mask(32'h0000_0010), .CFG_BASE_ADDR(CROSSBAR_M1));
    // # 5000ns; // writting
    // // --------------------------------------------

    // for (int i=0; i<2; i++) begin
    //   # 13ns
    //   DMA_master_wvalid_mask(.fsb_wvalid_mask(32'h0000_0000), .CFG_BASE_ADDR(CROSSBAR_M1));
    //   # 31ns
    //   DMA_master_wvalid_mask(.fsb_wvalid_mask(32'h0000_0010), .CFG_BASE_ADDR(CROSSBAR_M1));
    //   # 20ns
    //   // pcim_reads_buffer1(.pcim_addr(64'h0000_0000_0000_0000), .CFG_BASE_ADDR(CROSSBAR_M1));
    //   pcim_pop_buffer(.pcim_addr(64'h0000_0000_0000_0000), .CFG_BASE_ADDR(CROSSBAR_M1), .BUFF_SIZE(BUFF1_SIZE), .pop_size(64));
    //   pcim_pop_buffer(.pcim_addr(64'h0000_0000_0000_0000), .CFG_BASE_ADDR(CROSSBAR_M1), .BUFF_SIZE(BUFF1_SIZE), .pop_size(64));
    //   pcim_pop_buffer(.pcim_addr(64'h0000_0000_0000_0000), .CFG_BASE_ADDR(CROSSBAR_M1), .BUFF_SIZE(BUFF1_SIZE), .pop_size(64));
    //   // # 20ns
    //   // // pcim_reads_buffer1(.pcim_addr(64'h0000_0000_0000_0000), .CFG_BASE_ADDR(CROSSBAR_M1));
    //   // pcim_pop_buffer(.pcim_addr(64'h0000_0000_0000_0000), .CFG_BASE_ADDR(CROSSBAR_M1), .BUFF_SIZE(BUFF1_SIZE), .pop_size(128));
    // end

    // // test the soft stop and reset feature
    // // --------------------------------------------
    // DMA_transfer_start(.start_stop_reg(WR_STOP), .CFG_BASE_ADDR(CROSSBAR_M1));
    // DMA_check_tail(.pcim_addr(64'h0000_0000_0000_0000), .BUFF_SIZE(BUFF1_SIZE));
    // # 500ns
    // DMA_transfer_reset(.reset_wd(WR_RESET), .CFG_BASE_ADDR(CROSSBAR_M1));
    // DMA_check_tail(.pcim_addr(64'h0000_0000_0000_0000), .BUFF_SIZE(BUFF1_SIZE));
    // # 500ns
    // DMA_transfer_start(.start_stop_reg(WR_START), .CFG_BASE_ADDR(CROSSBAR_M1));
    // DMA_check_tail(.pcim_addr(64'h0000_0000_0000_0000), .BUFF_SIZE(BUFF1_SIZE));
    // // --------------------------------------------

    // for (int i=0; i<3; i++) begin
    //   # 13ns
    //   DMA_master_wvalid_mask(.fsb_wvalid_mask(32'h0000_0000), .CFG_BASE_ADDR(CROSSBAR_M1));
    //   # 31ns
    //   DMA_master_wvalid_mask(.fsb_wvalid_mask(32'h0000_0010), .CFG_BASE_ADDR(CROSSBAR_M1));
    //   # 20ns
    //   // pcim_reads_buffer1(.pcim_addr(64'h0000_0000_0000_0000), .CFG_BASE_ADDR(CROSSBAR_M1));
    //   pcim_pop_buffer(.pcim_addr(64'h0000_0000_0000_0000), .CFG_BASE_ADDR(CROSSBAR_M1), .BUFF_SIZE(BUFF1_SIZE), .pop_size(64));
    //   pcim_pop_buffer(.pcim_addr(64'h0000_0000_0000_0000), .CFG_BASE_ADDR(CROSSBAR_M1), .BUFF_SIZE(BUFF1_SIZE), .pop_size(64));
    //   pcim_pop_buffer(.pcim_addr(64'h0000_0000_0000_0000), .CFG_BASE_ADDR(CROSSBAR_M1), .BUFF_SIZE(BUFF1_SIZE), .pop_size(64));
    //   // # 20ns
    //   // // pcim_reads_buffer1(.pcim_addr(64'h0000_0000_0000_0000), .CFG_BASE_ADDR(CROSSBAR_M1));
    //   // pcim_pop_buffer(.pcim_addr(64'h0000_0000_0000_0000), .CFG_BASE_ADDR(CROSSBAR_M1), .BUFF_SIZE(BUFF1_SIZE), .pop_size(128));
    // end

    // # 500ns
    // DMA_write_stat_check(.timeout_count(50), .CFG_BASE_ADDR(CROSSBAR_M1));
    // // DMA_buffer_full_check(.buffer_address(pcim_addr+WR_BUFF_SIZE), .timeout_count(50));
    // DMA_adapter_error_check(.CFG_BASE_ADDR(CROSSBAR_M1));



    // $display ("No.2B ===> Trace data write to Host:");

    // pcim_DMA_write_buffer(.pcim_addr(64'h0000_0000_1000_0000), // +64h40 to raise AXI_ERRM_AWADDR_BOUNDARY
    //   .BURST_LEN(1),
    //   .WR_BUFF_SIZE(BUFF2_SIZE),
    //   .CFG_BASE_ADDR(CROSSBAR_M3));
    // pcim_reads_buffer2(.pcim_addr(64'h0000_0000_1000_0000), .CFG_BASE_ADDR(CROSSBAR_M3));

    //---------------------------
    // Report pass/fail status
    //---------------------------
    $display("[%t] : Checking total error count...", $realtime);
    if (error_count > 0)begin
       fail = 1;
    end
    $display("[%t] : Detected %3d errors during this test", $realtime, error_count);

    if (fail || (tb.chk_prot_err_stat())) begin
       $display("[%t] : *** TEST FAILED ***", $realtime);
    end else begin
       $display("[%t] : *** TEST PASSED ***", $realtime);
    end

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

  task compare_dword(logic [32:0] act_data, exp_data);
    if(act_data !== exp_data) begin
        $display($time,,,"***ERROR*** : Data Mismatch!!!");
        $display("Expected Data: %0h", exp_data);
        $display("Actual   Data: %0h", act_data);
       error_count ++;
    end
    else begin
        $display("~~~PASS~~~ : Actual Data %0h is Matched with Expected Data %0h.", act_data, exp_data);
    end
  endtask

  task disp_err(input string s);
    $display($time,,,"***ERROR*** : %s", s);
    error_count ++;
  endtask


  task poke_ddr_stat();
    $display("[%t] : Start poking ddr stats", $realtime);
    tb.nsec_delay(100);
    tb.poke_stat(.addr(8'h0c), .ddr_idx(0), .data(32'h0000_0000));
    tb.poke_stat(.addr(8'h0c), .ddr_idx(1), .data(32'h0000_0000));
    tb.poke_stat(.addr(8'h0c), .ddr_idx(2), .data(32'h0000_0000));
    tb.nsec_delay(27000);
  endtask

// ============================================================================
// basic peek and poke test with axi_fifo_mm_s
// ============================================================================

  task ocl_power_up_init(logic [31:0] CFG_BASE_ADDR);
    logic [31:0] rd_reg;
    tb.peek_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(rd_reg));
    $display($time,,,"Read Interrupt Status Register: %0h", rd_reg);

    tb.poke_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(32'hFFFF_FFFF));
    $display($time,,,"Clear reset done interrupt bits");

    tb.peek_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(rd_reg));
    $display($time,,,"Interrupt Status Register: %0h", rd_reg);

    tb.peek_ocl(.addr(CFG_BASE_ADDR+IER_REG), .data(rd_reg));
    $display($time,,,"Interrupt Enable Register: %0h", rd_reg);

    tb.peek_ocl(.addr(CFG_BASE_ADDR+TDFV_REG), .data(rd_reg));
    $display($time,,,"Transmit Data FIFO Vacancy Register: %0h", rd_reg);

    tb.peek_ocl(.addr(CFG_BASE_ADDR+RDFO_REG), .data(rd_reg));
    $display($time,,,"Receive Data FIFO Occupancy Register: %0h", rd_reg);
  endtask

  task ocl_FSB_poke_test(logic [31:0] CFG_BASE_ADDR, int num);
    logic [31:0] rd_reg;
    
    tb.poke_ocl(.addr(CFG_BASE_ADDR+IER_REG), .data(32'h0C00_0000));
    $display($time,,,"Enable Transmit and Receive Complete interrupt");

    // write
    for (int i=0; i<num; i++) 
    begin
      // tb.peek_ocl(.addr(CFG_BASE_ADDR+TDFV_REG), .data(rd_reg));
      // $display("The Transmit Data FIFO Vacancy is : %d", rd_reg);
      tb.poke_ocl(.addr(CFG_BASE_ADDR+TDFD_REG), .data(32'h1 + 4*i));
      tb.poke_ocl(.addr(CFG_BASE_ADDR+TDFD_REG), .data(32'h2 + 4*i));
      tb.poke_ocl(.addr(CFG_BASE_ADDR+TDFD_REG), .data(32'h3 + 4*i));
      tb.poke_ocl(.addr(CFG_BASE_ADDR+TDFD_REG), .data(32'h4 + 4*i));
      tb.poke_ocl(.addr(CFG_BASE_ADDR+AXIL_TLR), .data(32'h00000010));
      // check if write complete
      tb.peek_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(rd_reg));
      $display($time,,,"Interrupt Status Register: %h", rd_reg);
      compare_dword(rd_reg, 32'h0820_0000);  // 0800_0000 is for write complete, 0020_0000 is for write FIFO empty

      tb.poke_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(32'hFFFF_FFFF));
      $display($time,,,"Clear ISR, transmit complete done reset");
      tb.peek_ocl(.addr(CFG_BASE_ADDR+TDFV_REG), .data(rd_reg));
      $display($time,,,"The Transmit Data FIFO Vacancy is : %d", rd_reg);
    end
  endtask

  task ocl_FSB_peek_test(logic [31:0] CFG_BASE_ADDR, int num);
    logic [31:0] rd_reg;
    logic [31:0] rdata_num_B;
    logic [31:0] rdata_word1;
    logic [31:0] rdata_word2;
    logic [31:0] rdata_word3;
    logic [31:0] rdata_word4;

    // tb.poke_ocl(.addr(CFG_BASE_ADDR+IER_REG), .data(32'h0410_0000));
    tb.poke_ocl(.addr(CFG_BASE_ADDR+IER_REG), .data(32'hFFFF_FFFF));
    $display($time,,,"Enable Write and Receive Complete and Receive FIFO Program Full threshold interrupt");

    // read
    for (int i=0; i<num; i++) 
    begin
      tb.peek_ocl(.addr(CFG_BASE_ADDR+AXIL_RLR), .data(rdata_num_B));
      $display($time,,,"READBACK data from RLR is : %h", rdata_num_B);

      tb.peek_ocl(.addr(CFG_BASE_ADDR+RDFD_REG), .data(rdata_word1));
      tb.peek_ocl(.addr(CFG_BASE_ADDR+RDFD_REG), .data(rdata_word2));
      tb.peek_ocl(.addr(CFG_BASE_ADDR+RDFD_REG), .data(rdata_word3));
      tb.peek_ocl(.addr(CFG_BASE_ADDR+RDFD_REG), .data(rdata_word4));
      // check if read complete
      #100ns
      tb.peek_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(rd_reg));
      $display($time,,,"Interrupt Status Register: %h", rd_reg);
      compare_dword(rd_reg, 32'h0408_0000);  // 0800_0000 is for read complete, 0008_0000 is for read FIFO empty

      tb.poke_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(32'h0400_0000));
      $display($time,,,"Clear ISR, receive complete done reset");

      tb.peek_ocl(.addr(CFG_BASE_ADDR+RDFO_REG), .data(rd_reg));
      $display($time,,,"Receive FIFO Occupancy Register: %h", rd_reg);

      $display ("FSB READBACK DATA %h %h %h %h", rdata_word1, rdata_word2, rdata_word3, rdata_word4);
      if (rdata_word1 == (32'h00000001+4*i) && rdata_word2 == (32'h00000002+4*i)
        // && rdata_word3 == ((((32'h3+4*i)&32'h0000000F)<<12) 
        //                 + (((32'h3+4*i)>>12)&32'h0000000F) 
        //                 + ((32'h3+4*i)&32'h00000FF0))
        && rdata_word3 == 32'h00000003+4*i
        && rdata_word4 == 32'h00000000) 
      begin
        $display ("FSB READBACK DATA PASSED~");
      end else begin
        $display ("FSB READBACK DATA FAILED!!!");
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
    tb.poke_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(32'hFFFFFFFF));
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

        tb.poke(.addr(AXI4_TDFD), .data(pcis_wr_data), .size(DataSize::DATA_SIZE'(6)));
        tb.poke_ocl(.addr(CFG_BASE_ADDR+AXI4_TLR), .data(32'h040));  // write 64 bytes
        // $display("[%t] : axi4 send data = %0h", $realtime, pcis_wr_data);

        tb.poke_ocl(.addr(CFG_BASE_ADDR+AXI4_RLR), .data(32'h040)); // readback 64 bytes
        tb.peek(.addr(AXI4_RDFD), .data(pcis_rd_data), .size(DataSize::DATA_SIZE'(6)));
        // $display("[%t] : axi4 readback data = %0h", $realtime, pcis_rd_data);
        for (int num_bytes=0; num_bytes<64; num_bytes++) begin           
             pcis_exp_data[((addr+num_bytes)*8)+:8] = pcis_wr_data[(num_bytes*8)+:8];
             pcis_act_data[((addr+num_bytes)*8)+:8] = pcis_rd_data[(num_bytes*8)+:8];
        end
      end
      compare_data(.act_data(pcis_act_data), .exp_data(pcis_exp_data));
    end
  endtask


// ============================================================================
// DMA test
// ============================================================================

  task DMA_pcim_config(logic [63:0] start_addr, logic [31:0] burst_len, buffer_size, CFG_BASE_ADDR);
    $display("[%t] : Configuring adapter for DMA write", $realtime);
    tb.poke_ocl(.addr(CFG_BASE_ADDR+WR_ADDR_LOW), .data(start_addr[31:0]));   // write address low
    tb.poke_ocl(.addr(CFG_BASE_ADDR+WR_ADDR_HIGH), .data(start_addr[63:32])); // write address high
    tb.poke_ocl(.addr(CFG_BASE_ADDR+WR_LEN), .data(burst_len));               // write burst size, 0=512bits
    tb.poke_ocl(.addr(CFG_BASE_ADDR+WR_BUF_SIZE), .data(buffer_size));        // buffer size, tail offset
  endtask


  task DMA_transfer_start(logic [31:0] start_stop_reg, CFG_BASE_ADDR);
    tb.poke_ocl(.addr(CFG_BASE_ADDR+CNTL_REG), .data(start_stop_reg));      // Start write
    if (start_stop_reg[0]==1)
      $display("[%t] : ## Start DMA engine!", $realtime);
    else begin
      $display("[%t] : ## Stop DMA engine!", $realtime);
      # 100ns; // wait to stop, normally you should check the CNTL_REG to see if stop is still pending
    end
  endtask

  task DMA_transfer_reset(logic [31:0] reset_wd, CFG_BASE_ADDR);
    $display("[%t] : ## Reset DMA engine!", $realtime);
    tb.poke_ocl(.addr(CFG_BASE_ADDR+RESET_REG), .data(reset_wd));         // reset the tail in CL
    hm_head = 0;                                                          // reset the head in SH
    tb.poke_ocl(.addr(CFG_BASE_ADDR+WR_READ_HEAD), .data(32'h0000_0000)); // reset the tail in CL
  endtask

  task DMA_check_tail(logic [63:0] pcim_addr, logic[31:0] BUFF_SIZE);
    logic [31:0] hm_tail;
    for (int j=0; j<4; j++) begin
      hm_tail[(j*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+BUFF_SIZE+j)); // check the write tail
    end
    $display("[%t] : Current tail word is %h", $realtime, hm_tail[31:0]);
  endtask

  task DMA_master_wvalid_mask(logic [31:0] fsb_wvalid_mask, CFG_BASE_ADDR);
    if (fsb_wvalid_mask[4])
      $display("[%t] : Start DMA write !", $realtime);
    else
      $display("[%t] : Stop DMA write !", $realtime);

    tb.poke_ocl(.addr(CFG_BASE_ADDR+CFG_REG), .data(fsb_wvalid_mask));        // set mask to enable fsb_wvalid
  endtask


  task DMA_write_stat_check(int timeout_count, logic [31:0] CFG_BASE_ADDR);
    int cnt = 0;
    logic [31:0] cfg_rdata_ctrl;
    logic [31:0] cfg_rdata_wr_phase_num;
    do begin
      # 2000ns;
      // check if is still writting
      tb.peek_ocl(.addr(CFG_BASE_ADDR+CNTL_REG), .data(cfg_rdata_ctrl));
      $display("[%t] : Waiting for 1st write activity to complete", $realtime);
      cnt ++;
      tb.peek_ocl(.addr(CFG_BASE_ADDR+WR_PHASE_NUM), .data(cfg_rdata_wr_phase_num));
      $display("[%t] : Number of write phases since last start is %d:", $realtime, cfg_rdata_wr_phase_num);
    end while((cfg_rdata_ctrl[0] !== 1'b0) && cnt < timeout_count);

    if ((cfg_rdata_ctrl[0] !== 1'b0) || (cnt == timeout_count))
      $display("[%t] : *** ERROR *** Timeout waiting for 1st writes to complete.", $realtime);
    else
      $display("[%t] : PASS ~~~ axi4 1st writes complete.", $realtime);
  endtask


  task DMA_adapter_error_check(logic [31:0] CFG_BASE_ADDR);
    logic [31:0] cfg_rdata;
    tb.peek_ocl(.addr(CFG_BASE_ADDR+CNTL_REG), .data(cfg_rdata));
    if (cfg_rdata[3])
      $display("[%t] : *** ERROR *** axi bus RESP is NOT OKAY.", $realtime);
    else
      $display("[%t] : ~~~ OKAY ~~~ axi bus RESP[1] is 0.", $realtime);
  endtask


  task DMA_buffer_full_check(logic [63:0] buffer_address, int timeout_count);
    int cnt = 0;
    int tail_change;
    logic [63:0] hm_tail_old = 0;
    logic [63:0] hm_tail;
    do begin
      # 2000ns
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


  task pcim_read (logic [31:0] start_addr, int read_num);
    int read_16B_num;
    logic [127:0] hm_fsb_pkt;
    read_16B_num = (read_num - read_num % 16) / 16;
    for (int j=0; j<read_16B_num; j++) begin
      for (int k=0; k<16; k++) begin
        hm_fsb_pkt[8*k+:8] = tb.hm_get_byte(.addr(start_addr+16*j+k));
      end
      $display("[%t] : readback fsb_data @ %h : %h", $realtime, start_addr+16*j, hm_fsb_pkt);
    end
    hm_fsb_pkt = 'X;
    read_16B_num =  (read_num % 16);
    if (read_16B_num) begin
      for (int k=0; k<read_16B_num; k++) begin
        hm_fsb_pkt[8*k+:8] = tb.hm_get_byte(.addr(start_addr+16+k));
      end
      $display("[%t] : readback fsb_data @ %h : %h", $realtime, start_addr+16*read_16B_num, hm_fsb_pkt);
    end
  endtask

  task pcim_pop_buffer(logic [63:0] pcim_addr, logic [31:0] CFG_BASE_ADDR, logic [31:0] BUFF_SIZE, int pop_size);
    logic [ 31:0] hm_tail;
    bit can_read;
    int unused_B_num;
    int read_B_num;
    $display("[%t] : ==> Start pop the DMA...", $realtime);

    for (int j=0; j<4; j++) begin
      hm_tail[(j*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+BUFF_SIZE+j)); // check the write tail
    end
    $display("[%t] : Readback the write tail word @ %h : %h", $realtime, pcim_addr+BUFF_SIZE, hm_tail[31:0]);


    if (hm_tail >= hm_head) begin
      unused_B_num = hm_tail - hm_head;
    end
    else begin
      unused_B_num = hm_tail - hm_head + BUFF_SIZE;
    end
    if (pop_size==-1) begin
      pop_size = unused_B_num;
    end
    can_read = unused_B_num >= pop_size;
    if (can_read) begin
      read_B_num = (BUFF_SIZE - hm_head >= pop_size) ? pop_size : BUFF_SIZE - hm_head;
      pcim_read(.start_addr(pcim_addr + hm_head), .read_num(read_B_num));
      hm_head = (hm_head + read_B_num) % BUFF_SIZE;

      read_B_num = pop_size - read_B_num;
      if (read_B_num>0) begin
        pcim_read(.start_addr(pcim_addr + hm_head), .read_num(read_B_num));
        hm_head = hm_head + read_B_num;
      end
      tb.poke_ocl(.addr(CFG_BASE_ADDR+WR_READ_HEAD), .data(32'h0000_0000 + hm_head));
      for (int j=0; j<4; j++) begin
        hm_tail[(j*8)+:8] = tb.hm_get_byte(.addr(pcim_addr+BUFF_SIZE+j)); // check the write tail
      end
      $display("[%t] : Update the head and check the tail: (%h, %h)", $realtime, hm_head, hm_tail);
    end
    else
      $display("[%t] : No more valid data at this pop: (%h, %h)", $realtime, hm_head, hm_tail);
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
          # 500ns // Delay enough time to ensure that we have data in buffer to read
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
          # 500ns // Delay enough time to ensure that we have data in buffer to read
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



// Table 2-4: Register Names and Descriptions (Xilinx IP Datasheet PG080)
// Register Name                AXI Address                       Access
// Interrupt Status Register (ISR) C_BASEADDR + 0x0 Read/Clear on Write(1)
// Interrupt Enable Register (IER)) C_BASEADDR + 0x4 Read/Write
// Transmit Data FIFO Reset (TDFR) C_BASEADDR + 0x8 Write(2)
// Transmit Data FIFO Vacancy (TDFV) C_BASEADDR + 0xC Read
    //  N writes to Transmit FIFO. 
    // The value of this register after reset is C_TX_FIFO_DEPTH-4. The register does not
    // decrement for every Transmit Data FIFO Data Write Port (TDFD) write. 
    // It decrements by two for every two write locations.
// Transmit Data FIFO 32-bit Wide Data Write Port (TDFD) C_BASEADDR + 0x10 or C_AXI4_BASEADDR + 0x0000 Write(3)
// Transmit Length Register (TLR) C_BASEADDR + 0x14 Write
// Receive Data FIFO reset (RDFR) C_BASEADDR + 0x18 Write(2)
// Receive Data FIFO Occupancy (RDFO) C_BASEADDR + 0x1C Read
// Receive Data FIFO 32-bit Wide Data Read Port (RDFD) C_BASEADDR + 0x20 or C_AXI4_BASEADDR + 0x1000 Read(3)
// Receive Length Register (RLR) C_BASEADDR + 0x24 Read
// AXI4-Stream Reset (SRR) C_BASEADDR + 0x28 Write(2)
// Transmit Destination Register (TDR) C_BASEADDR + 0x2C Write
// Receive Destination Register (RDR) C_BASEADDR + 0x30 Read
// Transmit ID Register(4) C_BASEADDR + x34 Write
// Transmit USER Register(4) C_BASEADDR + x38 Write
// Receive ID Register(4) C_BASEADDR + x3C Read
// Receive USER Register(4) C_BASEADDR + x40 Read