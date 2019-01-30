/**
 *  cl_crossbar_tb.sv
 *
 *  testbench for crossbar functionality
 */

module cl_crossbar_tb();

  import tb_type_defines_pkg::*;

  // parameter for axil read/write adapter
  parameter CROSSBAR_M0  = 32'h0000_0000;

  parameter ISR_REG = 32'h0000_0000;
  parameter IER_REG = 32'h0000_0004;

  parameter TDR_REG = 32'h0000_002C;
  parameter RDR_REG = 32'h0000_0030;

  parameter TDFV_REG = 32'h0000_000C;
  parameter RDFO_REG = 32'h0000_001C;

  parameter AXIL_TLR = 32'h0000_0014;
  parameter AXIL_RLR = 32'h0000_0024;

  parameter TDFD_REG = 32'h0000_0010;
  parameter RDFD_REG = 32'h0000_0020;

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
    for (int i=0; i<2; i++)
    begin
      $display($time,,,"initiate FIFO IP No. %d", i);
      ocl_power_up_init(.CFG_BASE_ADDR(CROSSBAR_M0 + 32'h100 * i));
      $display($time,,,"write to fsb_client No. %d", i);
      ocl_FSB_poke_test(.CFG_BASE_ADDR(CROSSBAR_M0 + 32'h100 * i), .num(1));
      $display($time,,,"read from fsb_client No. %d", i);
      ocl_FSB_peek_test(.CFG_BASE_ADDR(CROSSBAR_M0 + 32'h100 * i), .num(1));
    end

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
        $display($time,,,"***ERROR*** : Data Mismatch!!! Expected Data: %0h, Actual   Data: %0h", exp_data, act_data);
       error_count ++;
    end
    else begin
        $display("~~~PASS~~~ : Data is Matched. Expected Data: %0h, Actual   Data: %0h", exp_data, act_data);
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
    tb.poke_stat(.addr(8'h0c), .ddr_idx(0), .data(32'h0000_0000));
    tb.poke_stat(.addr(8'h0c), .ddr_idx(1), .data(32'h0000_0000));
    tb.poke_stat(.addr(8'h0c), .ddr_idx(2), .data(32'h0000_0000));
  endtask

// ============================================================================
// basic peek and poke test with axi_fifo_mm_s
// ============================================================================

  task ocl_power_up_init(logic [31:0] CFG_BASE_ADDR);
    logic [31:0] rd_reg;
    tb.peek_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(rd_reg));
    $display($time,,,"Read ISR: %0h", rd_reg);
    compare_dword(rd_reg, 32'h01d0_0000);

    tb.poke_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(32'hFFFF_FFFF));
    $display($time,,,"Clear ISR");

    tb.peek_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(rd_reg));
    $display($time,,,"Read ISR: %0h", rd_reg);

    tb.peek_ocl(.addr(CFG_BASE_ADDR+IER_REG), .data(rd_reg));
    $display($time,,,"Read IER: %0h", rd_reg);

    tb.peek_ocl(.addr(CFG_BASE_ADDR+TDFV_REG), .data(rd_reg));
    $display($time,,,"Read TDFV: %0h", rd_reg);

    tb.peek_ocl(.addr(CFG_BASE_ADDR+RDFO_REG), .data(rd_reg));
    $display($time,,,"Read RDFO: %0h", rd_reg);
  endtask

  task ocl_FSB_poke_test(logic [31:0] CFG_BASE_ADDR, int num);
    logic [31:0] rd_reg;
    tb.poke_ocl(.addr(CFG_BASE_ADDR+IER_REG), .data(32'h0C00_0000));
    $display($time,,,"Enable Transmit and Receive Complete interrupts");

    tb.poke_ocl(.addr(CFG_BASE_ADDR+TDR_REG), .data(32'h0000_0000));
    $display($time,,,"Transmit Destination Address 0x0");

    // write
    for (int i=0; i<num; i++) 
    begin
      tb.poke_ocl(.addr(CFG_BASE_ADDR+TDFD_REG), .data(32'h1 + 4*i));
      tb.poke_ocl(.addr(CFG_BASE_ADDR+TDFD_REG), .data(32'h2 + 4*i));
      tb.poke_ocl(.addr(CFG_BASE_ADDR+TDFD_REG), .data(32'h3 + 4*i));
      tb.poke_ocl(.addr(CFG_BASE_ADDR+TDFD_REG), .data(32'h4 + 4*i));
      tb.poke_ocl(.addr(CFG_BASE_ADDR+AXIL_TLR), .data(32'h00000010));

      tb.peek_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(rd_reg));
      $display($time,,,"Read ISR: %h", rd_reg);
      // 0800_0000 is for write complete, 0020_0000 is for write FIFO empty
      compare_dword(rd_reg, 32'h0820_0000);

      tb.poke_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(32'hFFFF_FFFF));
      $display($time,,,"Clear ISR");

      tb.peek_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(rd_reg));
      $display($time,,,"Read ISR: %h", rd_reg);

      tb.peek_ocl(.addr(CFG_BASE_ADDR+TDFV_REG), .data(rd_reg));
      $display($time,,,"Transmit FIFO Vacancy is: %h", rd_reg);
    end
  endtask

  task ocl_FSB_peek_test(logic [31:0] CFG_BASE_ADDR, int num);
    logic [31:0] rd_reg;
    tb.poke_ocl(.addr(CFG_BASE_ADDR+IER_REG), .data(32'h0410_0000));
    $display($time,,,"Enable Write and Receive Complete and Receive FIFO Program Full threshold interrupt");

    tb.peek_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(rd_reg));
    $display($time,,,"Read ISR: %h", rd_reg);

    tb.poke_ocl(.addr(CFG_BASE_ADDR+IER_REG), .data(32'hFFFF_FFFF));
    $display($time,,,"Clear ISR");

    // read
    for (int i=0; i<num; i++) 
    begin
      tb.peek_ocl(.addr(CFG_BASE_ADDR+AXIL_RLR), .data(rd_reg));
      $display($time,,,"Read RLR : %h", rd_reg);
      tb.peek_ocl(.addr(CFG_BASE_ADDR+RDFD_REG), .data(rd_reg));
      compare_dword(rd_reg, (32'h00000001+4*i));
      tb.peek_ocl(.addr(CFG_BASE_ADDR+RDFD_REG), .data(rd_reg));
      compare_dword(rd_reg, (32'h00000002+4*i));
      tb.peek_ocl(.addr(CFG_BASE_ADDR+RDFD_REG), .data(rd_reg));
      compare_dword(rd_reg, (32'h00000003+4*i));
      tb.peek_ocl(.addr(CFG_BASE_ADDR+RDFD_REG), .data(rd_reg));
      compare_dword(rd_reg, (32'h0000_0000));

      tb.peek_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(rd_reg));
      $display($time,,,"Read ISR: %h", rd_reg);
      // 0008_0000 is for write FIFO empty
      compare_dword(rd_reg, 32'h0408_0000);

      tb.poke_ocl(.addr(CFG_BASE_ADDR+ISR_REG), .data(32'hFFFF_FFFF));
      $display($time,,,"Clear ISR");

      tb.peek_ocl(.addr(CFG_BASE_ADDR+RDFO_REG), .data(rd_reg));
      $display($time,,,"Receive FIFO Occupancy is: %h", rd_reg);
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