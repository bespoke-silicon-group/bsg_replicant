`include "bsg_axi_bus_pkg.vh"

// ---------------------------------------------
// axil ocl interface
// ---------------------------------------------
wire clk_i    = clk_main_a0    ;
wire resetn_i = rst_main_n_sync;

`declare_bsg_axil_bus_s(1, sh_ocl_si_s, sh_ocl_so_s);
sh_ocl_si_s s_axil_ocl_li_cast;
sh_ocl_so_s s_axil_ocl_lo_cast;

assign s_axil_ocl_li_cast.awaddr  = sh_ocl_awaddr;
assign s_axil_ocl_li_cast.awvalid = sh_ocl_awvalid;
assign s_axil_ocl_li_cast.wdata   = sh_ocl_wdata;
assign s_axil_ocl_li_cast.wstrb   = sh_ocl_wstrb;
assign s_axil_ocl_li_cast.wvalid  = sh_ocl_wvalid;
assign s_axil_ocl_li_cast.bready  = sh_ocl_bready;
assign s_axil_ocl_li_cast.araddr  = sh_ocl_araddr;
assign s_axil_ocl_li_cast.arvalid = sh_ocl_arvalid;
assign s_axil_ocl_li_cast.rready  = sh_ocl_rready;

assign ocl_sh_awready = s_axil_ocl_lo_cast.awready;
assign ocl_sh_wready  = s_axil_ocl_lo_cast.wready;
assign ocl_sh_bresp   = s_axil_ocl_lo_cast.bresp;
assign ocl_sh_bvalid  = s_axil_ocl_lo_cast.bvalid;
assign ocl_sh_arready = s_axil_ocl_lo_cast.arready;
assign ocl_sh_rdata   = s_axil_ocl_lo_cast.rdata;
assign ocl_sh_rresp   = s_axil_ocl_lo_cast.rresp;
assign ocl_sh_rvalid  = s_axil_ocl_lo_cast.rvalid;


// ---------------------------------------------
// axi4 pcis interface
// ---------------------------------------------

`declare_bsg_axi_bus_s(1, axi_id_width_p, axi_addr_width_p, axi_data_width_p, axi_pcis_si_s, axi_pcis_so_s);
axi_pcis_si_s s_axi_pcis_li_cast, m_axi_ddr_lo_cast;
axi_pcis_so_s s_axi_pcis_lo_cast, m_axi_ddr_li_cast;

// AXI4 pcis interface
assign s_axi_pcis_li_cast.awid     = sh_cl_dma_pcis_awid;
assign s_axi_pcis_li_cast.awaddr   = sh_cl_dma_pcis_awaddr;
assign s_axi_pcis_li_cast.awlen    = sh_cl_dma_pcis_awlen;
assign s_axi_pcis_li_cast.awsize   = sh_cl_dma_pcis_awsize;
assign s_axi_pcis_li_cast.awburst  = 2'b01;
assign s_axi_pcis_li_cast.awlock    = '0;
assign s_axi_pcis_li_cast.awcache   = '0;
assign s_axi_pcis_li_cast.awprot    = '0;
assign s_axi_pcis_li_cast.awqos     = '0;
assign s_axi_pcis_li_cast.awregion = '0;
assign s_axi_pcis_li_cast.awvalid  = sh_cl_dma_pcis_awvalid;
assign s_axi_pcis_li_cast.wdata    = sh_cl_dma_pcis_wdata;
assign s_axi_pcis_li_cast.wstrb    = sh_cl_dma_pcis_wstrb;
assign s_axi_pcis_li_cast.wlast    = sh_cl_dma_pcis_wlast;
assign s_axi_pcis_li_cast.wvalid   = sh_cl_dma_pcis_wvalid;
assign s_axi_pcis_li_cast.bready   = sh_cl_dma_pcis_bready;
assign s_axi_pcis_li_cast.arid     = sh_cl_dma_pcis_arid;
assign s_axi_pcis_li_cast.araddr   = sh_cl_dma_pcis_araddr;
assign s_axi_pcis_li_cast.arlen    = sh_cl_dma_pcis_arlen;
assign s_axi_pcis_li_cast.arsize   = sh_cl_dma_pcis_arsize;
assign s_axi_pcis_li_cast.arburst  = 2'b01;
assign s_axi_pcis_li_cast.arlock   = '0;
assign s_axi_pcis_li_cast.arcache  = '0;
assign s_axi_pcis_li_cast.arprot   = '0;
assign s_axi_pcis_li_cast.arqos    = '0;
assign s_axi_pcis_li_cast.arregion = '0;
assign s_axi_pcis_li_cast.arvalid  = sh_cl_dma_pcis_arvalid;
assign s_axi_pcis_li_cast.rready   = sh_cl_dma_pcis_rready;

assign cl_sh_dma_pcis_awready = s_axi_pcis_lo_cast.awready;
assign cl_sh_dma_pcis_wready  = s_axi_pcis_lo_cast.wready;
assign cl_sh_dma_pcis_bid     = s_axi_pcis_lo_cast.bid;
assign cl_sh_dma_pcis_bresp   = s_axi_pcis_lo_cast.bresp;
assign cl_sh_dma_pcis_bvalid  = s_axi_pcis_lo_cast.bvalid;
assign cl_sh_dma_pcis_arready = s_axi_pcis_lo_cast.arready;
assign cl_sh_dma_pcis_rid     = s_axi_pcis_lo_cast.rid;
assign cl_sh_dma_pcis_rdata   = s_axi_pcis_lo_cast.rdata;
assign cl_sh_dma_pcis_rresp   = s_axi_pcis_lo_cast.rresp;
assign cl_sh_dma_pcis_rlast   = s_axi_pcis_lo_cast.rlast;
assign cl_sh_dma_pcis_rvalid  = s_axi_pcis_lo_cast.rvalid;


// AXI4 DDR interface
assign cl_sh_ddr_awid    = m_axi_ddr_lo_cast.awid;
assign cl_sh_ddr_awaddr  = m_axi_ddr_lo_cast.awaddr;
assign cl_sh_ddr_awlen   = m_axi_ddr_lo_cast.awlen;
assign cl_sh_ddr_awsize  = m_axi_ddr_lo_cast.awsize;
assign cl_sh_ddr_awburst = m_axi_ddr_lo_cast.awburst;  // Burst mode, only INCR is supported,
assign cl_sh_ddr_awvalid = m_axi_ddr_lo_cast.awvalid;
assign cl_sh_ddr_wdata   = m_axi_ddr_lo_cast.wdata;
assign cl_sh_ddr_wstrb   = m_axi_ddr_lo_cast.wstrb;
assign cl_sh_ddr_wlast   = m_axi_ddr_lo_cast.wlast;
assign cl_sh_ddr_wvalid  = m_axi_ddr_lo_cast.wvalid;
assign cl_sh_ddr_bready  = m_axi_ddr_lo_cast.bready;
assign cl_sh_ddr_arid    = m_axi_ddr_lo_cast.arid;
assign cl_sh_ddr_araddr  = m_axi_ddr_lo_cast.araddr;
assign cl_sh_ddr_arlen   = m_axi_ddr_lo_cast.arlen;
assign cl_sh_ddr_arsize  = m_axi_ddr_lo_cast.arsize;
assign cl_sh_ddr_arburst = m_axi_ddr_lo_cast.arburst;  // Burst mode, only INCR is supported,
assign cl_sh_ddr_arvalid = m_axi_ddr_lo_cast.arvalid;
assign cl_sh_ddr_rready  = m_axi_ddr_lo_cast.rready;

assign m_axi_ddr_li_cast.awready = sh_cl_ddr_awready;
assign m_axi_ddr_li_cast.wready  = sh_cl_ddr_wready;
assign m_axi_ddr_li_cast.bid     = sh_cl_ddr_bid;
assign m_axi_ddr_li_cast.bresp   = sh_cl_ddr_bresp;
assign m_axi_ddr_li_cast.bvalid  = sh_cl_ddr_bvalid;
assign m_axi_ddr_li_cast.arready = sh_cl_ddr_arready;
assign m_axi_ddr_li_cast.rid     = sh_cl_ddr_rid;
assign m_axi_ddr_li_cast.rdata   = sh_cl_ddr_rdata;
assign m_axi_ddr_li_cast.rresp   = sh_cl_ddr_rresp;
assign m_axi_ddr_li_cast.rlast   = sh_cl_ddr_rlast;
assign m_axi_ddr_li_cast.rvalid  = sh_cl_ddr_rvalid;

// NOT USED in AXI4
assign cl_sh_ddr_wid = '0;

// synopsys translate_off
initial begin
  assert (axi_id_width_p == 6)
    else begin
      $error("## manycore axi id width width mismatches with the shell");
      $finish();
    end
  assert (axi_addr_width_p == 64)
    else begin
      $error("## manycore axi address width mismatches with the shell");
      $finish();
    end
  assert (axi_data_width_p == 512)
    else begin
      $error("## manycore axi data width mismatches with the shell");
      $finish();
    end
end
// synopsys translate_on
