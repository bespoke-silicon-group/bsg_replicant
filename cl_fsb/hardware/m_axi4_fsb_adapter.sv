/**
 *  m_axi4_fsb_adapter.v
 *
 *  cl_bsg (master) -> axi-4 (slave)
 */

 module m_axi4_fsb_adapter 
  #(parameter DATA_WIDTH=512
    ,parameter NUM_RD_TAG=512
    ,parameter FSB_WIDTH=80
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
//---------------------------------------------
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
//        4 - fsb_wvalid mask
// 0x08:  (Write Only)
//        0 - write to go
//        1 - read to go
// 0x0c:
//        0 - write reset
//        1 - read reset

// 0x20:  write start address low
// 0x24:  write start address high
// 0x28:  write end point (read head)
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
logic cfg_fsb_wr_mask = 0;

logic [63:0] cfg_write_address = 0;
logic [31:0] cfg_hm_read_head = 0;
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

assign atg_dst_sel = cfg_atg_dst_sel;

always @(posedge clk)
   if (cfg_wr_stretch)
   begin
      if (cfg_addr_q==8'h0)
      begin
         cfg_rd_compare_en <= cfg_wdata_q[3];
         cfg_fsb_wr_mask <= cfg_wdata_q[4];
      end
      else if (cfg_addr_q==8'h20)
         cfg_write_address[31:0] <= cfg_wdata_q;
      else if (cfg_addr_q==8'h24)
         cfg_write_address[63:32] <= cfg_wdata_q;
      else if (cfg_addr_q==8'h28)
         cfg_hm_read_head <= cfg_wdata_q;
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
        8'h00 : cfg_bus.rdata <= {8'h0, 8'h0, 8'h0, 4'h0, cfg_rd_compare_en, 3'h0};
        8'h08 : cfg_bus.rdata <= {28'b0, bresp_q, wr_stop_pend, rd_inp, wr_inp};
        8'h20 : cfg_bus.rdata <= cfg_write_address[31:0];
        8'h24 : cfg_bus.rdata <= cfg_write_address[63:32];
        8'h28 : cfg_bus.rdata <= cfg_hm_read_head;
        8'h2c : cfg_bus.rdata <= {cfg_write_user, cfg_write_last_length, cfg_write_length};
        8'h30 : cfg_bus.rdata <= cfg_adress_offset;

        8'h40 : cfg_bus.rdata <= cfg_read_address[31:0];
        8'h44 : cfg_bus.rdata <= cfg_read_address[63:32];
        8'h48 : cfg_bus.rdata <= cfg_read_data;
        8'h4c : cfg_bus.rdata <= {cfg_read_user, cfg_read_last_length, cfg_read_length};

        8'he0 : cfg_bus.rdata <= {31'b0, cfg_atg_dst_sel};

        default : cfg_bus.rdata <= 32'hffffffff;
      endcase
  end


//--------------------------------
// AXI Write state machine      
//--------------------------------

// transaction control signals
logic wr_hm_pause;        // host memory is full and tail is updated
logic wr_hm_avaliable;    // host memory is avaliable again

logic wr_fsb_pause;       // fsb is invalid and tail is updated
logic wr_fsb_avaliable;   // fsb is avaliable again, i.e. valid=1

logic wr_soft_stop;       // stop from controller
assign wr_stop_pend = wr_soft_stop;

logic wr_dat_tail_flag;   // tail write flag, flips every frame

// address
logic [63:0] wr_addr_bus;        // current relative write address
logic [31:0] wr_last_addr;   // the last address for 64B-write


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
            if ((wr_hm_pause || wr_fsb_pause || wr_soft_stop) && wr_dat_tail_flag)  // wr_trans_done
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
        else if (wr_hm_avaliable && wr_fsb_avaliable && !wr_soft_stop)
          wr_state_nxt = WR_ADDR;
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
assign wr_inp = ((wr_state!=WR_IDLE) && (wr_state!=WR_STOP));


//--------------------------------
// AXI Write Control Signals
//--------------------------------

logic axi_whole_valid_i; // fsb pkt is packed up for one axi phase
logic axi_frctn_valid_i; // fsb pkt is partly packed and hold for valid signal

// this is left for testing, useful to terminate single buff write

// logic wr_trans_done;  // write from cfg_write_address to addr + offset, test only
// logic [31:0] wr_mem_left = 0;
// logic [31:0] wr_last_addr;
// logic [31:0] wr_mem_left_next;

// assign wr_mem_left_next = wr_mem_left - DATA_BYTE_NUM - DATA_BYTE_NUM * cfg_write_length;
// assign wr_last_addr = {cfg_adress_offset[31:6], 6'd0}; // must be 64 bytes aligned

// always_ff @(posedge clk)
//    if (wr_state==WR_IDLE)
//    begin
//       wr_mem_left <= wr_last_addr;
//    end
//    else if ((wr_state==WR_DAT) && (wr_state_nxt!=WR_DAT))
//    begin
//     if (wr_dat_tail_flag)
//     begin
//       wr_mem_left <= wr_mem_left_next;  // tail is already updated, so we can count on
//       wr_trans_done <= 1'b0;
//     end
//     else
//     begin
//       wr_mem_left <= wr_mem_left;  // tais will be send next cycle
//       wr_trans_done <= (wr_mem_left==0);
//     end
//    end


// these handles AXI stops
//--------------------------------

// 1. buffer full
// two cases when the buffer becomes full
// a): head < tail & tail >= wr_last_addr
// b): head > tail & head - tail <= 0x40
// where head stands for the read pointer at this write
// tail stands for the write pointer after this write

logic [31:0] wr_addr_next;
logic wr_buffer_full;

assign wr_hm_pause = wr_dat_tail_flag && wr_buffer_full;
assign wr_hm_avaliable = !wr_buffer_full;

always_ff @(posedge clk)
  if (wr_state==WR_IDLE)
    begin
      wr_buffer_full <= 0;
    end
  else if ((wr_addr_next >= cfg_hm_read_head + wr_last_addr)
    || (((cfg_hm_read_head - wr_addr_next) <= DATA_BYTE_NUM)
      && (cfg_hm_read_head!=wr_addr_next)))
  begin
    wr_buffer_full <= 1'b1;
  end
  else
    begin
      wr_buffer_full <= 0;
    end


// 2. FSB invalid
always_ff @(posedge clk)
  if (!wr_dat_tail_flag)  // pause until the tail is sent
    wr_fsb_pause <= axi_frctn_valid_i;  // we assume always send fraction


// 3. soft stop (take effect immediately)
always_ff @(posedge clk)
   if (!sync_rst_n)
      wr_soft_stop <= 0;
   else
      wr_soft_stop <= cfg_wr_stop || (wr_soft_stop && (wr_state_nxt!=WR_IDLE));



//--------------------------------
// AXI interface signal generator
//--------------------------------

// tail write flag
//--------------------------------
always_ff @( posedge clk)
  if ((wr_state==WR_IDLE))
    begin
      wr_dat_tail_flag <= 0;  // I choose to keep the flag when soft stop occurs
    end
  else if ((wr_state==WR_DAT) && (wr_state_nxt!=WR_DAT))
    begin
      wr_dat_tail_flag <= ~wr_dat_tail_flag;
    end


// write address channel
//--------------------------------
logic [31:0] wr_addr_inc_64;

logic [63:0] wr_address;  // absolute bus address to write
logic wr_len;             // burst length

assign wr_last_addr = {cfg_adress_offset[31:6], 6'd0};

assign wr_addr_inc_64 = (wr_addr_next==wr_last_addr) ? 0
                        : wr_addr_next + DATA_BYTE_NUM * (8'b1 + cfg_write_length);

always_ff @(posedge clk)
  if ((wr_state==WR_IDLE))
    wr_addr_next <= 0;
  else if (axi_whole_valid_i && (wr_state==WR_DAT) && (wr_state_nxt!=WR_DAT))
    wr_addr_next <= wr_addr_inc_64;
  else
    wr_addr_next <= wr_addr_next;

always_ff @(posedge clk)
   if ((wr_state==WR_IDLE))
   begin
      wr_addr_bus <= cfg_write_address;  // the start address must be 64 bytes aligned
   end
   else if ((wr_state==WR_DAT) && (wr_state_nxt!=WR_DAT))
   begin
      wr_addr_bus <= !wr_dat_tail_flag 
                ? cfg_write_address + wr_last_addr + DATA_BYTE_NUM
                : cfg_write_address + wr_addr_next;
   end

assign wr_address = wr_addr_bus;

assign wr_len = wr_dat_tail_flag ? 8'b0 : cfg_write_length;

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
    awvalid <= 1'b1;  // always avaliable
    awaddr <= wr_address;
    awid <= 0;
    awlen <= wr_len;
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


// write data channel
//--------------------------------
logic wr_phase_valid;
logic wr_phase_end;
logic wr_data_last;
logic axi_ready_o;    // axi ready signal to fsb FSM

logic [7:0] wr_running_length;

logic [DATA_WIDTH-1:0] wr_phase_data;
logic [(DATA_WIDTH/8)-1:0] wr_phase_strb;

logic [DATA_WIDTH-1:0] wr_data;
logic [(DATA_WIDTH/8)-1:0] wr_strb;

logic [31:0] wr_addr_frac_next;
logic [31:0] wr_tail_data;

assign wr_phase_valid = wr_dat_tail_flag 
                        ? (wr_state==WR_DAT) 
                        : (axi_whole_valid_i || axi_frctn_valid_i);

assign wr_tail_data = wr_fsb_pause 
                      ? wr_addr_frac_next + wr_addr_next
                      : wr_addr_next;

assign wr_data = wr_dat_tail_flag 
                ? {{(DATA_WIDTH-32){1'b1}}, wr_tail_data} 
                : wr_phase_data;

assign wr_strb = wr_dat_tail_flag ? 64'h0000_0000_0000_00FF : wr_phase_strb;

assign wr_data_last = (wr_state==WR_DAT) && (wr_state_nxt!=WR_DAT);

assign bready = 1;  // Don't do anything with BRESP

assign axi_ready_o = (wr_state==WR_ADDR) && (wr_state_nxt!=WR_ADDR) && !wr_dat_tail_flag;

// burst control
assign wr_phase_end = (wr_running_length==0);

always_ff @(posedge clk)
  if (wr_state==WR_ADDR)
    wr_running_length <= cfg_write_length;
  else if (wvalid && wready)
    wr_running_length <= wr_running_length - 1;

always_ff @(posedge clk)
  if(wr_phase_valid)
    begin
      wdata <= wr_data;
      wstrb <= wr_strb;
    end

always_ff @( posedge clk)
  if (!sync_rst_n)
    begin
      wid    <= 0;
      wvalid <= 0;
      wlast  <= 0;
    end
  else if ((wr_state==WR_DAT) && (wr_state_nxt==WR_DAT) && wr_phase_valid)
    begin
      wid    <= 0;
      wvalid <= 1'b1;
      wlast  <= wr_dat_tail_flag ? 1'b1 : wr_phase_end;
    end
  else
    begin
      wid    <= 0;
      wvalid <= 1'b0;
      wlast  <= 1'b0;
    end

// record the bus status
logic bresp_q;
always_ff @(posedge clk)
  if (bvalid & bready)
    bresp_q = bresp;



// ======================================
// fsb side
// TODO: add FIFO to maximize bandwidth
// ======================================

logic fsb_ready;
logic fsb_piled_up;
logic fsb_v_o_masked; // fsb is valid again

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

always_comb
  begin
    fsb_state_nxt = fsb_state;
    case(fsb_state)

      FSB_INIT :
        begin
          if (axi_ready_o)
            begin
              if(fsb_v_o_masked)
                fsb_state_nxt = FSB_PILE;
              else
                fsb_state_nxt = FSB_HOLD;
            end
          else
            fsb_state_nxt = FSB_INIT;
        end

      FSB_PILE :
        begin
          if (fsb_piled_up && fsb_v_o_masked)  // we assume fsb pkts are multiple of DATA_FSB_NUM
            fsb_state_nxt = FSB_SEND;
          else if (!fsb_v_o_masked)
            fsb_state_nxt = FSB_HOLD;
          else
            fsb_state_nxt = FSB_PILE;
        end

      FSB_HOLD :
        begin
          if (wr_data_last)
            fsb_state_nxt = FSB_INIT;
          else
            fsb_state_nxt = FSB_HOLD;
        end

      FSB_SEND :
        begin
          if (wr_data_last)  // only support 1 phase for now
            fsb_state_nxt = FSB_INIT;
          else
            fsb_state_nxt = FSB_SEND;
        end

    endcase // fsb_state
  end

always_ff @(posedge clk)
   if(!sync_rst_n)
      fsb_state <= FSB_INIT;
   else
      fsb_state <= fsb_state_nxt;


//--------------------------------
// FSB control part
//--------------------------------
// asynchronous data transfer should be finally supported

// fsb_v_o_masked1: 1  1  1  1  1  1  1
// fsb_piled_up   : 0  1  2  3  4  5  6| 7  7..8  9  10 11 12
// fsb_state      : P  P  P  P  P  P  P  S
// (P=fsb_ready)  : 
// fsb_yumi       : 1  1  1  1  1  1  1

assign fsb_v_o_masked = cfg_fsb_wr_mask & fsb_wvalid;

assign fsb_ready = (fsb_state==FSB_PILE);
assign axi_whole_valid_i = (fsb_state==FSB_SEND);
assign axi_frctn_valid_i = (fsb_state==FSB_HOLD);

// dequeue the fsb master
assign fsb_yumi = fsb_ready && fsb_v_o_masked;
assign wr_fsb_avaliable = fsb_v_o_masked;


// fsb pkt counter
//--------------------------------
logic [DATA_WIDTH-1:0] axi_phase_d;
assign wr_phase_data = axi_phase_d;

logic [1:0] cnt_16B;
always_ff @(posedge clk)
  if (wr_state==WR_IDLE)
    cnt_16B <= 0;
  else if (fsb_yumi)
    cnt_16B <= cnt_16B + 2'b1;

assign fsb_piled_up = (cnt_16B==2'd3) && fsb_yumi;


// strb and address generator
//--------------------------------
logic [(DATA_WIDTH/8)-1:0] wr_phase_strb_comb;
logic [31:0] wr_addr_frac_comb;

always_ff @(posedge clk)
begin
  if ((fsb_state==FSB_PILE || fsb_state==FSB_INIT)
      && (fsb_state_nxt==FSB_HOLD)) // when fsb is invalid
  begin
    wr_phase_strb <= wr_phase_strb_comb;
    wr_addr_frac_next <= wr_addr_frac_comb;
  end
  else
  begin
    wr_phase_strb <= 64'hFFFF_FFFF_FFFF_FFFF;
    wr_addr_frac_next <= wr_addr_frac_next;
  end
end

always_comb
begin
  case(cnt_16B)
    2'd0: 
    begin 
      wr_addr_frac_comb = 32'd0;
      wr_phase_strb_comb = {(0){16'hFFFF}} & 64'hFFFF_FFFF_FFFF_FFFF;
    end
    2'd1: 
    begin 
      wr_addr_frac_comb = 32'd16;
      wr_phase_strb_comb = {(1){16'hFFFF}} & 64'hFFFF_FFFF_FFFF_FFFF;
    end
    2'd2: 
    begin 
      wr_addr_frac_comb = 32'd32;
      wr_phase_strb_comb = {(2){16'hFFFF}} & 64'hFFFF_FFFF_FFFF_FFFF;
    end
    2'd3: 
    begin 
      wr_addr_frac_comb = 32'd48;
      wr_phase_strb_comb = {(3){16'hFFFF}} & 64'hFFFF_FFFF_FFFF_FFFF;
    end
    default: begin end
  endcase
end

// data register
//--------------------------------
always_ff @(posedge clk)
  if (fsb_yumi)
  case(cnt_16B)
    2'd0: axi_phase_d[128*0+:128] <= {48'd0, fsb_wdata};
    2'd1: axi_phase_d[128*1+:128] <= {48'd0, fsb_wdata};
    2'd2: axi_phase_d[128*2+:128] <= {48'd0, fsb_wdata};
    2'd3: axi_phase_d[128*3+:128] <= {48'd0, fsb_wdata};
    default : begin end
  endcase // cnt_16B


// // count the fsb pkt number before LCM
// logic [4:0] cnt_alignment;
// logic [FSB_WIDTH-1:0] wr_fsb_data_rsd;

// always_ff @(posedge clk)
// begin
//   if (wr_state==WR_IDLE)
//   begin
//     cnt_alignment <= 0;
//     wr_fsb_data_rsd <= 0;
//   end
//   else if (fsb_yumi)
//   begin
//     cnt_alignment <= cnt_alignment + 1'b1;
//     wr_fsb_data_rsd <= fsb_wdata;
//   // else if (reset_fsb_adapter)
//   //   cnt_alignment <= 0;
//   end
// end

// assign fsb_piled_up = ((cnt_alignment==5'd6) || (cnt_alignment==5'd12) 
//                     || (cnt_alignment==5'd19) || (cnt_alignment==5'd25) 
//                     || (cnt_alignment==5'd31)) && fsb_yumi;

// always_ff @(posedge clk)
// begin
//   if (fsb_yumi)
//     case(cnt_alignment)
//       5'd0:   axi_phase_d[FSB_WIDTH*0+:FSB_WIDTH] <= fsb_wdata;
//       5'd1:   axi_phase_d[FSB_WIDTH*1+:FSB_WIDTH] <= fsb_wdata;
//       5'd2:   axi_phase_d[FSB_WIDTH*2+:FSB_WIDTH] <= fsb_wdata;
//       5'd3:   axi_phase_d[FSB_WIDTH*3+:FSB_WIDTH] <= fsb_wdata;
//       5'd4:   axi_phase_d[FSB_WIDTH*4+:FSB_WIDTH] <= fsb_wdata;
//       5'd5:   axi_phase_d[FSB_WIDTH*5+:FSB_WIDTH] <= fsb_wdata;
//       5'd6:   axi_phase_d[DATA_WIDTH-1:FSB_WIDTH*6] <= fsb_wdata[31:0];

//       5'd7:   axi_phase_d[FSB_WIDTH*0+:48+FSB_WIDTH] <= {fsb_wdata, wr_fsb_data_rsd[79:32]};
//       5'd8:   axi_phase_d[48+FSB_WIDTH*1+:FSB_WIDTH] <= fsb_wdata;
//       5'd9:   axi_phase_d[48+FSB_WIDTH*2+:FSB_WIDTH] <= fsb_wdata;
//       5'd10:  axi_phase_d[48+FSB_WIDTH*3+:FSB_WIDTH] <= fsb_wdata;
//       5'd11:  axi_phase_d[48+FSB_WIDTH*4+:FSB_WIDTH] <= fsb_wdata;
//       5'd12:  axi_phase_d[DATA_WIDTH-1:48+FSB_WIDTH*5] <= fsb_wdata[63:0];

//       5'd13:  axi_phase_d[FSB_WIDTH*0+:16+FSB_WIDTH] <= {fsb_wdata, wr_fsb_data_rsd[79:64]};
//       5'd14:  axi_phase_d[16+FSB_WIDTH*1+:FSB_WIDTH] <= fsb_wdata;
//       5'd15:  axi_phase_d[16+FSB_WIDTH*2+:FSB_WIDTH] <= fsb_wdata;
//       5'd16:  axi_phase_d[16+FSB_WIDTH*3+:FSB_WIDTH] <= fsb_wdata;
//       5'd17:  axi_phase_d[16+FSB_WIDTH*4+:FSB_WIDTH] <= fsb_wdata;
//       5'd18:  axi_phase_d[16+FSB_WIDTH*5+:FSB_WIDTH] <= fsb_wdata;
//       5'd19:  axi_phase_d[DATA_WIDTH-1:16+FSB_WIDTH*6] <= fsb_wdata[15:0];

//       5'd20:  axi_phase_d[FSB_WIDTH*0+:64+FSB_WIDTH] <= {fsb_wdata, wr_fsb_data_rsd[79:16]};
//       5'd21:  axi_phase_d[64+FSB_WIDTH*1+:FSB_WIDTH] <= fsb_wdata;
//       5'd22:  axi_phase_d[64+FSB_WIDTH*2+:FSB_WIDTH] <= fsb_wdata;
//       5'd23:  axi_phase_d[64+FSB_WIDTH*3+:FSB_WIDTH] <= fsb_wdata;
//       5'd24:  axi_phase_d[64+FSB_WIDTH*4+:FSB_WIDTH] <= fsb_wdata;
//       5'd25:  axi_phase_d[DATA_WIDTH-1:64+FSB_WIDTH*5] <= fsb_wdata[47:0];

//       5'd26:  axi_phase_d[FSB_WIDTH*0+:32+FSB_WIDTH] <= {fsb_wdata, wr_fsb_data_rsd[79:48]};
//       5'd27:  axi_phase_d[32+FSB_WIDTH*1+:FSB_WIDTH] <= fsb_wdata;
//       5'd28:  axi_phase_d[32+FSB_WIDTH*2+:FSB_WIDTH] <= fsb_wdata;
//       5'd29:  axi_phase_d[32+FSB_WIDTH*3+:FSB_WIDTH] <= fsb_wdata;
//       5'd30:  axi_phase_d[32+FSB_WIDTH*4+:FSB_WIDTH] <= fsb_wdata;
//       5'd31:  axi_phase_d[DATA_WIDTH-1:32+FSB_WIDTH*5] <= fsb_wdata;
//       default: begin end
//     endcase
// end

// always_comb
// begin
//   case(cnt_alignment)
//     0:  begin
//           wr_addr_frac_comb = 32'd0;
//           wr_phase_strb_comb = {(0){10'b11_1111_1111}} & 64'hFFFF_FFFF_FFFF_FFFF; end
//     1:  begin
//           wr_addr_frac_comb = 32'd10;
//           wr_phase_strb_comb = {(1){10'b11_1111_1111}} & 64'hFFFF_FFFF_FFFF_FFFF; end
//     2:  begin
//           wr_addr_frac_comb = 32'd20;
//           wr_phase_strb_comb = {(2){10'b11_1111_1111}} & 64'hFFFF_FFFF_FFFF_FFFF; end
//     3:  begin
//           wr_addr_frac_comb = 32'd30;
//           wr_phase_strb_comb = {(3){10'b11_1111_1111}} & 64'hFFFF_FFFF_FFFF_FFFF; end
//     4:  begin
//           wr_addr_frac_comb = 32'd40;
//           wr_phase_strb_comb = {(4){10'b11_1111_1111}} & 64'hFFFF_FFFF_FFFF_FFFF; end
//     5:  begin
//           wr_addr_frac_comb = 32'd50;
//           wr_phase_strb_comb = {(5){10'b11_1111_1111}} & 64'hFFFF_FFFF_FFFF_FFFF; end
//     6:  begin
//           wr_addr_frac_comb = 32'd60;
//           wr_phase_strb_comb = {(6){10'b11_1111_1111}} & 64'hFFFF_FFFF_FFFF_FFFF; end

//     7:  begin
//           wr_addr_frac_comb = 32'd64 + 32'd6;
//           wr_phase_strb_comb =(({(7-7){10'b11_1111_1111}}<<6)|6'b11_1111) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     8:  begin
//           wr_addr_frac_comb = 32'd64 + 32'd16;
//           wr_phase_strb_comb =(({(8-7){10'b11_1111_1111}}<<6)|6'b11_1111) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     9:  begin
//           wr_addr_frac_comb = 32'd64 + 32'd26;
//           wr_phase_strb_comb =(({(9-7){10'b11_1111_1111}}<<6)|6'b11_1111) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     10: begin
//           wr_addr_frac_comb = 32'd64 + 32'd36;
//           wr_phase_strb_comb =(({(10-7){10'b11_1111_1111}}<<6)|6'b11_1111) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     11: begin
//           wr_addr_frac_comb = 32'd64 + 32'd46;
//           wr_phase_strb_comb =(({(11-7){10'b11_1111_1111}}<<6)|6'b11_1111) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     12: begin
//           wr_addr_frac_comb = 32'd64 + 32'd56;
//           wr_phase_strb_comb =(({(12-7){10'b11_1111_1111}}<<6)|6'b11_1111) & 64'hFFFF_FFFF_FFFF_FFFF; end

//     13: begin
//           wr_addr_frac_comb = 32'd64*2 + 32'd2;
//           wr_phase_strb_comb = (({(13-13){10'b11_1111_1111}}<<2)|2'b11) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     14: begin
//           wr_addr_frac_comb = 32'd64*2 + 32'd12;
//           wr_phase_strb_comb = (({(14-13){10'b11_1111_1111}}<<2)|2'b11) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     15: begin
//           wr_addr_frac_comb = 32'd64*2 + 32'd22;
//           wr_phase_strb_comb = (({(15-13){10'b11_1111_1111}}<<2)|2'b11) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     16: begin
//           wr_addr_frac_comb = 32'd64*2 + 32'd32;
//           wr_phase_strb_comb = (({(16-13){10'b11_1111_1111}}<<2)|2'b11) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     17: begin
//           wr_addr_frac_comb = 32'd64*2 + 32'd42;
//           wr_phase_strb_comb = (({(17-13){10'b11_1111_1111}}<<2)|2'b11) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     18: begin
//           wr_addr_frac_comb = 32'd64*2 + 32'd52;
//           wr_phase_strb_comb = (({(18-13){10'b11_1111_1111}}<<2)|2'b11) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     19: begin
//           wr_addr_frac_comb = 32'd64*2 + 32'd62;
//           wr_phase_strb_comb = (({(19-13){10'b11_1111_1111}}<<2)|2'b11) & 64'hFFFF_FFFF_FFFF_FFFF; end

//     20: begin
//           wr_addr_frac_comb = 32'd64*3 + 32'd8;
//           wr_phase_strb_comb = (({(20-20){10'b11_1111_1111}}<<8)|8'b1111_1111) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     21: begin
//           wr_addr_frac_comb = 32'd64*3 + 32'd18;
//           wr_phase_strb_comb = (({(21-20){10'b11_1111_1111}}<<8)|8'b1111_1111) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     22: begin
//           wr_addr_frac_comb = 32'd64*3 + 32'd28;
//           wr_phase_strb_comb = (({(22-20){10'b11_1111_1111}}<<8)|8'b1111_1111) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     23: begin
//           wr_addr_frac_comb = 32'd64*3 + 32'd38;
//           wr_phase_strb_comb = (({(23-20){10'b11_1111_1111}}<<8)|8'b1111_1111) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     24: begin
//           wr_addr_frac_comb = 32'd64*3 + 32'd48;
//           wr_phase_strb_comb = (({(24-20){10'b11_1111_1111}}<<8)|8'b1111_1111) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     25: begin
//           wr_addr_frac_comb = 32'd64*3 + 32'd58;
//           wr_phase_strb_comb = (({(25-20){10'b11_1111_1111}}<<8)|8'b1111_1111) & 64'hFFFF_FFFF_FFFF_FFFF; end

//     26: begin
//           wr_addr_frac_comb = 32'd64*4 + 32'd4;
//           wr_phase_strb_comb = (({(26-26){10'b11_1111_1111}}<<4)|4'b1111) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     27: begin
//           wr_addr_frac_comb = 32'd64*4 + 32'd14;
//           wr_phase_strb_comb = (({(27-26){10'b11_1111_1111}}<<4)|4'b1111) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     28: begin
//           wr_addr_frac_comb = 32'd64*4 + 32'd24;
//           wr_phase_strb_comb = (({(28-26){10'b11_1111_1111}}<<4)|4'b1111) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     29: begin
//           wr_addr_frac_comb = 32'd64*4 + 32'd34;
//           wr_phase_strb_comb = (({(29-26){10'b11_1111_1111}}<<4)|4'b1111) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     30: begin
//           wr_addr_frac_comb = 32'd64*4 + 32'd44;
//           wr_phase_strb_comb = (({(30-26){10'b11_1111_1111}}<<4)|4'b1111) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     31: begin
//           wr_addr_frac_comb = 32'd64*4 + 32'd54;
//           wr_phase_strb_comb = (({(31-26){10'b11_1111_1111}}<<4)|4'b1111) & 64'hFFFF_FFFF_FFFF_FFFF; end
//     default: begin end
//   endcase
// end

//--------------------------------
// AXI read state machine (to be added)
//--------------------------------
assign rd_inp = 0;


endmodule
