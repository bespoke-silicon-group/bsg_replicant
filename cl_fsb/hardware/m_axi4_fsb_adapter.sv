/**
 *  m_axi4_fsb_adapter.sv
 *
 *  cl_bsg (CL) -> axi-4 (SH)
 */

 module m_axi4_fsb_adapter 
  #(parameter DATA_WIDTH=512
    ,parameter FSB_WIDTH=80
  ) (
   input clk_i
   ,input resetn_i

   ,cfg_bus_t.master cfg_bus
   ,axi_bus_t.slave cl_sh_pcim_bus

   ,input fsb_wvalid
   ,input [FSB_WIDTH-1:0] fsb_wdata
   ,output logic fsb_yumi

   ,output logic atg_dst_sel
 );
 
localparam DATA_BYTE_NUM = DATA_WIDTH/8;


//-------------------------------------
// AXI4 register slice 
// Flop signals between CL and SH
//-------------------------------------

// axi-4 signals to below
logic [5:0] awid; 
logic [63:0] awaddr;
logic[7:0] awlen;
logic awvalid;
logic[10:0] awuser = 0; // not used
logic awready;

logic [5:0] wid; // not used
logic [DATA_WIDTH-1:0] wdata = 0;
logic [(DATA_WIDTH/8)-1:0] wstrb = 0;
logic wlast;
logic wvalid;
logic wready;

logic [5:0] bid;
logic [1:0] bresp;
logic  bvalid;
logic [17:0] buser = 0;
logic  bready;

logic [5:0] arid;
logic [63:0] araddr;
logic [7:0] arlen;
logic  arvalid;
logic [10:0] aruser = 0; // not used
logic arready;

logic [5:0] rid;
logic [DATA_WIDTH-1:0] rdata;
logic [1:0] rresp;
logic rlast;
logic rvalid;
logic [17:0] ruser = 0;
logic rready;

axi_bus_t #(.NUM_SLOTS(1),.ID_WIDTH(6),.ADDR_WIDTH(64),.DATA_WIDTH(512)) axi4_m_bus();

assign axi4_m_bus.awid = awid;
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

assign bid = axi4_m_bus.bid;
assign bresp = axi4_m_bus.bresp;
assign bvalid = axi4_m_bus.bvalid;
assign axi4_m_bus.bready = bready;

assign axi4_m_bus.arid = arid;
assign axi4_m_bus.araddr = araddr;
assign axi4_m_bus.arlen = arlen;
assign axi4_m_bus.arsize = 3'h6;
assign axi4_m_bus.arvalid = arvalid;
assign arready = axi4_m_bus.arready;

assign rid = axi4_m_bus.rid;
assign rdata = axi4_m_bus.rdata;
assign rresp = axi4_m_bus.rresp;
assign rlast = axi4_m_bus.rlast;
assign rvalid = axi4_m_bus.rvalid;
assign axi4_m_bus.rready = rready;


axi_register_slice_v2_1_17_axi_register_slice #(
    .C_FAMILY("virtexuplus"),
    .C_AXI_PROTOCOL(0),
    .C_AXI_ID_WIDTH(6),
    .C_AXI_ADDR_WIDTH(64),
    .C_AXI_DATA_WIDTH(512),
    .C_AXI_SUPPORTS_USER_SIGNALS(0),
    .C_AXI_AWUSER_WIDTH(1),
    .C_AXI_ARUSER_WIDTH(1),
    .C_AXI_WUSER_WIDTH(1),
    .C_AXI_RUSER_WIDTH(1),
    .C_AXI_BUSER_WIDTH(1),
    .C_REG_CONFIG_AW(1),
    .C_REG_CONFIG_W(1),
    .C_REG_CONFIG_B(1),
    .C_REG_CONFIG_AR(1),
    .C_REG_CONFIG_R(1),
    .C_NUM_SLR_CROSSINGS(0),
    .C_PIPELINES_MASTER_AW(0),
    .C_PIPELINES_MASTER_W(0),
    .C_PIPELINES_MASTER_B(0),
    .C_PIPELINES_MASTER_AR(0),
    .C_PIPELINES_MASTER_R(0),
    .C_PIPELINES_SLAVE_AW(0),
    .C_PIPELINES_SLAVE_W(0),
    .C_PIPELINES_SLAVE_B(0),
    .C_PIPELINES_SLAVE_AR(0),
    .C_PIPELINES_SLAVE_R(0),
    .C_PIPELINES_MIDDLE_AW(0),
    .C_PIPELINES_MIDDLE_W(0),
    .C_PIPELINES_MIDDLE_B(0),
    .C_PIPELINES_MIDDLE_AR(0),
    .C_PIPELINES_MIDDLE_R(0)
  ) inst (
    .aclk(clk_i),
    .aclk2x(1'H0),
    .aresetn(resetn_i),
    .s_axi_awid(axi4_m_bus.awid),
    .s_axi_awaddr(axi4_m_bus.awaddr),
    .s_axi_awlen(axi4_m_bus.awlen),
    .s_axi_awsize(axi4_m_bus.awsize),
    .s_axi_awburst(2'h0),
    .s_axi_awlock(1'h0),
    .s_axi_awcache(4'h0),
    .s_axi_awprot(3'h0),
    .s_axi_awregion(4'h0),
    .s_axi_awqos(4'h0),
    .s_axi_awuser(1'H0),
    .s_axi_awvalid(axi4_m_bus.awvalid),
    .s_axi_awready(axi4_m_bus.awready),
    .s_axi_wid(6'H0000),
    .s_axi_wdata(axi4_m_bus.wdata),
    .s_axi_wstrb(axi4_m_bus.wstrb),
    .s_axi_wlast(axi4_m_bus.wlast),
    .s_axi_wuser(1'H0),
    .s_axi_wvalid(axi4_m_bus.wvalid),
    .s_axi_wready(axi4_m_bus.wready),
    .s_axi_bid(axi4_m_bus.bid),
    .s_axi_bresp(axi4_m_bus.bresp),
    .s_axi_buser(),
    .s_axi_bvalid(axi4_m_bus.bvalid),
    .s_axi_bready(axi4_m_bus.bready),
    .s_axi_arid(axi4_m_bus.arid),
    .s_axi_araddr(axi4_m_bus.araddr),
    .s_axi_arlen(axi4_m_bus.arlen),
    .s_axi_arsize(axi4_m_bus.arsize),
    .s_axi_arburst(2'h0),
    .s_axi_arlock(1'h0),
    .s_axi_arcache(4'h0),
    .s_axi_arprot(3'h0),
    .s_axi_arregion(4'h0),
    .s_axi_arqos(4'h0),
    .s_axi_aruser(1'H0),
    .s_axi_arvalid(axi4_m_bus.arvalid),
    .s_axi_arready(axi4_m_bus.arready),
    .s_axi_rid(axi4_m_bus.rid),
    .s_axi_rdata(axi4_m_bus.rdata),
    .s_axi_rresp(axi4_m_bus.rresp),
    .s_axi_rlast(axi4_m_bus.rlast),
    .s_axi_ruser(),
    .s_axi_rvalid(axi4_m_bus.rvalid),
    .s_axi_rready(axi4_m_bus.rready),
    .m_axi_awid(cl_sh_pcim_bus.awid),
    .m_axi_awaddr(cl_sh_pcim_bus.awaddr),
    .m_axi_awlen(cl_sh_pcim_bus.awlen),
    .m_axi_awsize(cl_sh_pcim_bus.awsize),
    .m_axi_awburst(),
    .m_axi_awlock(),
    .m_axi_awcache(),
    .m_axi_awprot(),
    .m_axi_awregion(),
    .m_axi_awqos(),
    .m_axi_awuser(),
    .m_axi_awvalid(cl_sh_pcim_bus.awvalid),
    .m_axi_awready(cl_sh_pcim_bus.awready),
    .m_axi_wid(),
    .m_axi_wdata(cl_sh_pcim_bus.wdata),
    .m_axi_wstrb(cl_sh_pcim_bus.wstrb),
    .m_axi_wlast(cl_sh_pcim_bus.wlast),
    .m_axi_wuser(),
    .m_axi_wvalid(cl_sh_pcim_bus.wvalid),
    .m_axi_wready(cl_sh_pcim_bus.wready),
    .m_axi_bid(cl_sh_pcim_bus.bid),
    .m_axi_bresp(cl_sh_pcim_bus.bresp),
    .m_axi_buser(1'H0),
    .m_axi_bvalid(cl_sh_pcim_bus.bvalid),
    .m_axi_bready(cl_sh_pcim_bus.bready),
    .m_axi_arid(cl_sh_pcim_bus.arid),
    .m_axi_araddr(cl_sh_pcim_bus.araddr),
    .m_axi_arlen(cl_sh_pcim_bus.arlen),
    .m_axi_arsize(cl_sh_pcim_bus.arsize),
    .m_axi_arburst(),
    .m_axi_arlock(),
    .m_axi_arcache(),
    .m_axi_arprot(),
    .m_axi_arregion(),
    .m_axi_arqos(),
    .m_axi_aruser(),
    .m_axi_arvalid(cl_sh_pcim_bus.arvalid),
    .m_axi_arready(cl_sh_pcim_bus.arready),
    .m_axi_rid(cl_sh_pcim_bus.rid),
    .m_axi_rdata(cl_sh_pcim_bus.rdata),
    .m_axi_rresp(cl_sh_pcim_bus.rresp),
    .m_axi_rlast(cl_sh_pcim_bus.rlast),
    .m_axi_ruser(1'H0),
    .m_axi_rvalid(cl_sh_pcim_bus.rvalid),
    .m_axi_rready(cl_sh_pcim_bus.rready)
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
logic[5:0] rid_q = 0;
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
// 0x00:  CFG_REG
//        3 - read compare                                (not implemented yet)
//        4 - fsb_wvalid mask
// 0x08:  CNTL_REG
//        0 - wr 0/1: stop/start; rd 0/1: write is out/in process
//        1 - read control
// 0x0c:  RESET_REG
//        0 - reset write FSM to IDLE state 
//        1 - reset read                                  (not implemented yet)

// 0x20:  write start address low
// 0x24:  write start address high

// 0x28:  write end point (read head)

// 0x2c:  write length select
//        7:0 - write phases number per transaction   (only 0 is supported now)
//        15:8 - DW size to adj last data phase           (not implemented yet)
//        31:16 - user defined                            (not implemented yet)

// 0x30:  WR_BUF_SIZE
//        buffer size and the tail address

// 0x40:  read address low                                (not implemented yet)
// 0x44:  read address high                               (not implemented yet)
// 0x48:  expected read data to compare with write        (not implemented yet)
// 0x4c:  read length select                              (not implemented yet)
//        7:0 - number of the AXI read data phases
//        15:8 - last data adj, i.e. number of DW to adj last data phase
//        31:16 - user defined

// 0xe0:  DST_SEL_REG
//        0 -  0/1 to select which dst module the atg drives



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
logic cfg_wvalid_mask = 0;

logic [63:0] cfg_write_address = 0;
logic [31:0] cfg_hm_read_head = 0;
logic [31:0] cfg_buffer_size = 0;

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
         cfg_wvalid_mask <= cfg_wdata_q[4];
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
         cfg_buffer_size <= cfg_wdata_q;

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
        8'h30 : cfg_bus.rdata <= cfg_buffer_size;

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
// assign wr_last_addr = {cfg_buffer_size[31:6], 6'd0}; // must be 64 bytes aligned

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
logic [31:0] wr_next_tail;
logic wr_buffer_full;

assign wr_next_tail = wr_addr_next;
assign wr_hm_pause = wr_dat_tail_flag && wr_buffer_full;
assign wr_hm_avaliable = !wr_buffer_full;

always_ff @(posedge clk)
  if (wr_state==WR_IDLE)
    begin
      wr_buffer_full <= 0;
    end
  else if ((wr_next_tail >= (cfg_hm_read_head + wr_last_addr))
    || (((cfg_hm_read_head - wr_next_tail) <= DATA_BYTE_NUM)
      && (cfg_hm_read_head != wr_next_tail)))
  begin
    wr_buffer_full <= 1'b1;
  end
  else
    begin
      wr_buffer_full <= 0;
    end


// 2. FSB invalid
always_ff @(posedge clk)
  if (!wr_dat_tail_flag)  // pause after the tail is sent
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

logic last_write_success;

always_ff @( posedge clk)
  if (!sync_rst_n)
    begin
      last_write_success  <= 1'b1;
    end
  else if (bvalid)
    begin
      last_write_success <= 1'b1;
    end
  else if ((wr_state==WR_ADDR) && (wr_state_nxt!=WR_ADDR))
    begin
      last_write_success <= 1'b0;
    end

assign bready = 1;  // Don't do anything with BRESP


// record the bus status
logic bresp_q;
always_ff @(posedge clk)
  if (bvalid & bready)
    bresp_q = bresp[1];

// write address channel
//--------------------------------
logic [31:0] wr_addr_inc_64;

logic [63:0] wr_address;  // absolute bus address to write
logic wr_len;             // burst length

assign wr_last_addr = cfg_buffer_size - DATA_BYTE_NUM*(32'b1 + cfg_write_length);

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
                ? cfg_write_address + cfg_buffer_size
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
  else if ((wr_state==WR_ADDR) && (wr_state_nxt==WR_ADDR) && last_write_success)
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

assign fsb_v_o_masked = cfg_wvalid_mask & fsb_wvalid;

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
      wr_phase_strb_comb = {(1){16'h0000}} & 64'hFFFF_FFFF_FFFF_FFFF;
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


//--------------------------------
// AXI read state machine (to be added)
//--------------------------------
assign rd_inp = 0;


endmodule
