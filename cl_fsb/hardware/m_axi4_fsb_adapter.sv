/**
 *  m_axi4_fsb_adapter.v
 *
 *  cl_bsg (master) -> axi-4 (slave)
 */

 module m_axi4_fsb_adapter 
  #(parameter DATA_WIDTH=512
    ,parameter NUM_RD_TAG=512
    ,parameter FSB_WIDTH=80
    ,parameter BUF_TAIL_OFFSET=64'h500
  ) (
   input clk_i
   ,input resetn_i

   ,cfg_bus_t.master cfg_bus
   ,axi_bus_t.slave cl_sh_pcim_bus
   ,output logic atg_dst_sel

   ,input fsb_wvalid
   ,input [FSB_WIDTH-1:0] fsb_wdata
   ,output logic fsb_yumi
 );
 
parameter DATA_BYTE_NUM = DATA_WIDTH/8;


//-------------------------------------
// AXI4 register slice 
// Flop signals between CL and SH
//-------------------------------------

// axi-4 signals to below
logic [8:0] awid; 
logic [63:0] awaddr;
logic[7:0] awlen;
logic awvalid;
logic[10:0] awuser = 0; // not used
logic awready;

logic [8:0] wid; // not used
logic [DATA_WIDTH-1:0] wdata = 0;
logic [(DATA_WIDTH/8)-1:0] wstrb = 0;
logic wlast;
logic wvalid;
logic wready;

logic [8:0] bid;
logic [1:0] bresp;
logic  bvalid;
logic [17:0] buser = 0;
logic  bready;

logic [8:0] arid;
logic [63:0] araddr;
logic [7:0] arlen;
logic  arvalid;
logic [10:0] aruser = 0; // not used
logic arready;

logic [8:0] rid;
logic [DATA_WIDTH-1:0] rdata;
logic [1:0] rresp;
logic rlast;
logic rvalid;
logic [17:0] ruser = 0;
logic rready;

axi_bus_t axi4_m_bus();

assign axi4_m_bus.awid = {7'b0, awid};
assign axi4_m_bus.awaddr = awaddr;
assign axi4_m_bus.awlen = awlen;
assign axi4_m_bus.awsize = 3'h6;
assign axi4_m_bus.awvalid = awvalid;
assign awready = axi4_m_bus.awready;

assign axi4_m_bus.wdata = wdata; 
assign axi4_m_bus.wstrb = wstrb;
assign axi4_m_bus.wlast = wlast;
assign axi4_m_bus.wvalid = wvalid;
assign wready = axi4_m_bus.wready;

assign bid = axi4_m_bus.bid[8:0];
assign bresp = axi4_m_bus.bresp;
assign bvalid = axi4_m_bus.bvalid;
assign axi4_m_bus.bready = bready;

assign axi4_m_bus.arid = {7'b0, arid};
assign axi4_m_bus.araddr = araddr;
assign axi4_m_bus.arlen = arlen;
assign axi4_m_bus.arsize = 3'h6;
assign axi4_m_bus.arvalid = arvalid;
assign arready = axi4_m_bus.arready;

assign rid = axi4_m_bus.rid[8:0];
assign rdata = axi4_m_bus.rdata;
assign rresp = axi4_m_bus.rresp;
assign rlast = axi4_m_bus.rlast;
assign rvalid = axi4_m_bus.rvalid;
assign axi4_m_bus.rready = rready;

axi_register_slice PCI_AXI4_REG_SLC (
 .aclk           (clk_i),
 .aresetn        (resetn_i),
                                                                                                                     
 .s_axi_awid     (axi4_m_bus.awid),
 .s_axi_awaddr   (axi4_m_bus.awaddr),
 .s_axi_awlen    (axi4_m_bus.awlen),
 .s_axi_awsize   (axi4_m_bus.awsize),
 .s_axi_awvalid  (axi4_m_bus.awvalid),
 .s_axi_awready  (axi4_m_bus.awready),
 .s_axi_wdata    (axi4_m_bus.wdata),
 .s_axi_wstrb    (axi4_m_bus.wstrb),
 .s_axi_wlast    (axi4_m_bus.wlast),
 .s_axi_wvalid   (axi4_m_bus.wvalid),
 .s_axi_wready   (axi4_m_bus.wready),
 .s_axi_bid      (axi4_m_bus.bid),
 .s_axi_bresp    (axi4_m_bus.bresp),
 .s_axi_bvalid   (axi4_m_bus.bvalid),
 .s_axi_bready   (axi4_m_bus.bready),
 .s_axi_arid     (axi4_m_bus.arid),
 .s_axi_araddr   (axi4_m_bus.araddr),
 .s_axi_arlen    (axi4_m_bus.arlen),
 .s_axi_arsize   (axi4_m_bus.arsize),
 .s_axi_arvalid  (axi4_m_bus.arvalid),
 .s_axi_arready  (axi4_m_bus.arready),
 .s_axi_rid      (axi4_m_bus.rid),
 .s_axi_rdata    (axi4_m_bus.rdata),
 .s_axi_rresp    (axi4_m_bus.rresp),
 .s_axi_rlast    (axi4_m_bus.rlast),
 .s_axi_rvalid   (axi4_m_bus.rvalid),
 .s_axi_rready   (axi4_m_bus.rready),  

 .m_axi_awid     (cl_sh_pcim_bus.awid),   
 .m_axi_awaddr   (cl_sh_pcim_bus.awaddr), 
 .m_axi_awlen    (cl_sh_pcim_bus.awlen),  
 .m_axi_awsize   (cl_sh_pcim_bus.awsize), 
 .m_axi_awvalid  (cl_sh_pcim_bus.awvalid),
 .m_axi_awready  (cl_sh_pcim_bus.awready),
 .m_axi_wdata    (cl_sh_pcim_bus.wdata),  
 .m_axi_wstrb    (cl_sh_pcim_bus.wstrb),  
 .m_axi_wlast    (cl_sh_pcim_bus.wlast),  
 .m_axi_wvalid   (cl_sh_pcim_bus.wvalid), 
 .m_axi_wready   (cl_sh_pcim_bus.wready), 
 .m_axi_bid      (cl_sh_pcim_bus.bid),    
 .m_axi_bresp    (cl_sh_pcim_bus.bresp),  
 .m_axi_bvalid   (cl_sh_pcim_bus.bvalid), 
 .m_axi_bready   (cl_sh_pcim_bus.bready), 
 .m_axi_arid     (cl_sh_pcim_bus.arid),   
 .m_axi_araddr   (cl_sh_pcim_bus.araddr), 
 .m_axi_arlen    (cl_sh_pcim_bus.arlen),  
 .m_axi_arsize   (cl_sh_pcim_bus.arsize), 
 .m_axi_arvalid  (cl_sh_pcim_bus.arvalid),
 .m_axi_arready  (cl_sh_pcim_bus.arready),
 .m_axi_rid      (cl_sh_pcim_bus.rid),    
 .m_axi_rdata    (cl_sh_pcim_bus.rdata),  
 .m_axi_rresp    (cl_sh_pcim_bus.rresp),  
 .m_axi_rlast    (cl_sh_pcim_bus.rlast),  
 .m_axi_rvalid   (cl_sh_pcim_bus.rvalid), 
 .m_axi_rready   (cl_sh_pcim_bus.rready)
 );


// Global clock
//---------------------------------------------
logic clk;
assign clk = clk_i;

// Sync reset
logic pre_sync_rst_n;
logic sync_rst_n;

always_ff @(negedge resetn_i or posedge clk_i)
   if (!resetn_i)
   begin
      pre_sync_rst_n <= 0;
      sync_rst_n <= 0;
   end
   else
   begin
      pre_sync_rst_n <= 1;
      sync_rst_n <= pre_sync_rst_n;
   end


//---------------------------------------------
// Flop read interface for timing (not used now)
//---------------------------------------------
logic[8:0] rid_q = 0;
logic[DATA_WIDTH-1:0] rdata_q = 0;
logic[1:0] rresp_q = 0;
logic rlast_q = 0;
logic rvalid_q = 0;

always @(posedge clk)
   begin
      rid_q <= rid;
      rdata_q <= rdata;
      rresp_q <= rresp;
      rlast_q <= rlast;
      rvalid_q <= rvalid;
   end


//-------------------------------------------
// register configuration
//-------------------------------------------
// 0x00:
//        3 - read compare
// 0x08:
//        0 - write to go
//        1 - read to go
// 0x0c:
//        0 - write reset
//        1 - read reset

// 0x20:  write start address low
// 0x24:  write start address high
// 0x28:  write end point (read tail)
// 0x2c:  write length select
//        7:0 - number of the AXI write data phases
//        15:8 - last data adj, i.e. number of DW to adj last data phase
//        31:16 - user defined
// 0x30:  write end address offset (buffer size)

// 0x40:  read address low
// 0x44:  read address high
// 0x48:  expected read data to compare with write 
// 0x4c:  read length select
//        7:0 - number of the AXI read data phases
//        15:8 - last data adj, i.e. number of DW to adj last data phase
//        31:16 - user defined

// 0xe0:  0 -  0/1 to select which dst module the atg drives



// pip the cfg_bus, store address and wdata
//-------------------------------------------
logic cfg_wr_stretch; // cfg_wr_stretch is 1 cycle ahead of the tst_cfg_ack
logic cfg_rd_stretch;

logic[7:0] cfg_addr_q = 0;  // the upper bits are decoded at cl_ocl_clv
logic[31:0] cfg_wdata_q = 0;

logic tst_cfg_ack;

assign cfg_bus.ack = tst_cfg_ack;

always @(posedge clk)
   if (!sync_rst_n)
   begin
      cfg_wr_stretch <= 0;
      cfg_rd_stretch <= 0;
   end
   else 
   begin
      cfg_wr_stretch <= cfg_bus.wr || (cfg_wr_stretch && !tst_cfg_ack);
      cfg_rd_stretch <= cfg_bus.rd || (cfg_rd_stretch && !tst_cfg_ack);
      if (cfg_bus.wr||cfg_bus.rd)
      begin
         cfg_addr_q <= cfg_bus.addr[7:0];
         cfg_wdata_q <= cfg_bus.wdata;
      end
   end


// Ack for cycle
always_ff @(posedge clk)
   if (!sync_rst_n)
      tst_cfg_ack <= 0;
   else
      tst_cfg_ack <= ((cfg_wr_stretch||cfg_rd_stretch) && !tst_cfg_ack); 


// store control registers
//-------------------------------------------
logic cfg_rd_compare_en = 0;

logic [63:0] cfg_write_address = 0;
logic [31:0] cfg_write_data = 0;
logic [31:0] cfg_adress_offset = 0;

logic[7:0] cfg_write_length;
logic[7:0] cfg_write_last_length;
logic[15:0] cfg_write_user;

logic [63:0] cfg_read_address = 0;
logic [31:0] cfg_read_data = 0;

logic[7:0] cfg_read_length;
logic[7:0] cfg_read_last_length;
logic[15:0] cfg_read_user;

logic cfg_atg_dst_sel = 0;

assign   atg_dst_sel = cfg_atg_dst_sel;

always @(posedge clk)
   if (cfg_wr_stretch)
   begin
      if (cfg_addr_q==8'h0)
      begin
         cfg_rd_compare_en <= cfg_wdata_q[3];
      end
      else if (cfg_addr_q==8'h20)
         cfg_write_address[31:0] <= cfg_wdata_q;
      else if (cfg_addr_q==8'h24)
         cfg_write_address[63:32] <= cfg_wdata_q;
      else if (cfg_addr_q==8'h28)
         cfg_write_data <= cfg_wdata_q;
      else if (cfg_addr_q==8'h2c)
         {cfg_write_user, cfg_write_last_length, cfg_write_length} <= cfg_wdata_q;
      else if (cfg_addr_q==8'h30)
         cfg_adress_offset <= cfg_wdata_q;

      else if (cfg_addr_q==8'h40)
         cfg_read_address[31:0] <= cfg_wdata_q;
      else if (cfg_addr_q==8'h44)
         cfg_read_address[63:32] <= cfg_wdata_q;
      else if (cfg_addr_q==8'h48)
         cfg_read_data <= cfg_wdata_q;
      else if (cfg_addr_q==8'h4c)
         {cfg_read_user, cfg_read_last_length, cfg_read_length} <= cfg_wdata_q;

      else if (cfg_addr_q==8'he0)
         cfg_atg_dst_sel <= cfg_wdata_q[0];
   end


// record control signals
//-------------------------------------------
logic wr_inp;
logic rd_inp;
logic wr_stop_pend;

logic cfg_wr_go;
logic cfg_rd_go;
logic cfg_wr_stop;
logic cfg_rd_stop;

logic cfg_write_reset;
logic cfg_read_reset;

assign cfg_wr_go = (cfg_wr_stretch && tst_cfg_ack && (cfg_addr_q==8'h08) && cfg_wdata_q[0]) && !wr_inp;
assign cfg_rd_go = (cfg_wr_stretch && tst_cfg_ack && (cfg_addr_q==8'h08) && cfg_wdata_q[1]) && !rd_inp;

assign cfg_wr_stop = (cfg_wr_stretch && tst_cfg_ack && (cfg_addr_q==8'h08) && ~cfg_wdata_q[0]);
assign cfg_rd_stop = (cfg_wr_stretch && tst_cfg_ack && (cfg_addr_q==8'h08) && ~cfg_wdata_q[1]);

assign cfg_write_reset = (cfg_wr_stretch && tst_cfg_ack && (cfg_addr_q==8'h0c) && cfg_wdata_q[0]);
assign cfg_read_reset = (cfg_wr_stretch && tst_cfg_ack && (cfg_addr_q==8'h0c) && cfg_wdata_q[1]);

// Readback mux
always_ff @(posedge clk)
begin
  if (cfg_wr_stretch)
    case (cfg_addr_q)
       8'h00:      cfg_bus.rdata <= {8'h0, 8'h0, 8'h0, 4'h0, cfg_rd_compare_en, 3'h0};
       8'h08:      cfg_bus.rdata <= {28'b0, bresp_q, wr_stop_pend, rd_inp, wr_inp};
       8'h20:      cfg_bus.rdata <= cfg_write_address[31:0];
       8'h24:      cfg_bus.rdata <= cfg_write_address[63:32];
       8'h28:      cfg_bus.rdata <= cfg_write_data;
       8'h2c:      cfg_bus.rdata <= {cfg_write_user, cfg_write_last_length, cfg_write_length};
       8'h30:      cfg_bus.rdata <= cfg_adress_offset;

       8'h40:      cfg_bus.rdata <= cfg_read_address[31:0];
       8'h44:      cfg_bus.rdata <= cfg_read_address[63:32]; 
       8'h48:      cfg_bus.rdata <= cfg_read_data; 
       8'h4c:      cfg_bus.rdata <= {cfg_read_user, cfg_read_last_length, cfg_read_length};

       8'he0:      cfg_bus.rdata <= {31'b0, cfg_atg_dst_sel};

       default:    cfg_bus.rdata <= 32'hffffffff;
    endcase
end


//--------------------------------
// AXI Write state machine      
//--------------------------------

logic wr_phase_end;   // end of data for this instruction (single transfer)
logic wr_trans_done;  // done with write from cfg_write_address to addr + offset

typedef enum logic[1:0] {
   WR_IDLE = 0
   ,WR_ADDR = 1
   ,WR_DAT = 2
   ,WR_STOP = 3
   } wr_state_t;

wr_state_t wr_state, wr_state_nxt;

always_comb
begin
   wr_state_nxt = wr_state;
   case (wr_state)

      WR_IDLE:
      begin
         if (cfg_wr_go)
            wr_state_nxt = WR_ADDR;
         else
            wr_state_nxt = WR_IDLE;
      end

      WR_ADDR:
      begin
         if (awready & awvalid)
            wr_state_nxt = WR_DAT;
         else  
            wr_state_nxt = WR_ADDR;
      end

      WR_DAT:
      begin
         if (wvalid && wready && wlast)
         begin
            if (wr_trans_done || wr_stop_pend)
               wr_state_nxt = WR_STOP;
            else
               wr_state_nxt = WR_ADDR;
         end
         else
            wr_state_nxt = WR_DAT;
      end

      WR_STOP:
      begin
        if (cfg_write_reset)
          wr_state_nxt = WR_IDLE;
        else
          wr_state_nxt = WR_STOP;
      end

   endcase
end

always_ff @(posedge clk)
   if (!sync_rst_n)
      wr_state <= WR_IDLE;
   else
      wr_state <= wr_state_nxt;


// AXI transfer in process
//--------------------------------
assign wr_inp = ((wr_state!=WR_IDLE) && (wr_state!=WR_STOP));


// AXI transfer stop
//--------------------------------


// limit the address range for the transfer (test only)
logic [31:0] wr_mem_left = 0;

logic [31:0] wr_last_addr;

logic [31:0] wr_mem_left_next;

assign wr_mem_left_next = wr_mem_left - DATA_BYTE_NUM - DATA_BYTE_NUM * cfg_write_length;

assign wr_last_addr = {cfg_adress_offset[31:6], 6'd0}; // must be 64 bytes aligned

always_ff @(posedge clk)
   if ((wr_state==WR_IDLE) || (wr_state==WR_STOP))
   begin
      wr_mem_left <= wr_last_addr;
   end
   else if ((wr_state==WR_DAT) && (wr_state_nxt!=WR_DAT))
   begin
    if(wr_dat_tail_flag)
    begin
      wr_mem_left <= wr_mem_left_next;  // tail is already updated, so we can count on
      wr_trans_done <= 1'b0;
    end
    else
    begin
      wr_mem_left <= wr_mem_left;  // tais will be send next cycle
      wr_trans_done <= (wr_mem_left==0);
    end
   end

// soft stop
always_ff @(posedge clk)
   if (!sync_rst_n)
      wr_stop_pend <= 0;
   else
      wr_stop_pend <= cfg_wr_stop || (wr_stop_pend && (wr_state_nxt!=WR_IDLE));


// AXI bus signals
//--------------------------------

assign bready = 1;  //Don't do anything with BRESP

logic bresp_q;

always_ff @(posedge clk)
  if (bvalid & bready)
    bresp_q = bresp;  // record the bus status


// AXI write address
//--------------------------------
logic [64:0] wr_addr_next;

logic [63:0] wr_addr;

assign wr_addr_next = wr_addr + DATA_BYTE_NUM + DATA_BYTE_NUM * cfg_write_length;

// the flag determines wheather to send data or tail in a axi write frame 
logic wr_dat_tail_flag = 0;

always_ff @( posedge clk)
   if (wr_state==WR_IDLE)
   begin
      wr_dat_tail_flag <= 0;  // keep this flag when soft stop occurs
      wr_addr <= cfg_write_address;  // the start address shall be 64 bytes aligned...
  end
   else if ((wr_state==WR_DAT) && (wr_state_nxt!=WR_DAT))
   begin
      // for next frame address
      wr_dat_tail_flag <= ~wr_dat_tail_flag;
      wr_addr <= wr_dat_tail_flag ? wr_addr_next : wr_addr;
   end

always_ff @( posedge clk)
  if(!sync_rst_n)
  begin
    awvalid <= 0;
    awaddr <= 0;
    awid <= 0;
    awlen <= 0;
    awuser <= 0;
  end
  else if ((wr_state==WR_ADDR) && (wr_state_nxt==WR_ADDR))
  begin
    awvalid <= 1'b1;  // addr is prepared just after the last phase, so always avaliable
    awaddr <= wr_dat_tail_flag ? (cfg_write_address + BUF_TAIL_OFFSET) : wr_addr;
    awid <= 0;
    awlen <= wr_dat_tail_flag ? 8'b0 : cfg_write_length;
    awuser <= 0;
  end
  else
  begin
    awvalid <= 0;
    // awaddr <= 0;
    awid <= 0;
    awlen <= 0;
    awuser <= 0;
  end


// AXI write data
//--------------------------------

logic wr_whole_phase_valid; // fsb pkt is packed up for one axi phase
logic wr_frctn_phase_valid; // fsb pkt is partly packed and hold for valid signal

logic wr_phase_valid;

assign wr_phase_valid = wr_dat_tail_flag ? (wr_state==WR_DAT) : wr_whole_phase_valid;

logic [DATA_WIDTH-1:0] wr_phase_data = 0;
logic [(DATA_WIDTH/8)-1:0] wr_phase_strb;

always_ff @(posedge clk)
begin
  if(wr_phase_valid)
  begin
    wdata <= wr_dat_tail_flag ? {{(DATA_WIDTH-64){1'b1}}, wr_addr_next} : wr_phase_data;
    wstrb <= wr_dat_tail_flag ? 64'h0000_0000_0000_00FF : wr_phase_strb;
  end
end

always_ff @( posedge clk)
   if (!sync_rst_n)
   begin
      wid <= 0;
      wvalid <= 0;
      wlast <= 0;
   end
   else if ((wr_state==WR_DAT) && (wr_state_nxt==WR_DAT) && wr_phase_valid)
   begin
      wid <= 0;
      wvalid <= 1'b1;
      wlast <= wr_dat_tail_flag ? 1'b1 : wr_phase_end;
   end
   else
   begin
      wid <= 0;
      wvalid <= 1'b0;
      wlast <= 1'b0;
   end



// burst control
//--------------------------------

logic [7:0] wr_running_length = 0;
assign wr_phase_end = (wr_running_length==0);

always_ff @(posedge clk)
   if (wr_state==WR_ADDR)
      wr_running_length <= cfg_write_length;
   else if (wvalid && wready)
      wr_running_length <= wr_running_length - 1;


//--------------------------------
// fsb packet state matchine
//--------------------------------

typedef enum logic[1:0] {
   FSB_INIT = 0
   ,FSB_PILE = 1
   ,FSB_HOLD = 2
   ,FSB_SEND = 3
   } fsb_pkt_state_t;

fsb_pkt_state_t fsb_state, fsb_state_nxt;

logic fsb_piled_up;

always_comb
begin
  fsb_state_nxt = fsb_state;
  case(fsb_state)

      FSB_INIT:
      begin
        if ((wr_state==WR_ADDR) && (wr_state_nxt!=WR_ADDR) && !wr_dat_tail_flag)
        begin
          if(fsb_wvalid)
            fsb_state_nxt = FSB_PILE;
          else
            fsb_state_nxt = FSB_HOLD;
        end
        else
          fsb_state_nxt = FSB_INIT;
      end

      FSB_PILE:
      begin
        if (fsb_piled_up)  // we assume fsb pkts are multiple of DATA_FSB_NUM
          fsb_state_nxt = FSB_SEND;
        else if (!fsb_wvalid)
          fsb_state_nxt = FSB_HOLD;
        else
          fsb_state_nxt = FSB_PILE;
      end

      FSB_HOLD:
      begin
        if (fsb_wvalid)
          fsb_state_nxt = FSB_PILE;
        else
          fsb_state_nxt = FSB_HOLD;
      end

      FSB_SEND:
      begin
        if (!wready)
          fsb_state_nxt = FSB_SEND;
        else if (wr_phase_end)
          fsb_state_nxt = FSB_INIT;
        else
          fsb_state_nxt = FSB_PILE;
      end

  endcase // fsb_state
end


// wr_phase_data are new, latch wdata enable
assign wr_whole_phase_valid = (fsb_state==FSB_SEND);
assign wr_frctn_phase_valid = (fsb_state==FSB_HOLD);

logic ready_fsb;

assign ready_fsb = (fsb_state==FSB_PILE);

always_ff @(posedge clk)
   if(!sync_rst_n)
      fsb_state <= FSB_INIT;
   else
      fsb_state <= fsb_state_nxt;


//--------------------------------
// FSB control part
// this should finally support the asynchronous data transfer.
//--------------------------------

// dequeue the fsb master
assign fsb_yumi = ready_fsb && fsb_wvalid;


// count the fsb pkt number before LCM
logic [4:0] cnt_alignment;

always_ff @(posedge clk)
begin
  if (wr_state==WR_IDLE)
    cnt_alignment <= 0;
  else if (ready_fsb && fsb_wvalid)
    cnt_alignment <= cnt_alignment + 1'b1;
  // else if (reset_fsb_adapter)
  //   cnt_alignment <= 0;
end

assign fsb_piled_up = (cnt_alignment==5'd6) || (cnt_alignment==5'd12) 
          || (cnt_alignment==5'd19) || (cnt_alignment==5'd25) 
          || (cnt_alignment==5'd31);

// count the fsb pkt stored in one axi4 phase
logic [2:0] cnt_fsb_latched;

always_ff @(posedge clk)
begin
  if ((wr_state==WR_IDLE) || fsb_piled_up)
    cnt_fsb_latched <= 0;
  else if (fsb_yumi)
    cnt_fsb_latched <= cnt_fsb_latched + 1'b1;
end

// logic [2:0] cnt_fsb;

// always_ff @(posedge clk)
// begin
//   if((fsb_state==FSB_INIT) || (fsb_state==FSB_SEND))
//     cnt_fsb <= 3'b0;
//   else if(fsb_state==FSB_PILE)
//     cnt_fsb <= cnt_fsb + 1'b1;
// end

// // end of FSB_PILE
// assign fsb_piled_up = (cnt_fsb==3'd5);

// always_ff @(posedge clk)
//   if ((fsb_state==FSB_SEND))
// assign wr_phase_strb = {(DATA_WIDTH/8){1'b1}} - ({4'b1111}<<60);

// one pkt is full, send to axi4 frame

// generate wstrb signal if fsb_wvalid turns 0 abruptly 
logic [(DATA_WIDTH/8)-1:0] wr_phase_strb_comb;

genvar i;
generate
  for (i=0; i<32; i++) begin

    if (i<=6) begin
      always_comb
      begin
        if ((cnt_alignment==i))
          wr_phase_strb_comb = {(i-0){10'b11_1111_1111}} & 64'hFFFF_FFFF_FFFF_FFFF;
      end
    end

    else if (6<i<=12) begin
      always_comb
      begin
        if ((cnt_alignment==i))
          wr_phase_strb_comb = (({(i-7){10'b11_1111_1111}}<<6)|6'b11_1111) & 64'hFFFF_FFFF_FFFF_FFFF;
      end
    end

    else if (12<i<=19) begin
      always_comb
      begin
        if ((cnt_alignment==i))
          wr_phase_strb_comb = (({(i-13){10'b11_1111_1111}}<<2)|2'b11) & 64'hFFFF_FFFF_FFFF_FFFF;
      end
    end

    else if (19<i<=25) begin
      always_comb
      begin
        if ((cnt_alignment==i))
          wr_phase_strb_comb = (({(i-20){10'b11_1111_1111}}<<8)|8'b1111_1111) & 64'hFFFF_FFFF_FFFF_FFFF;
      end
    end

    else if (25<i<=31) begin
      always_comb
      begin
        if ((cnt_alignment==i))
          wr_phase_strb_comb = (({(i-26){10'b11_1111_1111}}<<4)|4'b1111) & 64'hFFFF_FFFF_FFFF_FFFF;
      end
    end
  end

endgenerate


always_ff @(posedge clk)
begin
  if ((fsb_state==FSB_PILE) && (fsb_state_nxt==FSB_HOLD))
    wr_phase_strb <= wr_phase_strb_comb;
  else
    wr_phase_strb <= 64'hFFFF_FFFF_FFFF_FFFF;
end


logic [FSB_WIDTH-1:0] wr_fsb_data_rsd;

always_ff @(posedge clk)
begin 
  if(fsb_piled_up)
    wr_fsb_data_rsd <= fsb_wdata;
end

always_ff @(posedge clk)
begin
  if (fsb_wvalid && ready_fsb)
    case(cnt_alignment)
      5'd0:   wr_phase_data[FSB_WIDTH*0+:FSB_WIDTH] <= fsb_wdata;
      5'd1:   wr_phase_data[FSB_WIDTH*1+:FSB_WIDTH] <= fsb_wdata;
      5'd2:   wr_phase_data[FSB_WIDTH*2+:FSB_WIDTH] <= fsb_wdata;
      5'd3:   wr_phase_data[FSB_WIDTH*3+:FSB_WIDTH] <= fsb_wdata;
      5'd4:   wr_phase_data[FSB_WIDTH*4+:FSB_WIDTH] <= fsb_wdata;
      5'd5:   wr_phase_data[FSB_WIDTH*5+:FSB_WIDTH] <= fsb_wdata;
      5'd6:   wr_phase_data[DATA_WIDTH-1:FSB_WIDTH*6] <= fsb_wdata[31:0];

      5'd7:   wr_phase_data[FSB_WIDTH*0+:48+FSB_WIDTH] <= {fsb_wdata, wr_fsb_data_rsd[79:32]};
      5'd8:   wr_phase_data[48+FSB_WIDTH*1+:FSB_WIDTH] <= fsb_wdata;
      5'd9:   wr_phase_data[48+FSB_WIDTH*2+:FSB_WIDTH] <= fsb_wdata;
      5'd10:  wr_phase_data[48+FSB_WIDTH*3+:FSB_WIDTH] <= fsb_wdata;
      5'd11:  wr_phase_data[48+FSB_WIDTH*4+:FSB_WIDTH] <= fsb_wdata;
      5'd12:  wr_phase_data[DATA_WIDTH-1:48+FSB_WIDTH*5] <= fsb_wdata[63:0];

      5'd13:  wr_phase_data[FSB_WIDTH*0+:16+FSB_WIDTH] <= {fsb_wdata, wr_fsb_data_rsd[79:64]};
      5'd14:  wr_phase_data[16+FSB_WIDTH*1+:FSB_WIDTH] <= fsb_wdata;
      5'd15:  wr_phase_data[16+FSB_WIDTH*2+:FSB_WIDTH] <= fsb_wdata;
      5'd16:  wr_phase_data[16+FSB_WIDTH*3+:FSB_WIDTH] <= fsb_wdata;
      5'd17:  wr_phase_data[16+FSB_WIDTH*4+:FSB_WIDTH] <= fsb_wdata;
      5'd18:  wr_phase_data[16+FSB_WIDTH*5+:FSB_WIDTH] <= fsb_wdata;
      5'd19:  wr_phase_data[DATA_WIDTH-1:16+FSB_WIDTH*6] <= fsb_wdata[15:0];

      5'd20:  wr_phase_data[FSB_WIDTH*0+:64+FSB_WIDTH] <= {fsb_wdata, wr_fsb_data_rsd[79:16]};
      5'd21:  wr_phase_data[64+FSB_WIDTH*1+:FSB_WIDTH] <= fsb_wdata;
      5'd22:  wr_phase_data[64+FSB_WIDTH*2+:FSB_WIDTH] <= fsb_wdata;
      5'd23:  wr_phase_data[64+FSB_WIDTH*3+:FSB_WIDTH] <= fsb_wdata;
      5'd24:  wr_phase_data[64+FSB_WIDTH*4+:FSB_WIDTH] <= fsb_wdata;
      5'd25:  wr_phase_data[DATA_WIDTH-1:64+FSB_WIDTH*5] <= fsb_wdata[47:0];

      5'd26:  wr_phase_data[FSB_WIDTH*0+:32+FSB_WIDTH] <= {fsb_wdata, wr_fsb_data_rsd[79:48]};
      5'd27:  wr_phase_data[32+FSB_WIDTH*1+:FSB_WIDTH] <= fsb_wdata;
      5'd28:  wr_phase_data[32+FSB_WIDTH*2+:FSB_WIDTH] <= fsb_wdata;
      5'd29:  wr_phase_data[32+FSB_WIDTH*3+:FSB_WIDTH] <= fsb_wdata;
      5'd30:  wr_phase_data[32+FSB_WIDTH*4+:FSB_WIDTH] <= fsb_wdata;
      5'd31:  wr_phase_data[DATA_WIDTH-1:32+FSB_WIDTH*5] <= fsb_wdata;
      default: begin end
    endcase
end


//--------------------------------
// AXI read state machine (to be added)
//--------------------------------
assign rd_inp = 0;


endmodule
