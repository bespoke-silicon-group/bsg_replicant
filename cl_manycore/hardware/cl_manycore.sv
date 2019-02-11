module cl_manycore
(
   `include "cl_ports.vh"
);

// For some silly reason, you need to leave this up here...
logic rst_main_n_sync;

`include "bsg_defines.v"
`include "bsg_manycore_packet.vh"
`include "cl_id_defines.vh"
`include "cl_defines.vh"

//--------------------------------------------
// Start with Tie-Off of Unused Interfaces
//---------------------------------------------
// The developer should use the next set of `include to properly tie-off any
// unused interface The list is put in the top of the module to avoid cases
// where developer may forget to remove it from the end of the file

`include "unused_flr_template.inc"
`include "unused_ddr_a_b_d_template.inc"
//`include "unused_ddr_c_template.inc"
`include "unused_pcim_template.inc"
//`include "unused_dma_pcis_template.inc"
`include "unused_cl_sda_template.inc"
`include "unused_sh_bar1_template.inc"
`include "unused_apppf_irq_template.inc"

//-------------------------------------------------
// Wires
//-------------------------------------------------
logic pre_sync_rst_n;

logic [15:0] vled_q;
logic [15:0] pre_cl_sh_status_vled;
logic [15:0] sh_cl_status_vdip_q;
logic [15:0] sh_cl_status_vdip_q2;

//-------------------------------------------------
// PCI ID Values
//-------------------------------------------------
assign cl_sh_id0[31:0] = `CL_SH_ID0;
assign cl_sh_id1[31:0] = `CL_SH_ID1;

//-------------------------------------------------
// Reset Synchronization
//-------------------------------------------------

always_ff @(negedge rst_main_n or posedge clk_main_a0)
   if (!rst_main_n)
   begin
      pre_sync_rst_n  <= 0;
      rst_main_n_sync <= 0;
   end
   else
   begin
      pre_sync_rst_n  <= 1;
      rst_main_n_sync <= pre_sync_rst_n;
   end

//-------------------------------------------------
// Virtual LED Register
//-------------------------------------------------
// Flop/synchronize interface signals
always_ff @(posedge clk_main_a0)
   if (!rst_main_n_sync) begin
      sh_cl_status_vdip_q[15:0]  <= 16'h0000;
      sh_cl_status_vdip_q2[15:0] <= 16'h0000;
      cl_sh_status_vled[15:0]    <= 16'h0000;
   end
   else begin
      sh_cl_status_vdip_q[15:0]  <= sh_cl_status_vdip[15:0];
      sh_cl_status_vdip_q2[15:0] <= sh_cl_status_vdip_q[15:0];
      cl_sh_status_vled[15:0]    <= pre_cl_sh_status_vled[15:0];
   end

// The register contains 16 read-only bits corresponding to 16 LED's.
// The same LED values can be read from the CL to Shell interface
// by using the linux FPGA tool: $ fpga-get-virtual-led -S 0
always_ff @(posedge clk_main_a0)
   if (!rst_main_n_sync) begin
      vled_q[15:0] <= 16'h0000;
   end
   else begin
      vled_q[15:0] <= 16'hbeef;
   end

assign pre_cl_sh_status_vled[15:0] = vled_q[15:0];
assign cl_sh_status0[31:0] = 32'h0;
assign cl_sh_status1[31:0] = 32'h0;

//-------------------------------------------------
// Post-Pipeline-Register OCL AXI-L Signals
//-------------------------------------------------
logic        m_axil_ocl_awvalid;
logic [31:0] m_axil_ocl_awaddr;
logic        m_axil_ocl_awready;

logic        m_axil_ocl_wvalid;
logic [31:0] m_axil_ocl_wdata;
logic [ 3:0] m_axil_ocl_wstrb;
logic        m_axil_ocl_wready;

logic        m_axil_ocl_bvalid;
logic [ 1:0] m_axil_ocl_bresp;
logic        m_axil_ocl_bready;

logic        m_axil_ocl_arvalid;
logic [31:0] m_axil_ocl_araddr;
logic        m_axil_ocl_arready;

logic        m_axil_ocl_rvalid;
logic [31:0] m_axil_ocl_rdata;
logic [ 1:0] m_axil_ocl_rresp;
logic        m_axil_ocl_rready;

//--------------------------------------------
// AXI4 signals for the Manycore
//---------------------------------------------
logic [5:0] m_axi4_manycore_awid;
logic [63:0] m_axi4_manycore_awaddr;
logic [7:0] m_axi4_manycore_awlen;
logic [2:0] m_axi4_manycore_awsize;
logic [1:0] m_axi4_manycore_awburst;
logic [0:0] m_axi4_manycore_awlock;
logic [3:0] m_axi4_manycore_awcache;
logic [2:0] m_axi4_manycore_awprot;
logic [3:0] m_axi4_manycore_awregion;
logic [3:0] m_axi4_manycore_awqos;
logic m_axi4_manycore_awvalid;
logic m_axi4_manycore_awready;

logic [511:0] m_axi4_manycore_wdata;
logic [63:0] m_axi4_manycore_wstrb;
logic m_axi4_manycore_wlast;
logic m_axi4_manycore_wvalid;
logic m_axi4_manycore_wready;

logic [5:0] m_axi4_manycore_bid;
logic [1:0] m_axi4_manycore_bresp;
logic m_axi4_manycore_bvalid;
logic m_axi4_manycore_bready;

logic [5:0] m_axi4_manycore_arid;
logic [63:0] m_axi4_manycore_araddr;
logic [7:0] m_axi4_manycore_arlen;
logic [2:0] m_axi4_manycore_arsize;
logic [1:0] m_axi4_manycore_arburst;
logic [0:0] m_axi4_manycore_arlock;
logic [3:0] m_axi4_manycore_arcache;
logic [2:0] m_axi4_manycore_arprot;
logic [3:0] m_axi4_manycore_arregion;
logic [3:0] m_axi4_manycore_arqos;
logic m_axi4_manycore_arvalid;
logic m_axi4_manycore_arready;

logic [5:0] m_axi4_manycore_rid;
logic [511:0] m_axi4_manycore_rdata;
logic [1:0] m_axi4_manycore_rresp;
logic m_axi4_manycore_rlast;
logic m_axi4_manycore_rvalid;
logic m_axi4_manycore_rready;

//--------------------------------------------
// Concatenated Signals
//---------------------------------------------
logic [2*6-1:0] m_axi4_concat_awid;
logic [2*64-1:0] m_axi4_concat_awaddr;
logic [2*8-1:0] m_axi4_concat_awlen;
logic [2*3-1:0] m_axi4_concat_awsize;
logic [2*2-1:0] m_axi4_concat_awburst;
logic [2*1-1:0] m_axi4_concat_awlock;
logic [2*3-1:0] m_axi4_concat_awcache;
logic [2*2-1:0] m_axi4_concat_awprot;
logic [2*3-1:0] m_axi4_concat_awregion;
logic [2*3-1:0] m_axi4_concat_awqos;
logic [2*1-1:0] m_axi4_concat_awvalid;
logic [2*1-1:0] m_axi4_concat_awready;

logic [2*512-1:0] m_axi4_concat_wdata;
logic [2*64-1:0] m_axi4_concat_wstrb;
logic [2*1-1:0] m_axi4_concat_wlast;
logic [2*1-1:0] m_axi4_concat_wvalid;
logic [2*1-1:0] m_axi4_concat_wready;

logic [2*6-1:0] m_axi4_concat_bid;
logic [2*2-1:0] m_axi4_concat_bresp;
logic [2*1-1:0] m_axi4_concat_bvalid;
logic [2*1-1:0] m_axi4_concat_bready;

logic [2*6-1:0] m_axi4_concat_arid;
logic [2*64-1:0] m_axi4_concat_araddr;
logic [2*8-1:0] m_axi4_concat_arlen;
logic [2*3-1:0] m_axi4_concat_arsize;
logic [2*2-1:0] m_axi4_concat_arburst;
logic [2*1-1:0] m_axi4_concat_arlock;
logic [2*3-1:0] m_axi4_concat_arcache;
logic [2*2-1:0] m_axi4_concat_arprot;
logic [2*3-1:0] m_axi4_concat_arregion;
logic [2*3-1:0] m_axi4_concat_arqos;
logic [2*1-1:0] m_axi4_concat_arvalid;
logic [2*1-1:0] m_axi4_concat_arready;

logic [2*6-1:0] m_axi4_concat_rid;
logic [2*512-1:0] m_axi4_concat_rdata;
logic [2*2-1:0] m_axi4_concat_rresp;
logic [2*1-1:0] m_axi4_concat_rlast;
logic [2*1-1:0] m_axi4_concat_rvalid;
logic [2*1-1:0] m_axi4_concat_rready;

//--------------------------------------------
// AXI4 DMA/Manycore System
//---------------------------------------------

//--------------------------------------------
// Concatenate Manycore and DMA Signals for the Crossbar
//---------------------------------------------
assign {m_axi4_manycore_rid, cl_sh_dma_pcis_rid} = m_axi4_concat_rid;
assign {m_axi4_manycore_rdata, cl_sh_dma_pcis_rdata} = m_axi4_concat_rdata;
assign {m_axi4_manycore_rresp, cl_sh_dma_pcis_rresp} = m_axi4_concat_rresp;
assign {m_axi4_manycore_rlast, cl_sh_dma_pcis_rlast} = m_axi4_concat_rlast;
assign {m_axi4_manycore_rvalid, cl_sh_dma_pcis_rvalid} = m_axi4_concat_rvalid;
assign m_axi4_concat_rready = {m_axi4_manycore_rready, sh_cl_dma_pcis_rready};

assign m_axi4_concat_awid = {m_axi4_manycore_awid, sh_cl_dma_pcis_awid};
assign m_axi4_concat_awaddr = {m_axi4_manycore_awaddr, sh_cl_dma_pcis_awaddr};
assign m_axi4_concat_awlen = {m_axi4_manycore_awlen, sh_cl_dma_pcis_awlen};
assign m_axi4_concat_awsize = {m_axi4_manycore_awsize, sh_cl_dma_pcis_awsize};
assign m_axi4_concat_awburst = {m_axi4_manycore_awburst, 2'b01};
assign m_axi4_concat_awlock = {m_axi4_manycore_awlock, 2'b00};
assign m_axi4_concat_awcache = {m_axi4_manycore_awcache, 4'b0000};
assign m_axi4_concat_awprot = {m_axi4_manycore_awprot, 3'b000};
assign m_axi4_concat_awregion = {m_axi4_manycore_awregion, 4'b0000};
assign m_axi4_concat_awqos = {m_axi4_manycore_awqos, 4'b0000};
assign m_axi4_concat_awvalid = {m_axi4_manycore_awvalid, sh_cl_dma_pcis_awvalid};
assign {m_axi4_manycore_awready, cl_sh_dma_pcis_awready} = m_axi4_concat_awready;

assign m_axi4_concat_wdata = {m_axi4_manycore_wdata, sh_cl_dma_pcis_wdata};
assign m_axi4_concat_wstrb = {m_axi4_manycore_wstrb, sh_cl_dma_pcis_wstrb};
assign m_axi4_concat_wlast = {m_axi4_manycore_wlast, sh_cl_dma_pcis_wlast};
assign m_axi4_concat_wvalid = {m_axi4_manycore_wvalid, sh_cl_dma_pcis_wvalid};
assign {m_axi4_manycore_wready, cl_sh_dma_pcis_wready} = m_axi4_concat_wready;

assign {m_axi4_manycore_bid, cl_sh_dma_pcis_bid} = m_axi4_concat_bid;
assign {m_axi4_manycore_bresp, cl_sh_dma_pcis_bresp} = m_axi4_concat_bresp;
assign {m_axi4_manycore_bvalid, cl_sh_dma_pcis_bvalid} = m_axi4_concat_bvalid;
assign m_axi4_concat_bready = {m_axi4_manycore_bready, sh_cl_dma_pcis_bready};

assign m_axi4_concat_arid = {m_axi4_manycore_arid, sh_cl_dma_pcis_arid};
assign m_axi4_concat_araddr = {m_axi4_manycore_araddr, sh_cl_dma_pcis_araddr};
assign m_axi4_concat_arlen = {m_axi4_manycore_arlen, sh_cl_dma_pcis_arlen};
assign m_axi4_concat_arsize = {m_axi4_manycore_arsize, sh_cl_dma_pcis_arsize};
assign m_axi4_concat_arburst = {m_axi4_manycore_arburst, 2'b01};
assign m_axi4_concat_arlock = {m_axi4_manycore_arlock, 2'b00};
assign m_axi4_concat_arcache = {m_axi4_manycore_arcache, 4'b0000};
assign m_axi4_concat_arprot = {m_axi4_manycore_arprot, 3'b000};
assign m_axi4_concat_arregion = {m_axi4_manycore_arregion, 4'b0000};
assign m_axi4_concat_arqos = {m_axi4_manycore_arqos, 4'b0000};
assign m_axi4_concat_arvalid = {m_axi4_manycore_arvalid, sh_cl_dma_pcis_arvalid};
assign {m_axi4_manycore_arready, cl_sh_dma_pcis_arready} = m_axi4_concat_arready;

axi_crossbar_v2_1_18_axi_crossbar #(
  .C_FAMILY                   ("virtexuplus"       ),
  .C_NUM_SLAVE_SLOTS          (2                   ),
  .C_NUM_MASTER_SLOTS         (1                   ),
  .C_AXI_ID_WIDTH             (6                   ),
  .C_AXI_ADDR_WIDTH           (64                  ),
  .C_AXI_DATA_WIDTH           (512                 ),
  .C_AXI_PROTOCOL             (0                   ), // 0 is AXI4 Full
  .C_NUM_ADDR_RANGES          (1                   ),
  .C_M_AXI_BASE_ADDR          (128'H00000000000000000000000000000000),
  .C_M_AXI_ADDR_WIDTH         (64'H0000004000000040),
  .C_S_AXI_BASE_ID            (64'H0000000000000000),
  .C_S_AXI_THREAD_ID_WIDTH    (64'H0000000500000005),
  .C_AXI_SUPPORTS_USER_SIGNALS(0                   ),
  .C_AXI_AWUSER_WIDTH         (1                   ),
  .C_AXI_ARUSER_WIDTH         (1                   ),
  .C_AXI_WUSER_WIDTH          (1                   ),
  .C_AXI_RUSER_WIDTH          (1                   ),
  .C_AXI_BUSER_WIDTH          (1                   ),
  .C_M_AXI_WRITE_CONNECTIVITY (32'H00000003        ),
  .C_M_AXI_READ_CONNECTIVITY  (32'H00000003        ),
  .C_R_REGISTER               (0                   ),
  .C_S_AXI_SINGLE_THREAD      (64'H0000000000000000),
  .C_S_AXI_WRITE_ACCEPTANCE   (64'H0000000200000002),
  .C_S_AXI_READ_ACCEPTANCE    (64'H0000000200000002),
  .C_M_AXI_WRITE_ISSUING      (32'H00000004        ),
  .C_M_AXI_READ_ISSUING       (32'H00000004        ),
  .C_S_AXI_ARB_PRIORITY       (64'H0000000000000000),
  .C_M_AXI_SECURE             (32'H00000000        ),
  .C_CONNECTIVITY_MODE        (0                   )
) xbar (
  .aclk          (clk_main_a0    ),
  .aresetn       (rst_main_n_sync),
  .s_axi_awid    (m_axi4_concat_awid   ),
  .s_axi_awaddr  (m_axi4_concat_awaddr ),
  .s_axi_awlen   (m_axi4_concat_awlen  ),
  .s_axi_awsize  (m_axi4_concat_awsize ),
  .s_axi_awburst (m_axi4_concat_awburst),
  .s_axi_awlock  (m_axi4_concat_awlock ),
  .s_axi_awcache (m_axi4_concat_awcache),
  .s_axi_awprot  (m_axi4_concat_awprot ),
  .s_axi_awqos   (m_axi4_concat_awqos  ),
  .s_axi_awuser  (2'b0),
  .s_axi_awvalid (m_axi4_concat_awvalid),
  .s_axi_awready (m_axi4_concat_awready),
  .s_axi_wid     (12'H0         ),
  .s_axi_wdata   (m_axi4_concat_wdata  ),
  .s_axi_wstrb   (m_axi4_concat_wstrb  ),
  .s_axi_wlast   (m_axi4_concat_wlast  ),
  .s_axi_wuser   (2'H0          ),
  .s_axi_wvalid  (m_axi4_concat_wvalid ),
  .s_axi_wready  (m_axi4_concat_wready ),
  .s_axi_bid     (m_axi4_concat_bid    ),
  .s_axi_bresp   (m_axi4_concat_bresp  ),
  .s_axi_buser   (              ),
  .s_axi_bvalid  (m_axi4_concat_bvalid ),
  .s_axi_bready  (m_axi4_concat_bready ),
  .s_axi_arid    (m_axi4_concat_arid   ),
  .s_axi_araddr  (m_axi4_concat_araddr ),
  .s_axi_arlen   (m_axi4_concat_arlen  ),
  .s_axi_arsize  (m_axi4_concat_arsize ),
  .s_axi_arburst (m_axi4_concat_arburst),
  .s_axi_arlock  (m_axi4_concat_arlock ),
  .s_axi_arcache (m_axi4_concat_arcache),
  .s_axi_arprot  (m_axi4_concat_arprot ),
  .s_axi_arqos   (m_axi4_concat_arqos  ),
  .s_axi_aruser  (2'H0          ),
  .s_axi_arvalid (m_axi4_concat_arvalid),
  .s_axi_arready (m_axi4_concat_arready),
  .s_axi_rid     (m_axi4_concat_rid    ),
  .s_axi_rdata   (m_axi4_concat_rdata  ),
  .s_axi_rresp   (m_axi4_concat_rresp  ),
  .s_axi_rlast   (m_axi4_concat_rlast  ),
  .s_axi_ruser   (              ),
  .s_axi_rvalid  (m_axi4_concat_rvalid ),
  .s_axi_rready  (m_axi4_concat_rready ),
  .m_axi_awid    (cl_sh_ddr_awid     ),
  .m_axi_awaddr  (cl_sh_ddr_awaddr   ),
  .m_axi_awlen   (cl_sh_ddr_awlen    ),
  .m_axi_awsize  (cl_sh_ddr_awsize   ),
  .m_axi_awburst (cl_sh_ddr_awburst  ),
  .m_axi_awlock  (                   ),
  .m_axi_awcache (                   ),
  .m_axi_awprot  (                   ),
  .m_axi_awregion(                   ),
  .m_axi_awqos   (                   ),
  .m_axi_awuser  (                   ),
  .m_axi_awvalid (cl_sh_ddr_awvalid  ),
  .m_axi_awready (sh_cl_ddr_awready  ),
  .m_axi_wid     (cl_sh_ddr_wid      ),
  .m_axi_wdata   (cl_sh_ddr_wdata    ),
  .m_axi_wstrb   (cl_sh_ddr_wstrb    ),
  .m_axi_wlast   (cl_sh_ddr_wlast    ),
  .m_axi_wuser   (                   ),
  .m_axi_wvalid  (cl_sh_ddr_wvalid   ),
  .m_axi_wready  (sh_cl_ddr_wready   ),
  .m_axi_bid     (sh_cl_ddr_bid      ),
  .m_axi_bresp   (sh_cl_ddr_bresp    ),
  .m_axi_buser   (                   ),
  .m_axi_bvalid  (sh_cl_ddr_bvalid   ),
  .m_axi_bready  (cl_sh_ddr_bready   ),
  .m_axi_arid    (cl_sh_ddr_arid     ),
  .m_axi_araddr  (cl_sh_ddr_araddr   ),
  .m_axi_arlen   (cl_sh_ddr_arlen    ),
  .m_axi_arsize  (cl_sh_ddr_arsize   ),
  .m_axi_arburst (cl_sh_ddr_arburst  ),
  .m_axi_arlock  (                   ),
  .m_axi_arcache (                   ),
  .m_axi_arprot  (                   ),
  .m_axi_arregion(                   ),
  .m_axi_arqos   (                   ),
  .m_axi_aruser  (                   ),
  .m_axi_arvalid (cl_sh_ddr_arvalid  ),
  .m_axi_arready (sh_cl_ddr_arready  ),
  .m_axi_rid     (sh_cl_ddr_rid      ),
  .m_axi_rdata   (sh_cl_ddr_rdata    ),
  .m_axi_rresp   (sh_cl_ddr_rresp    ),
  .m_axi_rlast   (sh_cl_ddr_rlast    ),
  .m_axi_ruser   (1'H0               ),
  .m_axi_rvalid  (sh_cl_ddr_rvalid   ),
  .m_axi_rready  (cl_sh_ddr_rready   ));

//--------------------------------------------
// AXI-Lite OCL System
//---------------------------------------------

// Tied to 0, for now:
//assign m_axil_ocl_awready = 1'b0;
//assign m_axil_ocl_wready = 1'b0;
//assign m_axil_ocl_bvalid = 1'b0;
//assign m_axil_ocl_bresp = 2'b00;
//assign m_axil_ocl_arready = 1'b0;

// Read data/response
//assign m_axil_ocl_rvalid = 1'b0;
//assign m_axil_ocl_rresp = 2'b00;

axi_register_slice_light AXIL_OCL_REG_SLC (
   .aclk          (clk_main_a0),
   .aresetn       (rst_main_n_sync),
   .s_axi_awaddr  (sh_ocl_awaddr),
   .s_axi_awprot   (2'h0),
   .s_axi_awvalid (sh_ocl_awvalid),
   .s_axi_awready (ocl_sh_awready),
   .s_axi_wdata   (sh_ocl_wdata),
   .s_axi_wstrb   (sh_ocl_wstrb),
   .s_axi_wvalid  (sh_ocl_wvalid),
   .s_axi_wready  (ocl_sh_wready),
   .s_axi_bresp   (ocl_sh_bresp),
   .s_axi_bvalid  (ocl_sh_bvalid),
   .s_axi_bready  (sh_ocl_bready),
   .s_axi_araddr  (sh_ocl_araddr),
   .s_axi_arvalid (sh_ocl_arvalid),
   .s_axi_arready (ocl_sh_arready),
   .s_axi_rdata   (ocl_sh_rdata),
   .s_axi_rresp   (ocl_sh_rresp),
   .s_axi_rvalid  (ocl_sh_rvalid),
   .s_axi_rready  (sh_ocl_rready),
   .m_axi_awaddr  (m_axil_ocl_awaddr),
   .m_axi_awprot  (),
   .m_axi_awvalid (m_axil_ocl_awvalid),
   .m_axi_awready (m_axil_ocl_awready),
   .m_axi_wdata   (m_axil_ocl_wdata),
   .m_axi_wstrb   (m_axil_ocl_wstrb),
   .m_axi_wvalid  (m_axil_ocl_wvalid),
   .m_axi_wready  (m_axil_ocl_wready),
   .m_axi_bresp   (m_axil_ocl_bresp),
   .m_axi_bvalid  (m_axil_ocl_bvalid),
   .m_axi_bready  (m_axil_ocl_bready),
   .m_axi_araddr  (m_axil_ocl_araddr),
   .m_axi_arvalid (m_axil_ocl_arvalid),
   .m_axi_arready (m_axil_ocl_arready),
   .m_axi_rdata   (m_axil_ocl_rdata),
   .m_axi_rresp   (m_axil_ocl_rresp),
   .m_axi_rvalid  (m_axil_ocl_rvalid),
   .m_axi_rready  (m_axil_ocl_rready)
  );

// parameters
parameter dmem_size_p = 1024;
parameter icache_entries_p = 1024;
parameter icache_tag_width_p = 12;
parameter dram_ch_addr_width_p = 26;
parameter epa_addr_width_p = 16;

parameter num_tiles_x_p = 4;
parameter num_tiles_y_p = 4;
parameter x_cord_width_lp = `BSG_SAFE_CLOG2(num_tiles_x_p);
parameter y_cord_width_lp = `BSG_SAFE_CLOG2(num_tiles_y_p+1);
parameter load_id_width_p = 11;

parameter num_cache_p = 2;
parameter data_width_p = 32;
parameter addr_width_p = 26;
parameter block_size_in_words_p = 16;
parameter sets_p = 32;

parameter axi_id_width_p = 6;
parameter axi_addr_width_p = 64;
parameter axi_data_width_p = 512;

`declare_bsg_manycore_link_sif_s(addr_width_p, data_width_p, x_cord_width_lp, y_cord_width_lp, load_id_width_p);
bsg_manycore_link_sif_s [num_cache_p-1:0] cache_link_sif_li;
bsg_manycore_link_sif_s [num_cache_p-1:0] cache_link_sif_lo;

logic [num_cache_p-1:0][x_cord_width_lp-1:0] cache_x_lo;
logic [num_cache_p-1:0][y_cord_width_lp-1:0] cache_y_lo;

bsg_manycore_link_sif_s loader_link_sif_li;
bsg_manycore_link_sif_s loader_link_sif_lo;

bsg_manycore_wrapper #(
  .dmem_size_p(dmem_size_p)
  ,.icache_entries_p(icache_entries_p)
  ,.icache_tag_width_p(icache_tag_width_p)
  ,.num_tiles_x_p(num_tiles_x_p)
  ,.num_tiles_y_p(num_tiles_y_p)
  ,.load_id_width_p(load_id_width_p)
  ,.addr_width_p(addr_width_p)
  ,.epa_addr_width_p(epa_addr_width_p)
  ,.dram_ch_addr_width_p(dram_ch_addr_width_p)
  ,.data_width_p(data_width_p)
  ,.num_cache_p(num_cache_p)
) manycore_wrapper (
  .clk_i(clk_main_a0)
  ,.reset_i(~rst_main_n_sync)

  ,.cache_link_sif_i(cache_link_sif_li)
  ,.cache_link_sif_o(cache_link_sif_lo)
  ,.cache_x_o(cache_x_lo)
  ,.cache_y_o(cache_y_lo)

  ,.loader_link_sif_i(loader_link_sif_li)
  ,.loader_link_sif_o(loader_link_sif_lo)
);


// cache
bsg_cache_wrapper #(
  .num_cache_p(num_cache_p)
  ,.data_width_p(data_width_p)
  ,.addr_width_p(addr_width_p+2)
  ,.block_size_in_words_p(block_size_in_words_p)
  ,.sets_p(sets_p)
  ,.lo_addr_width_p(addr_width_p-1)

  ,.axi_id_width_p(axi_id_width_p)
  ,.axi_addr_width_p(axi_addr_width_p)
  ,.axi_data_width_p(axi_data_width_p)
  ,.axi_burst_len_p(1)

  ,.link_addr_width_p(addr_width_p)
  ,.link_lo_addr_width_p(addr_width_p-1)
  ,.x_cord_width_p(x_cord_width_lp)
  ,.y_cord_width_p(y_cord_width_lp)
  ,.load_id_width_p(load_id_width_p)
) cw (
  .clk_i(clk_main_a0)
  ,.reset_i(~rst_main_n_sync)

  ,.my_x_i(cache_x_lo)
  ,.my_y_i(cache_y_lo)
  ,.link_sif_i(cache_link_sif_lo)
  ,.link_sif_o(cache_link_sif_li)

  ,.axi_awid_o    (m_axi4_manycore_awid)
  ,.axi_awaddr_o  (m_axi4_manycore_awaddr)
  ,.axi_awlen_o   (m_axi4_manycore_awlen)
  ,.axi_awsize_o  (m_axi4_manycore_awsize)
  ,.axi_awburst_o (m_axi4_manycore_awburst)
  ,.axi_awcache_o (m_axi4_manycore_awcache)
  ,.axi_awprot_o  (m_axi4_manycore_awprot)
  ,.axi_awlock_o  (m_axi4_manycore_awlock)
  ,.axi_awvalid_o (m_axi4_manycore_awvalid)
  ,.axi_awready_i (m_axi4_manycore_awready)

  ,.axi_wdata_o   (m_axi4_manycore_wdata)
  ,.axi_wstrb_o   (m_axi4_manycore_wstrb)
  ,.axi_wlast_o   (m_axi4_manycore_wlast)
  ,.axi_wvalid_o  (m_axi4_manycore_wvalid)
  ,.axi_wready_i  (m_axi4_manycore_wready)

  ,.axi_bid_i     (m_axi4_manycore_bid)
  ,.axi_bresp_i   (m_axi4_manycore_bresp)
  ,.axi_bvalid_i  (m_axi4_manycore_bvalid)
  ,.axi_bready_o  (m_axi4_manycore_bready)

  ,.axi_arid_o    (m_axi4_manycore_arid)
  ,.axi_araddr_o  (m_axi4_manycore_araddr)
  ,.axi_arlen_o   (m_axi4_manycore_arlen)
  ,.axi_arsize_o  (m_axi4_manycore_arsize)
  ,.axi_arburst_o (m_axi4_manycore_arburst)
  ,.axi_arcache_o (m_axi4_manycore_arcache)
  ,.axi_arprot_o  (m_axi4_manycore_arprot)
  ,.axi_arlock_o  (m_axi4_manycore_arlock)
  ,.axi_arvalid_o (m_axi4_manycore_arvalid)
  ,.axi_arready_i (m_axi4_manycore_arready)

  ,.axi_rid_i     (m_axi4_manycore_rid)
  ,.axi_rdata_i   (m_axi4_manycore_rdata)
  ,.axi_rresp_i   (m_axi4_manycore_rresp)
  ,.axi_rlast_i   (m_axi4_manycore_rlast)
  ,.axi_rvalid_i  (m_axi4_manycore_rvalid)
  ,.axi_rready_o  (m_axi4_manycore_rready)
);

assign m_axi4_manycore_awregion = 4'b0;
assign m_axi4_manycore_awqos = 4'b0;

assign m_axi4_manycore_arregion = 4'b0;
assign m_axi4_manycore_arqos = 4'b0;


// axil_to_mcl
//
logic mcl_v_li;
logic [127:0] mcl_data_li;
logic mcl_yumi_lo;

logic mcl_v_lo;
logic mcl_ready_li;
logic [127:0] mcl_data_lo;

axil_to_mcl #(
  .mcl_intf_num_p(1)
  ,.fifo_width_p(128)
) axil_to_mcl_inst (
  .clk_i(clk_main_a0)
  ,.reset_i(~rst_main_n_sync)

  ,.s_axil_mcl_awvalid (m_axil_ocl_awvalid)
  ,.s_axil_mcl_awaddr  (m_axil_ocl_awaddr)
  ,.s_axil_mcl_awready (m_axil_ocl_awready)
  ,.s_axil_mcl_wvalid  (m_axil_ocl_wvalid)
  ,.s_axil_mcl_wdata   (m_axil_ocl_wdata)
  ,.s_axil_mcl_wstrb   (m_axil_ocl_wstrb)
  ,.s_axil_mcl_wready  (m_axil_ocl_wready)
  ,.s_axil_mcl_bresp   (m_axil_ocl_bresp)
  ,.s_axil_mcl_bvalid  (m_axil_ocl_bvalid)
  ,.s_axil_mcl_bready  (m_axil_ocl_bready)
  ,.s_axil_mcl_araddr  (m_axil_ocl_araddr)
  ,.s_axil_mcl_arvalid (m_axil_ocl_arvalid)
  ,.s_axil_mcl_arready (m_axil_ocl_arready)
  ,.s_axil_mcl_rdata   (m_axil_ocl_rdata)
  ,.s_axil_mcl_rresp   (m_axil_ocl_rresp)
  ,.s_axil_mcl_rvalid  (m_axil_ocl_rvalid)
  ,.s_axil_mcl_rready  (m_axil_ocl_rready)

  ,.mcl_v_i(mcl_v_li)
  ,.mcl_data_i(mcl_data_li)
  ,.mcl_yumi_o(mcl_yumi_lo)

  ,.mcl_v_o(mcl_v_lo)
  ,.mcl_ready_i(mcl_ready_li)
  ,.mcl_data_o(mcl_data_lo)
);

logic endpoint_v_lo;
logic endpoint_yumi_li;
logic [data_width_p-1:0] endpoint_data_lo;
logic [(data_width_p>>3)-1:0] endpoint_mask_lo;
logic [addr_width_p-1:0] endpoint_addr_lo;
logic endpoint_we_lo;

`declare_bsg_manycore_packet_s(addr_width_p,data_width_p,x_cord_width_lp,y_cord_width_lp,load_id_width_p);
localparam bsg_manycore_packet_width_lp = `bsg_manycore_packet_width(addr_width_p,data_width_p,x_cord_width_lp,y_cord_width_lp,load_id_width_p);
bsg_manycore_packet_s endpoint_out_packet_li;
logic endpoint_out_v_li;
logic endpoint_out_ready_lo;

bsg_manycore_endpoint_standard #(
  .x_cord_width_p(x_cord_width_lp)
  ,.y_cord_width_p(y_cord_width_lp)
  ,.data_width_p(data_width_p)
  ,.addr_width_p(addr_width_p)
  ,.fifo_els_p(4)
  ,.max_out_credits_p(16)
  ,.load_id_width_p(load_id_width_p)
) axil_endpoint_standard (
  .clk_i(clk_main_a0)
  ,.reset_i(~rst_main_n_sync)
     
  ,.link_sif_i(loader_link_sif_lo)
  ,.link_sif_o(loader_link_sif_li)

  // receiving packets from network
  ,.in_v_o(endpoint_v_lo)
  ,.in_yumi_i(endpoint_yumi_li)
  ,.in_data_o(endpoint_data_lo)
  ,.in_mask_o(endpoint_mask_lo)
  ,.in_addr_o(endpoint_addr_lo)
  ,.in_we_o(endpoint_we_lo)
  ,.in_src_x_cord_o()
  ,.in_src_y_cord_o()

  // sending packets out to network
  ,.out_v_i(endpoint_out_v_li)
  ,.out_packet_i(endpoint_out_packet_li)
  ,.out_ready_o(endpoint_out_ready_lo)
  
  ,.returned_data_r_o()
  ,.returned_load_id_r_o()
  ,.returned_v_r_o()
  ,.returned_fifo_full_o()
  ,.returned_yumi_i(1'b0)

  ,.returning_data_i('0)
  ,.returning_v_i(1'b0)

  ,.out_credits_o()

  ,.my_x_i(2'b11)
  ,.my_y_i(3'b100)
);

assign endpoint_out_v_li = mcl_v_lo;
assign mcl_ready_li = endpoint_out_ready_lo;
always_comb begin
  endpoint_out_packet_li = mcl_data_lo[0+:bsg_manycore_packet_width_lp];
end

assign mcl_v_li = endpoint_v_lo;
assign endpoint_yumi_li = endpoint_v_lo & mcl_yumi_lo;
assign mcl_data_li = {{(128-32-26){1'b0}}, endpoint_addr_lo, endpoint_data_lo};

//-----------------------------------------------
// Debug bridge, used if need Virtual JTAG
//-----------------------------------------------
`ifndef DISABLE_VJTAG_DEBUG

// Flop for timing global clock counter
logic[63:0] sh_cl_glcount0_q;

always_ff @(posedge clk_main_a0)
   if (!rst_main_n_sync)
      sh_cl_glcount0_q <= 0;
   else
      sh_cl_glcount0_q <= sh_cl_glcount0;


// Integrated Logic Analyzers (ILA)
ila_0 CL_ILA_0 (
                .clk    (clk_main_a0),
                .probe0 (m_axil_ocl_awvalid),
                .probe1 (m_axil_ocl_awaddr),
                .probe2 (m_axil_ocl_awready),
                .probe3 (m_axil_ocl_arvalid),
                .probe4 (m_axil_ocl_araddr),
                .probe5 (m_axil_ocl_arready)
                );

 ila_0 CL_ILA_1 (
                .clk    (clk_main_a0),
                .probe0 (m_axil_ocl_bvalid),
                .probe1 (sh_cl_glcount0_q),
                .probe2 (m_axil_ocl_bready),
                .probe3 (m_axil_ocl_rvalid),
                .probe4 ({32'b0,m_axil_ocl_rdata[31:0]}),
                .probe5 (m_axil_ocl_rready)
                 );

// Debug Bridge 
cl_debug_bridge CL_DEBUG_BRIDGE (
     .clk(clk_main_a0),
     .S_BSCAN_drck(drck),
     .S_BSCAN_shift(shift),
     .S_BSCAN_tdi(tdi),
     .S_BSCAN_update(update),
     .S_BSCAN_sel(sel),
     .S_BSCAN_tdo(tdo),
     .S_BSCAN_tms(tms),
     .S_BSCAN_tck(tck),
     .S_BSCAN_runtest(runtest),
     .S_BSCAN_reset(reset),
     .S_BSCAN_capture(capture),
     .S_BSCAN_bscanid_en(bscanid_en)
);

`endif //  `ifndef DISABLE_VJTAG_DEBUG
endmodule





