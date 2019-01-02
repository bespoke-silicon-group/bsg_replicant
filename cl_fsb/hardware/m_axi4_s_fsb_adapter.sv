/**
 *  m_axi4_s_fsb_adapter.sv
 *
 *  cl_bsg (CL) -> axi-4 (SH)
 */

`include "bsg_axi_bus_pkg.vh"

 module m_axi4_s_fsb_adapter 
  #(
    fsb_width_p=80
    ,data_width_p=512
  ,axi_id_width_p = 6
  ,axi_addr_width_p = 64
  ,axil_base_addr_p = 32'h0000_1000
  ,axil_base_width_p = 12
  ,axil_mosi_bus_width_lp = `bsg_axil_mosi_bus_width(1)
  ,axil_miso_bus_width_lp = `bsg_axil_miso_bus_width(1)
  ,axi_mosi_bus_width_lp = `bsg_axi_mosi_bus_width(1, axi_id_width_p, axi_addr_width_p, data_width_p)
  ,axi_miso_bus_width_lp = `bsg_axi_miso_bus_width(1, axi_id_width_p, axi_addr_width_p, data_width_p)
  ,axis_bus_width_lp = `bsg_axis_bus_width(data_width_p)
  ) (
   input clk_i
   ,input reset_i
  ,input [axil_mosi_bus_width_lp-1:0] s_axil_bus_i
  ,output [axil_miso_bus_width_lp-1:0] s_axil_bus_o
  ,input        [axi_miso_bus_width_lp-1:0] m_axi_bus_i
  ,output       [axi_mosi_bus_width_lp-1:0] m_axi_bus_o
   ,input fsb_wvalid
   ,input [fsb_width_p-1:0] fsb_wdata
   ,output logic fsb_yumi

   ,output logic atg_dst_sel
 );
 
localparam data_byte_num_lp = data_width_p/8;

`declare_bsg_axil_bus_s(1, bsg_axil_mosi_bus_s, bsg_axil_miso_bus_s);
bsg_axil_mosi_bus_s s_axil_bus_i_cast, axil_mosi_bus_r;
bsg_axil_miso_bus_s s_axil_bus_o_cast, axil_miso_bus_r;
assign s_axil_bus_i_cast = s_axil_bus_i;
assign s_axil_bus_o = s_axil_bus_o_cast;

`declare_bsg_axi_bus_s(1, axi_id_width_p, axi_addr_width_p, data_width_p,
  bsg_axi_mosi_bus_s, bsg_axi_miso_bus_s);
bsg_axi_mosi_bus_s m_axi_bus_o_cast, axi_mosi_bus_r;
bsg_axi_miso_bus_s m_axi_bus_i_cast, axi_miso_bus_r;
assign m_axi_bus_i_cast = m_axi_bus_i;
assign m_axi_bus_o      = m_axi_bus_o_cast;


//-------------------------------------
// Flop signals between CL and SH
//-------------------------------------
axi_register_slice_light axil_register_slice (
  .aclk         (clk_i                    ),
  .aresetn      (~reset_i                 ),
  .s_axi_awaddr (s_axil_bus_i_cast.awaddr ),
  .s_axi_awprot (3'h0                     ),
  .s_axi_awvalid(s_axil_bus_i_cast.awvalid),
  .s_axi_awready(s_axil_bus_o_cast.awready),
  .s_axi_wdata  (s_axil_bus_i_cast.wdata  ),
  .s_axi_wstrb  (s_axil_bus_i_cast.wstrb  ),
  .s_axi_wvalid (s_axil_bus_i_cast.wvalid ),
  .s_axi_wready (s_axil_bus_o_cast.wready ),
  .s_axi_bresp  (s_axil_bus_o_cast.bresp  ),
  .s_axi_bvalid (s_axil_bus_o_cast.bvalid ),
  .s_axi_bready (s_axil_bus_i_cast.bready ),
  .s_axi_araddr (s_axil_bus_i_cast.araddr ),
  .s_axi_arvalid(s_axil_bus_i_cast.arvalid),
  .s_axi_arready(s_axil_bus_o_cast.arready),
  .s_axi_rdata  (s_axil_bus_o_cast.rdata  ),
  .s_axi_rresp  (s_axil_bus_o_cast.rresp  ),
  .s_axi_rvalid (s_axil_bus_o_cast.rvalid ),
  .s_axi_rready (s_axil_bus_i_cast.rready ),
  
  .m_axi_awaddr (axil_mosi_bus_r.awaddr    ),
  .m_axi_awprot (                         ),
  .m_axi_awvalid(axil_mosi_bus_r.awvalid   ),
  .m_axi_awready(axil_miso_bus_r.awready   ),
  .m_axi_wdata  (axil_mosi_bus_r.wdata     ),
  .m_axi_wstrb  (axil_mosi_bus_r.wstrb     ),
  .m_axi_wvalid (axil_mosi_bus_r.wvalid    ),
  .m_axi_wready (axil_miso_bus_r.wready    ),
  .m_axi_bresp  (axil_miso_bus_r.bresp     ),
  .m_axi_bvalid (axil_miso_bus_r.bvalid    ),
  .m_axi_bready (axil_mosi_bus_r.bready    ),
  .m_axi_araddr (axil_mosi_bus_r.araddr    ),
  .m_axi_arvalid(axil_mosi_bus_r.arvalid   ),
  .m_axi_arready(axil_miso_bus_r.arready   ),
  .m_axi_rdata  (axil_miso_bus_r.rdata     ),
  .m_axi_rresp  (axil_miso_bus_r.rresp     ),
  .m_axi_rvalid (axil_miso_bus_r.rvalid    ),
  .m_axi_rready (axil_mosi_bus_r.rready    )
);


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


// Sync reset
//---------------------------------------------
// logic pre_sync_rst_n;
logic sync_rst_n;

assign sync_rst_n =  ~reset_i;
// always_ff @(negedge (~reset_i) or posedge clk_i)
// begin
//    if (reset_i)
//    begin
//       pre_sync_rst_n <= 0;
//       sync_rst_n <= 0;
//    end
//    else
//    begin
//       pre_sync_rst_n <= 1;
//       sync_rst_n <= pre_sync_rst_n;
//    end
//  end

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

typedef enum logic[2:0] {
   SLV_IDLE = 0,
   SLV_WR_ADDR = 1,
   SLV_CYC = 2,
   SLV_RESP = 3
   } axi_cfg_state_e;

axi_cfg_state_e slv_state, slv_state_nxt;

logic slv_wr_req, slv_rd_req;
logic slv_cyc_done;
logic slv_mx_rsp_ready;

//State machine
always_comb
begin
   slv_state_nxt = slv_state;
   if (reset_i)
      slv_state_nxt = SLV_IDLE;
   else
   begin
   case (slv_state)

      SLV_IDLE:
      begin
         if (slv_wr_req)
            slv_state_nxt = SLV_WR_ADDR;
         else if (slv_rd_req)
            slv_state_nxt = SLV_CYC;
         else
            slv_state_nxt = SLV_IDLE;
      end

      SLV_WR_ADDR:
      begin
         slv_state_nxt = SLV_CYC;
      end

      SLV_CYC:
      begin
         if (slv_cyc_done)
            slv_state_nxt = SLV_RESP;
         else
            slv_state_nxt = SLV_CYC;
      end

      SLV_RESP:
      begin
         if (slv_mx_rsp_ready)
            slv_state_nxt = SLV_IDLE;
         else
            slv_state_nxt = SLV_RESP;
      end

   endcase
   end
end

//State machine flops
always_ff @(negedge sync_rst_n or posedge clk_i)
   if (!sync_rst_n)
      slv_state <= SLV_IDLE;
   else
      slv_state <= slv_state_nxt;


// input signals from master

// w/r start request, addr valid
assign slv_wr_req = axil_mosi_bus_r.awvalid;
assign slv_rd_req = axil_mosi_bus_r.arvalid;

// select the wr cycle
logic slv_cyc_wr; 
always_ff @(negedge sync_rst_n or posedge clk_i)
   if (!sync_rst_n)
      slv_cyc_wr <= 0;
   else if (slv_state==SLV_IDLE)
      slv_cyc_wr <= slv_wr_req;

// latch the w/r address
logic [31:0] slv_req_rd_addr;
logic [31:0] slv_req_wr_addr;
logic [31:0] base_addr_cast;

always_ff @(negedge sync_rst_n or posedge clk_i)
  if (!sync_rst_n)
  begin
    {slv_req_rd_addr, slv_req_wr_addr} <= 64'd0;
    base_addr_cast <= axil_base_addr_p;
  end
  else if ((slv_state == SLV_IDLE) && slv_wr_req)
  begin
    slv_req_wr_addr <= axil_mosi_bus_r.awaddr;
  end
  else if ((slv_state == SLV_IDLE) && slv_rd_req)
  begin
    slv_req_rd_addr <= axil_mosi_bus_r.araddr;
  end

logic [31:0] slv_mx_addr;
assign slv_mx_addr = (slv_cyc_wr)? slv_req_wr_addr : slv_req_rd_addr;

// w|r data ready, bready|rready
assign slv_mx_rsp_ready = (slv_cyc_wr) ? axil_mosi_bus_r.bready : axil_mosi_bus_r.rready;

// w|r data valid, wvalid| read is always valid from slave
logic slv_mx_req_valid;
assign slv_mx_req_valid = (slv_cyc_wr)? axil_mosi_bus_r.wvalid: 1'b1;

// input signal from slave
logic base_addr_eq;
assign base_addr_eq = (slv_mx_addr[axil_base_width_p+:4]==base_addr_cast[axil_base_width_p+:4]);


logic cfg_data_ack;
assign slv_cyc_done = base_addr_eq ? cfg_data_ack : 1'b1;


// output signal to slave

// cfg address & data
logic [7:0] cfg_addr;
logic [31:0] cfg_wdata;
always_ff @(negedge sync_rst_n or posedge clk_i)
   if (!sync_rst_n)
   begin
      cfg_addr <= '{default:'0};
      cfg_wdata <= '{default:'0};
   end
   else
   begin
     cfg_addr <= slv_mx_addr[7:0];
     cfg_wdata <= axil_mosi_bus_r.wdata;
   end

// cfg write & read enable signals, generate 1 clock pulse
logic cfg_wen, cfg_ren, slv_did_req;
always_ff @(negedge sync_rst_n or posedge clk_i)
  if (!sync_rst_n)
    begin
      cfg_wen <= 0;
      cfg_ren <= 0;
    end
  else
    begin
      cfg_wen <= base_addr_eq ? ((slv_state==SLV_CYC) & slv_mx_req_valid & slv_cyc_wr & !slv_did_req)
        : 0;
      cfg_ren <= base_addr_eq ? ((slv_state==SLV_CYC) & slv_mx_req_valid & !slv_cyc_wr & !slv_did_req)
        : 0;
    end

always_ff @(negedge sync_rst_n or posedge clk_i)
  if (!sync_rst_n)
    begin
      slv_did_req <= 0;
    end
  else if (slv_state==SLV_IDLE)
    begin
      slv_did_req <= 0;
    end
  else if (cfg_wen || cfg_ren)
    begin
      slv_did_req <= 1;
    end


// latch the return data
logic [31:0] cfg_rdata;
always_ff @(negedge sync_rst_n or posedge clk_i)
   if (!sync_rst_n)
      axil_miso_bus_r.rdata <= 0;
   else if (slv_cyc_done)
      axil_miso_bus_r.rdata <= base_addr_eq ? cfg_rdata : 32'hdead_beef;

// ready back to axil for request
always_ff @(negedge sync_rst_n or posedge clk_i)
  if (!sync_rst_n)
    begin
      axil_miso_bus_r.awready <= 0;
      axil_miso_bus_r.wready <= 0;
      axil_miso_bus_r.arready <= 0;
    end
  else
    begin
      axil_miso_bus_r.awready <= (slv_state_nxt==SLV_WR_ADDR);
      axil_miso_bus_r.wready <= ((slv_state==SLV_CYC) && (slv_state_nxt!=SLV_CYC)) && slv_cyc_wr;
      axil_miso_bus_r.arready <= ((slv_state==SLV_CYC) && (slv_state_nxt!=SLV_CYC)) && ~slv_cyc_wr;
    end


// response back to axil
assign axil_miso_bus_r.bresp = 2'b00;
assign axil_miso_bus_r.bvalid = (slv_state==SLV_RESP) && slv_cyc_wr;

assign axil_miso_bus_r.rresp = 2'b00;
assign axil_miso_bus_r.rvalid = (slv_state==SLV_RESP) && !slv_cyc_wr;


logic cfg_wr_stretch, cfg_rd_stretch;
logic[7:0] cfg_addr_q;
logic[31:0] cfg_wdata_q;

always @(posedge clk_i)
   if (!sync_rst_n)
   begin
      cfg_wr_stretch <= 0;
      cfg_rd_stretch <= 0;
   end
   else 
   begin
      cfg_wr_stretch <= cfg_wen || (cfg_wr_stretch && !cfg_data_ack);
      cfg_rd_stretch <= cfg_ren || (cfg_rd_stretch && !cfg_data_ack);
      if (cfg_wen||cfg_ren)
      begin
         cfg_addr_q <= cfg_addr;
         cfg_wdata_q <= cfg_wdata;
      end
   end


// Ack for cycle
always_ff @(posedge clk_i)
   if (!sync_rst_n)
      cfg_data_ack <= 0;
   else
      cfg_data_ack <= ((cfg_wr_stretch||cfg_rd_stretch) && !cfg_data_ack); 


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

always @(posedge clk_i)
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

assign cfg_wr_go = (cfg_wr_stretch && cfg_data_ack && (cfg_addr_q==8'h08) && cfg_wdata_q[0]) && !wr_inp;
assign cfg_rd_go = (cfg_wr_stretch && cfg_data_ack && (cfg_addr_q==8'h08) && cfg_wdata_q[1]) && !rd_inp;

assign cfg_wr_stop = (cfg_wr_stretch && cfg_data_ack && (cfg_addr_q==8'h08) && ~cfg_wdata_q[0]);
assign cfg_rd_stop = (cfg_wr_stretch && cfg_data_ack && (cfg_addr_q==8'h08) && ~cfg_wdata_q[1]);

assign cfg_write_reset = (cfg_wr_stretch && cfg_data_ack && (cfg_addr_q==8'h0c) && cfg_wdata_q[0]);
assign cfg_read_reset = (cfg_wr_stretch && cfg_data_ack && (cfg_addr_q==8'h0c) && cfg_wdata_q[1]);

// Readback mux
always_ff @(posedge clk_i)
  begin
    if (cfg_wr_stretch)
      case (cfg_addr_q)
        8'h00 : cfg_rdata <= {8'h0, 8'h0, 8'h0, 4'h0, cfg_rd_compare_en, 3'h0};
        8'h08 : cfg_rdata <= {28'b0, bresp_q, wr_stop_pend, rd_inp, wr_inp};
        8'h20 : cfg_rdata <= cfg_write_address[31:0];
        8'h24 : cfg_rdata <= cfg_write_address[63:32];
        8'h28 : cfg_rdata <= cfg_hm_read_head;
        8'h2c : cfg_rdata <= {cfg_write_user, cfg_write_last_length, cfg_write_length};
        8'h30 : cfg_rdata <= cfg_buffer_size;

        8'h40 : cfg_rdata <= cfg_read_address[31:0];
        8'h44 : cfg_rdata <= cfg_read_address[63:32];
        8'h48 : cfg_rdata <= cfg_read_data;
        8'h4c : cfg_rdata <= {cfg_read_user, cfg_read_last_length, cfg_read_length};

        8'he0 : cfg_rdata <= {31'b0, cfg_atg_dst_sel};

        default : cfg_rdata <= 32'hffffffff;
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

always_ff @(posedge clk_i)
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

// assign wr_mem_left_next = wr_mem_left - data_byte_num_lp - data_byte_num_lp * cfg_write_length;
// assign wr_last_addr = {cfg_buffer_size[31:6], 6'd0}; // must be 64 bytes aligned

// always_ff @(posedge clk_i)
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

always_ff @(posedge clk_i)
  if (wr_state==WR_IDLE)
    begin
      wr_buffer_full <= 0;
    end
  else if ((wr_next_tail >= (cfg_hm_read_head + wr_last_addr))
    || (((cfg_hm_read_head - wr_next_tail) <= data_byte_num_lp)
      && (cfg_hm_read_head != wr_next_tail)))
  begin
    wr_buffer_full <= 1'b1;
  end
  else
    begin
      wr_buffer_full <= 0;
    end


// 2. FSB invalid
always_ff @(posedge clk_i)
  if (!wr_dat_tail_flag)  // pause after the tail is sent
    wr_fsb_pause <= axi_frctn_valid_i;  // we assume always send fraction


// 3. soft stop (take effect immediately)
always_ff @(posedge clk_i)
   if (!sync_rst_n)
      wr_soft_stop <= 0;
   else
      wr_soft_stop <= cfg_wr_stop || (wr_soft_stop && (wr_state_nxt!=WR_IDLE));



//--------------------------------
// AXI interface signal generator
//--------------------------------

// tail write flag
//--------------------------------
always_ff @( posedge clk_i)
  if ((wr_state==WR_IDLE))
    begin
      wr_dat_tail_flag <= 0;  // I choose to keep the flag when soft stop occurs
    end
  else if ((wr_state==WR_DAT) && (wr_state_nxt!=WR_DAT))
    begin
      wr_dat_tail_flag <= ~wr_dat_tail_flag;
    end

logic last_write_success;

always_ff @( posedge clk_i)
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
always_ff @(posedge clk_i)
  if (bvalid & bready)
    bresp_q = bresp[1];

// write address channel
//--------------------------------
logic [31:0] wr_addr_inc_64;

logic [63:0] wr_address;  // absolute bus address to write
logic wr_len;             // burst length

assign wr_last_addr = cfg_buffer_size - data_byte_num_lp*(32'b1 + cfg_write_length);

assign wr_addr_inc_64 = (wr_addr_next==wr_last_addr) ? 0
                        : wr_addr_next + data_byte_num_lp * (8'b1 + cfg_write_length);

always_ff @(posedge clk_i)
  if ((wr_state==WR_IDLE))
    wr_addr_next <= 0;
  else if (axi_whole_valid_i && (wr_state==WR_DAT) && (wr_state_nxt!=WR_DAT))
    wr_addr_next <= wr_addr_inc_64;
  else
    wr_addr_next <= wr_addr_next;

always_ff @(posedge clk_i)
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

always_ff @( posedge clk_i)
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

logic [data_width_p-1:0] wr_phase_data;
logic [(data_width_p/8)-1:0] wr_phase_strb;

logic [data_width_p-1:0] wr_data;
logic [(data_width_p/8)-1:0] wr_strb;

logic [31:0] wr_addr_frac_next;
logic [31:0] wr_tail_data;

assign wr_phase_valid = wr_dat_tail_flag 
                        ? (wr_state==WR_DAT) 
                        : (axi_whole_valid_i || axi_frctn_valid_i);

assign wr_tail_data = wr_fsb_pause 
                      ? wr_addr_frac_next + wr_addr_next
                      : wr_addr_next;

assign wr_data = wr_dat_tail_flag 
                ? {{(data_width_p-32){1'b1}}, wr_tail_data} 
                : wr_phase_data;

assign wr_strb = wr_dat_tail_flag ? 64'h0000_0000_0000_00FF : wr_phase_strb;

assign wr_data_last = (wr_state==WR_DAT) && (wr_state_nxt!=WR_DAT);

assign axi_ready_o = (wr_state==WR_ADDR) && (wr_state_nxt!=WR_ADDR) && !wr_dat_tail_flag;

// burst control
assign wr_phase_end = (wr_running_length==0);

always_ff @(posedge clk_i)
  if (wr_state==WR_ADDR)
    wr_running_length <= cfg_write_length;
  else if (wvalid && wready)
    wr_running_length <= wr_running_length - 1;

always_ff @(posedge clk_i)
  if(wr_phase_valid)
    begin
      wdata <= wr_data;
      wstrb <= wr_strb;
    end

always_ff @( posedge clk_i)
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

always_ff @(posedge clk_i)
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
logic [data_width_p-1:0] axi_phase_d;
assign wr_phase_data = axi_phase_d;

logic [1:0] cnt_16B;
always_ff @(posedge clk_i)
  if (wr_state==WR_IDLE)
    cnt_16B <= 0;
  else if (fsb_yumi)
    cnt_16B <= cnt_16B + 2'b1;

assign fsb_piled_up = (cnt_16B==2'd3) && fsb_yumi;


// strb and address generator
//--------------------------------
logic [(data_width_p/8)-1:0] wr_phase_strb_comb;
logic [31:0] wr_addr_frac_comb;

always_ff @(posedge clk_i)
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
always_ff @(posedge clk_i)
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
