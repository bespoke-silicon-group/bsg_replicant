/**
 *  m_axi4_axis_adapter.sv
 *
 *  axi-stream (CL) -> axi-4 (SH)
 */

`include "bsg_axi_bus_pkg.vh"

module m_axi4_axis_adapter #(
  data_width_p = 512
  ,axi_id_width_p = 6
  ,axi_addr_width_p = 64
  ,axi_mosi_bus_width_lp = `bsg_axi_mosi_bus_width(1, axi_id_width_p, axi_addr_width_p, data_width_p)
  ,axi_miso_bus_width_lp = `bsg_axi_miso_bus_width(1, axi_id_width_p, axi_addr_width_p, data_width_p)
  ,axis_bus_width_lp = `bsg_axis_bus_width(data_width_p)
) (
  input                                    clk_i
  ,input                                    reset_i
  ,cfg_bus_t.master cfg_bus
  ,input        [axi_miso_bus_width_lp-1:0] m_axi_bus_i
  ,output       [axi_mosi_bus_width_lp-1:0] m_axi_bus_o
  // trace data in
  ,input        [    axis_bus_width_lp-1:0] s_axis_bus_i
  ,output       [    axis_bus_width_lp-1:0] s_axis_bus_o
  ,output logic                             atg_dst_sel
);

localparam data_byte_num_lp = data_width_p/8;
localparam max_burst_size_lp = 2;  // 512bit * 2

`declare_bsg_axi_bus_s(1, axi_id_width_p, axi_addr_width_p, data_width_p,
  bsg_axi_mosi_bus_s, bsg_axi_miso_bus_s);
bsg_axi_mosi_bus_s m_axi_bus_o_cast, axi_mosi_bus_r;
bsg_axi_miso_bus_s m_axi_bus_i_cast, axi_miso_bus_r;
assign m_axi_bus_i_cast = m_axi_bus_i;
assign m_axi_bus_o      = m_axi_bus_o_cast;


`declare_bsg_axis_bus_s(data_width_p, bsg_axis_mosi_bus_s, bsg_axis_miso_bus_s);
bsg_axis_mosi_bus_s s_axis_bus_i_cast;
bsg_axis_miso_bus_s s_axis_bus_o_cast;
assign s_axis_bus_i_cast = s_axis_bus_i;
assign s_axis_bus_o      = s_axis_bus_o_cast;

logic [data_width_p-1:0] axis_txd_tdata;
logic axis_txd_tvalid;
logic axis_txd_tready;

assign axis_txd_tdata = s_axis_bus_i_cast.txd_tdata;
// assign axis_txlast = axis_data_bus.txd_tlast;
assign axis_txd_tvalid = s_axis_bus_i_cast.txd_tvalid;
// assign axis_txkeep = axis_data_bus.txd_tkeep;
assign s_axis_bus_o_cast.txd_tready = axis_txd_tready;

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
logic [data_width_p-1:0] wdata = 0;
logic [(data_width_p/8)-1:0] wstrb = 0;
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
logic [data_width_p-1:0] rdata;
logic [1:0] rresp;
logic rlast;
logic rvalid;
logic [17:0] ruser = 0;
logic rready;

assign axi_mosi_bus_r.awid = awid;
assign axi_mosi_bus_r.awaddr = awaddr;
assign axi_mosi_bus_r.awlen = awlen;
assign axi_mosi_bus_r.awsize = 3'h6;
assign axi_mosi_bus_r.awvalid = awvalid;
assign awready = axi_miso_bus_r.awready;

assign axi_mosi_bus_r.wdata = wdata; 
assign axi_mosi_bus_r.wstrb = wstrb;
assign axi_mosi_bus_r.wlast = wlast;
assign axi_mosi_bus_r.wvalid = wvalid;
assign wready = axi_miso_bus_r.wready;

assign bid = axi_miso_bus_r.bid;
assign bresp = axi_miso_bus_r.bresp;
assign bvalid = axi_miso_bus_r.bvalid;
assign axi_mosi_bus_r.bready = bready;

assign axi_mosi_bus_r.arid = arid;
assign axi_mosi_bus_r.araddr = araddr;
assign axi_mosi_bus_r.arlen = arlen;
assign axi_mosi_bus_r.arsize = 3'h6;
assign axi_mosi_bus_r.arvalid = arvalid;
assign arready = axi_miso_bus_r.arready;

assign rid = axi_miso_bus_r.rid;
assign rdata = axi_miso_bus_r.rdata;
assign rresp = axi_miso_bus_r.rresp;
assign rlast = axi_miso_bus_r.rlast;
assign rvalid = axi_miso_bus_r.rvalid;
assign axi_mosi_bus_r.rready = rready;


axi_register_slice_v2_1_15_axi_register_slice #(
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
    .aresetn(~reset_i),
    .s_axi_awid(axi_mosi_bus_r.awid),
    .s_axi_awaddr(axi_mosi_bus_r.awaddr),
    .s_axi_awlen(axi_mosi_bus_r.awlen),
    .s_axi_awsize(axi_mosi_bus_r.awsize),
    .s_axi_awburst(2'h0),
    .s_axi_awlock(1'h0),
    .s_axi_awcache(4'h0),
    .s_axi_awprot(3'h0),
    .s_axi_awregion(4'h0),
    .s_axi_awqos(4'h0),
    .s_axi_awuser(1'H0),
    .s_axi_awvalid(axi_mosi_bus_r.awvalid),
    .s_axi_awready(axi_miso_bus_r.awready),
    .s_axi_wid(6'H0000),
    .s_axi_wdata(axi_mosi_bus_r.wdata),
    .s_axi_wstrb(axi_mosi_bus_r.wstrb),
    .s_axi_wlast(axi_mosi_bus_r.wlast),
    .s_axi_wuser(1'H0),
    .s_axi_wvalid(axi_mosi_bus_r.wvalid),
    .s_axi_wready(axi_miso_bus_r.wready),
    .s_axi_bid(axi_miso_bus_r.bid),
    .s_axi_bresp(axi_miso_bus_r.bresp),
    .s_axi_buser(),
    .s_axi_bvalid(axi_miso_bus_r.bvalid),
    .s_axi_bready(axi_mosi_bus_r.bready),
    .s_axi_arid(axi_mosi_bus_r.arid),
    .s_axi_araddr(axi_mosi_bus_r.araddr),
    .s_axi_arlen(axi_mosi_bus_r.arlen),
    .s_axi_arsize(axi_mosi_bus_r.arsize),
    .s_axi_arburst(2'h0),
    .s_axi_arlock(1'h0),
    .s_axi_arcache(4'h0),
    .s_axi_arprot(3'h0),
    .s_axi_arregion(4'h0),
    .s_axi_arqos(4'h0),
    .s_axi_aruser(1'H0),
    .s_axi_arvalid(axi_mosi_bus_r.arvalid),
    .s_axi_arready(axi_miso_bus_r.arready),
    .s_axi_rid(axi_miso_bus_r.rid),
    .s_axi_rdata(axi_miso_bus_r.rdata),
    .s_axi_rresp(axi_miso_bus_r.rresp),
    .s_axi_rlast(axi_miso_bus_r.rlast),
    .s_axi_ruser(),
    .s_axi_rvalid(axi_miso_bus_r.rvalid),
    .s_axi_rready(axi_mosi_bus_r.rready),
    .m_axi_awid(m_axi_bus_o_cast.awid),
    .m_axi_awaddr(m_axi_bus_o_cast.awaddr),
    .m_axi_awlen(m_axi_bus_o_cast.awlen),
    .m_axi_awsize(m_axi_bus_o_cast.awsize),
    .m_axi_awburst(),
    .m_axi_awlock(),
    .m_axi_awcache(),
    .m_axi_awprot(),
    .m_axi_awregion(),
    .m_axi_awqos(),
    .m_axi_awuser(),
    .m_axi_awvalid(m_axi_bus_o_cast.awvalid),
    .m_axi_awready(m_axi_bus_i_cast.awready),
    .m_axi_wid(),
    .m_axi_wdata(m_axi_bus_o_cast.wdata),
    .m_axi_wstrb(m_axi_bus_o_cast.wstrb),
    .m_axi_wlast(m_axi_bus_o_cast.wlast),
    .m_axi_wuser(),
    .m_axi_wvalid(m_axi_bus_o_cast.wvalid),
    .m_axi_wready(m_axi_bus_i_cast.wready),
    .m_axi_bid(m_axi_bus_i_cast.bid),
    .m_axi_bresp(m_axi_bus_i_cast.bresp),
    .m_axi_buser(1'H0),
    .m_axi_bvalid(m_axi_bus_i_cast.bvalid),
    .m_axi_bready(m_axi_bus_o_cast.bready),
    .m_axi_arid(m_axi_bus_o_cast.arid),
    .m_axi_araddr(m_axi_bus_o_cast.araddr),
    .m_axi_arlen(m_axi_bus_o_cast.arlen),
    .m_axi_arsize(m_axi_bus_o_cast.arsize),
    .m_axi_arburst(),
    .m_axi_arlock(),
    .m_axi_arcache(),
    .m_axi_arprot(),
    .m_axi_arregion(),
    .m_axi_arqos(),
    .m_axi_aruser(),
    .m_axi_arvalid(m_axi_bus_o_cast.arvalid),
    .m_axi_arready(m_axi_bus_i_cast.arready),
    .m_axi_rid(m_axi_bus_i_cast.rid),
    .m_axi_rdata(m_axi_bus_i_cast.rdata),
    .m_axi_rresp(m_axi_bus_i_cast.rresp),
    .m_axi_rlast(m_axi_bus_i_cast.rlast),
    .m_axi_ruser(1'H0),
    .m_axi_rvalid(m_axi_bus_i_cast.rvalid),
    .m_axi_rready(m_axi_bus_o_cast.rready)
  );



// Global clock
//---------------------------------------------
logic clk;
assign clk = clk_i;

// Sync reset
//---------------------------------------------
logic pre_sync_rst_n;
logic sync_rst_n;

always_ff @(negedge ~reset_i or posedge clk_i)
   if (reset_i)
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
logic[data_width_p-1:0] rdata_q = 0;
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
//        4 - axis_txvalid mask
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

logic wr_axis_pause;       // axis is invalid and tail is updated
logic wr_axis_valid;   // fsb is avaliable again, i.e. valid=1

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
            if ((wr_hm_pause || wr_axis_pause || wr_soft_stop) && wr_dat_tail_flag)  // wr_trans_done
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
        else if (wr_hm_avaliable && wr_axis_valid && !wr_soft_stop)
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
    || (((cfg_hm_read_head - wr_next_tail) <= data_byte_num_lp*(32'b1 + cfg_write_length))
      && (cfg_hm_read_head != wr_next_tail)))
  begin
    wr_buffer_full <= 1'b1;
  end
  else
    begin
      wr_buffer_full <= 0;
    end


// 2. AXIS invalid
always_ff @(posedge clk)
  if (!wr_dat_tail_flag)  // pause after the tail is sent
    wr_axis_pause <= axi_frctn_valid_i;  // we assume always send fraction


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
logic [31:0] wr_addr_inc_pkt;

logic [63:0] wr_address;  // absolute bus address to write
logic wr_len;             // burst length

logic wr_phase_valid;

assign wr_last_addr = cfg_buffer_size - data_byte_num_lp*(32'b1 + cfg_write_length);

assign wr_addr_inc_pkt = (wr_addr_next==wr_last_addr) ? 0
                        : wr_addr_next + data_byte_num_lp * (32'b1 + cfg_write_length);

always_ff @(posedge clk)
  if ((wr_state==WR_IDLE))
    wr_addr_next <= 0;
  else if (axi_whole_valid_i && (wr_state==WR_DAT) && (wr_state_nxt!=WR_DAT))
    wr_addr_next <= wr_addr_inc_pkt;
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
  else if ((wr_state==WR_ADDR) && (wr_state_nxt==WR_ADDR) && 1) //last_write_success, wr_phase_valid
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

logic wr_phase_next_end;
logic wr_data_last;
logic axi_trans_start;    // axi ready signal to axis FSM

logic [data_width_p*max_burst_size_lp-1:0] wr_data_phases;
logic [data_width_p*max_burst_size_lp/8-1:0] wr_strb_phases;

logic [data_width_p*max_burst_size_lp-1:0] wr_data;
logic [data_width_p*max_burst_size_lp/8-1:0] wr_strb;

logic [31:0] wr_addr_frac_next;
logic [31:0] wr_tail_data;

assign wr_phase_valid = wr_dat_tail_flag 
                        ? (wr_state==WR_DAT) 
                        : (axi_whole_valid_i || axi_frctn_valid_i);

assign wr_tail_data = wr_axis_pause 
                      ? wr_addr_next + wr_addr_frac_next
                      : wr_addr_next;

assign wr_data = wr_dat_tail_flag 
                ? {{(data_width_p*max_burst_size_lp-32){1'b1}}, wr_tail_data} 
                : wr_data_phases;

assign wr_strb = wr_dat_tail_flag 
                ? {{(data_width_p*max_burst_size_lp/8-32){1'b0}},32'hFF}
                : wr_strb_phases;

assign wr_data_last = (wr_state==WR_DAT) && (wr_state_nxt!=WR_DAT);

assign axi_trans_start = (wr_state==WR_ADDR) && (wr_state_nxt!=WR_ADDR) && !wr_dat_tail_flag;


// burst control
logic [2:0] t_cnt;
logic [2:0] t_cnt_next;

assign t_cnt_next = (wvalid && wready) ? (t_cnt + 1'b1) : t_cnt;
assign wr_phase_next_end = (t_cnt_next==cfg_write_length[2:0]);


always_ff @(posedge clk)
  if ((wr_state==WR_ADDR) || (t_cnt==cfg_write_length[2:0]))
    t_cnt <= 1'b0;
  else
    t_cnt <= t_cnt_next;

always_ff @(posedge clk)
  if(wr_phase_valid && ~wvalid) begin
      wdata <= wr_data[data_width_p-1:0]; 
      wstrb <= wr_strb[data_width_p/8-1:0];
  end
  else 
  if (wvalid && wready) begin
    case (t_cnt)  // only 2 burst is supported
      3'd0: 
        begin 
          wdata <= wr_data[2*data_width_p-1:data_width_p]; 
          wstrb <= wr_strb[2*data_width_p/8-1:data_width_p/8];
        end
      // 3'd1:
      //   begin 
      //     wdata <= wr_data[3*data_width_p-1:2*data_width_p]; 
      //     wstrb <= wr_strb[3*data_width_p/8-1:2*data_width_p/8];
      //   end
      // 3'd2:
      //   begin 
      //     wdata <= wr_data[4*data_width_p-1:3*data_width_p]; 
      //     wstrb <= wr_strb[4*data_width_p/8-1:3*data_width_p/8];
      //   end
      // 3'd3:
      //   begin 
      //     wdata <= wr_data[5*data_width_p-1:4*data_width_p]; 
      //     wstrb <= wr_strb[5*data_width_p/8-1:4*data_width_p/8];
      //   end
      // 3'd4:
      //   begin 
      //     wdata <= wr_data[6*data_width_p-1:5*data_width_p]; 
      //     wstrb <= wr_strb[6*data_width_p/8-1:5*data_width_p/8];
      //   end
      // 3'd5:
      //   begin 
      //     wdata <= wr_data[7*data_width_p-1:6*data_width_p]; 
      //     wstrb <= wr_strb[7*data_width_p/8-1:6*data_width_p/8];
      //   end
      // 3'd6:
      //   begin 
      //     wdata <= wr_data[8*data_width_p-1:7*data_width_p]; 
      //     wstrb <= wr_strb[8*data_width_p/8-1:7*data_width_p/8];
      //   end
      default: begin end
    endcase // t_cnt
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
      wlast  <= wr_dat_tail_flag ? 1'b1 : wr_phase_next_end;
    end
  else
    begin
      wid    <= 0;
      wvalid <= 1'b0;
      wlast  <= 1'b0;
    end
  


// ======================================
// axi stream side
// TODO: add FIFO to maximize bandwidth
// ======================================

logic [511:0] axis_txdata;
// logic axis_txlast;
logic axis_txvalid;
// logic [63:0] axis_txkeep;  // we assume this is '1
logic axis_txready;

assign axis_txdata = axis_txd_tdata;
// assign axis_txlast = axis_data_bus.txd_tlast;
assign axis_txvalid = axis_txd_tvalid;
// assign axis_txkeep = axis_data_bus.txd_tkeep;
assign axis_txd_tready = axis_txready;

logic axis_pile_ready;
logic axis_packet_ready;
logic axis_tx_wvalid; // axis is valid again

//--------------------------------
// fsb packet state matchine
//--------------------------------

typedef enum logic[1:0] {
   AXIS_INIT = 0
   ,AXIS_PILE = 1
   ,AXIS_HOLD = 2
   ,AXIS_SEND = 3
   } axis_frame_state_t;

axis_frame_state_t axis_state, axis_snext;

always_comb
  begin
    axis_snext = axis_state;
    case(axis_state)

      AXIS_INIT :
        begin
          if (axi_trans_start)
            begin
              if(axis_tx_wvalid)
                axis_snext = AXIS_PILE;
              else
                axis_snext = AXIS_HOLD;
            end
          else
            axis_snext = AXIS_INIT;
        end

      AXIS_PILE :
        begin
          if (axis_packet_ready && axis_tx_wvalid)  // we assume fsb pkts are multiple of DATA_FSB_NUM
            axis_snext = AXIS_SEND;
          else if (!axis_tx_wvalid)
            axis_snext = AXIS_HOLD;
          else
            axis_snext = AXIS_PILE;
        end

      AXIS_HOLD :
        begin
          if (wr_data_last)
            axis_snext = AXIS_INIT;
          else
            axis_snext = AXIS_HOLD;
        end

      AXIS_SEND :
        begin
          if (wr_data_last)  // only support 1 phase for now
            axis_snext = AXIS_INIT;
          else
            axis_snext = AXIS_SEND;
        end

    endcase // axis_state
  end

always_ff @(posedge clk)
   if(!sync_rst_n)
      axis_state <= AXIS_INIT;
   else
      axis_state <= axis_snext;


//--------------------------------
// AXIS control part
//--------------------------------
// asynchronous data transfer should be finally supported

assign axis_tx_wvalid = cfg_wvalid_mask & axis_txvalid;

assign axis_pile_ready = (axis_state==AXIS_PILE);
assign axi_whole_valid_i = (axis_state==AXIS_SEND);
assign axi_frctn_valid_i = (axis_state==AXIS_HOLD);

// dequeue the axis master
assign axis_txready = axis_pile_ready & axis_tx_wvalid;
assign wr_axis_valid = axis_tx_wvalid;


// AXIS pkt counter
//--------------------------------
logic [data_width_p*max_burst_size_lp-1:0] axi_phase_d;
assign wr_data_phases = axi_phase_d;

logic [2:0] cnt_phases;
assign axis_packet_ready = (cnt_phases==cfg_write_length[2:0]) && axis_txready;

always_ff @(posedge clk)
  if ((wr_state==WR_IDLE) || axis_packet_ready)
    cnt_phases <= 0;
  else if (axis_txready)
    cnt_phases <= cnt_phases + 2'b1;


// strb and address generator
//--------------------------------
logic [(data_width_p*max_burst_size_lp/8)-1:0] wr_strb_comb;
logic [31:0] wr_addr_frac_comb;

always_ff @(posedge clk)
begin
  if ((axis_state==AXIS_PILE || axis_state==AXIS_INIT)
      && (axis_snext==AXIS_HOLD)) // when AXIS is invalid
  begin
    wr_addr_frac_next <= wr_addr_frac_comb;
    wr_strb_phases <= wr_strb_comb;
  end
  else
  begin
    wr_addr_frac_next <= wr_addr_frac_next;
    wr_strb_phases <= {data_width_p/8*max_burst_size_lp{1'b1}};
  end
end

always_comb
begin
  case(cnt_phases)
    3'd0: 
    begin 
      wr_addr_frac_comb = 32'd0;
      wr_strb_comb = 64'h0000_0000_0000_0000 | {data_width_p/8*max_burst_size_lp{1'b0}};
    end
    3'd1: 
    begin 
      wr_addr_frac_comb = 32'd64;
      wr_strb_comb = {(1){64'hFFFF_FFFF_FFFF_FFFF}} | {data_width_p/8*max_burst_size_lp{1'b0}};
    end
    3'd2: 
    begin 
      wr_addr_frac_comb = 32'd128;
      wr_strb_comb = {(2){64'hFFFF_FFFF_FFFF_FFFF}} | {data_width_p/8*max_burst_size_lp{1'b0}};
    end
    3'd3: 
    begin 
      wr_addr_frac_comb = 32'd192;
      wr_strb_comb = {(3){64'hFFFF_FFFF_FFFF_FFFF}} | {data_width_p/8*max_burst_size_lp{1'b0}};
    end
    3'd4: 
    begin 
      wr_addr_frac_comb = 32'd256;
      wr_strb_comb = {(4){64'hFFFF_FFFF_FFFF_FFFF}} | {data_width_p/8*max_burst_size_lp{1'b0}};
    end
    3'd5: 
    begin 
      wr_addr_frac_comb = 32'd320;
      wr_strb_comb = {(5){64'hFFFF_FFFF_FFFF_FFFF}} | {data_width_p/8*max_burst_size_lp{1'b0}};
    end
    3'd6: 
    begin 
      wr_addr_frac_comb = 32'd384;
      wr_strb_comb = {(6){64'hFFFF_FFFF_FFFF_FFFF}} | {data_width_p/8*max_burst_size_lp{1'b0}};
    end
    3'd7: 
    begin 
      wr_addr_frac_comb = 32'd448;
      wr_strb_comb = {(7){64'hFFFF_FFFF_FFFF_FFFF}} | {data_width_p/8*max_burst_size_lp{1'b0}};
    end
    default: begin end
  endcase
end


// data register
//--------------------------------
always_ff @(posedge clk)
  if (axis_txready)
  case(cnt_phases)  // only 2 burst is supported
    3'd0: axi_phase_d[1*data_width_p-1:0*data_width_p] <= axis_txdata;
    3'd1: axi_phase_d[2*data_width_p-1:1*data_width_p] <= axis_txdata;
    // 3'd2: axi_phase_d[3*data_width_p-1:2*data_width_p] <= axis_txdata;
    // 3'd3: axi_phase_d[4*data_width_p-1:3*data_width_p] <= axis_txdata;
    // 3'd4: axi_phase_d[5*data_width_p-1:4*data_width_p] <= axis_txdata;
    // 3'd5: axi_phase_d[6*data_width_p-1:5*data_width_p] <= axis_txdata;
    // 3'd6: axi_phase_d[7*data_width_p-1:6*data_width_p] <= axis_txdata;
    // 3'd7: axi_phase_d[8*data_width_p-1:7*data_width_p] <= axis_txdata;
    default : begin end
  endcase // cnt_phases


//--------------------------------
// AXI read state machine (to be added)
//--------------------------------
assign rd_inp = 0;

endmodule // the
