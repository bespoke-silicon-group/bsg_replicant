// ---------------------------------------------
// axil ocl interface
// ---------------------------------------------
wire clk_i    = clk_main_a0    ;
wire resetn_i = rst_main_n_sync;

logic [31:0] s_axil_awaddr_li ;
logic        s_axil_awvalid_li;
logic        s_axil_awready_lo;
logic [31:0] s_axil_wdata_li  ;
logic [ 3:0] s_axil_wstrb_li  ;
logic        s_axil_wvalid_li ;
logic        s_axil_wready_lo ;
logic [ 1:0] s_axil_bresp_lo  ;
logic        s_axil_bvalid_lo ;
logic        s_axil_bready_li ;
logic [31:0] s_axil_araddr_li ;
logic        s_axil_arvalid_li;
logic        s_axil_arready_lo;
logic [31:0] s_axil_rdata_lo  ;
logic [ 1:0] s_axil_rresp_lo  ;
logic        s_axil_rvalid_lo ;
logic        s_axil_rready_li ;

assign s_axil_awaddr_li  = sh_ocl_awaddr;
assign s_axil_awvalid_li = sh_ocl_awvalid;
assign s_axil_wdata_li   = sh_ocl_wdata;
assign s_axil_wstrb_li   = sh_ocl_wstrb;
assign s_axil_wvalid_li  = sh_ocl_wvalid;
assign s_axil_bready_li  = sh_ocl_bready;
assign s_axil_araddr_li  = sh_ocl_araddr;
assign s_axil_arvalid_li = sh_ocl_arvalid;
assign s_axil_rready_li  = sh_ocl_rready;

assign ocl_sh_awready = s_axil_awready_lo;
assign ocl_sh_wready  = s_axil_wready_lo;
assign ocl_sh_bresp   = s_axil_bresp_lo;
assign ocl_sh_bvalid  = s_axil_bvalid_lo;
assign ocl_sh_arready = s_axil_arready_lo;
assign ocl_sh_rdata   = s_axil_rdata_lo;
assign ocl_sh_rresp   = s_axil_rresp_lo;
assign ocl_sh_rvalid  = s_axil_rvalid_lo;


// ---------------------------------------------
// axi4 pcis interface
// ---------------------------------------------
`include "cl_manycore_pkg.v"
import cl_manycore_pkg::*;

logic [      axi_id_width_p-1:0] s_axi_awid_li   ;
logic [    axi_addr_width_p-1:0] s_axi_awaddr_li ;
logic [                     7:0] s_axi_awlen_li  ;
logic [                     2:0] s_axi_awsize_li ;
logic [                     1:0] s_axi_awburst_li;
logic                            s_axi_awvalid_li;
logic                            s_axi_awready_lo;
logic [    axi_data_width_p-1:0] s_axi_wdata_li  ;
logic [(axi_data_width_p/8)-1:0] s_axi_wstrb_li  ;
logic                            s_axi_wlast_li  ;
logic                            s_axi_wvalid_li ;
logic                            s_axi_wready_lo ;
logic [      axi_id_width_p-1:0] s_axi_bid_lo    ;
logic [                     1:0] s_axi_bresp_lo  ;
logic                            s_axi_bvalid_lo ;
logic                            s_axi_bready_li ;
logic [      axi_id_width_p-1:0] s_axi_arid_li   ;
logic [    axi_addr_width_p-1:0] s_axi_araddr_li ;
logic [                     7:0] s_axi_arlen_li  ;
logic [                     2:0] s_axi_arsize_li ;
logic [                     1:0] s_axi_arburst_li;
logic                            s_axi_arvalid_li;
logic                            s_axi_arready_lo;
logic [      axi_id_width_p-1:0] s_axi_rid_lo    ;
logic [    axi_data_width_p-1:0] s_axi_rdata_lo  ;
logic [                     1:0] s_axi_rresp_lo  ;
logic                            s_axi_rlast_lo  ;
logic                            s_axi_rvalid_lo ;
logic                            s_axi_rready_li ;

assign s_axi_awid_li    = sh_cl_dma_pcis_awid;
assign s_axi_awaddr_li  = sh_cl_dma_pcis_awaddr;
assign s_axi_awlen_li   = sh_cl_dma_pcis_awlen;
assign s_axi_awsize_li  = sh_cl_dma_pcis_awsize;
assign s_axi_awburst_li = 2'b01;
assign s_axi_awvalid_li = sh_cl_dma_pcis_awvalid;
assign s_axi_wdata_li   = sh_cl_dma_pcis_wdata;
assign s_axi_wstrb_li   = sh_cl_dma_pcis_wstrb;
assign s_axi_wlast_li   = sh_cl_dma_pcis_wlast;
assign s_axi_wvalid_li  = sh_cl_dma_pcis_wvalid;
assign s_axi_bready_li  = sh_cl_dma_pcis_bready;
assign s_axi_arid_li    = sh_cl_dma_pcis_arid;
assign s_axi_araddr_li  = sh_cl_dma_pcis_araddr;
assign s_axi_arlen_li   = sh_cl_dma_pcis_arlen;
assign s_axi_arsize_li  = sh_cl_dma_pcis_arsize;
assign s_axi_arburst_li = 2'b01;
assign s_axi_arvalid_li = sh_cl_dma_pcis_arvalid;
assign s_axi_rready_li  = sh_cl_dma_pcis_rready;

assign cl_sh_dma_pcis_awready = s_axi_awready_lo;
assign cl_sh_dma_pcis_wready  = s_axi_wready_lo;
assign cl_sh_dma_pcis_bid     = s_axi_bid_lo;
assign cl_sh_dma_pcis_bresp   = s_axi_bresp_lo;
assign cl_sh_dma_pcis_bvalid  = s_axi_bvalid_lo;
assign cl_sh_dma_pcis_arready = s_axi_arready_lo;
assign cl_sh_dma_pcis_rid     = s_axi_rid_lo;
assign cl_sh_dma_pcis_rdata   = s_axi_rdata_lo;
assign cl_sh_dma_pcis_rresp   = s_axi_rresp_lo;
assign cl_sh_dma_pcis_rlast   = s_axi_rlast_lo;
assign cl_sh_dma_pcis_rvalid  = s_axi_rvalid_lo;


// AXI4 DDR interface
logic [      axi_id_width_p-1:0] m_axi_awid_lo   ;
logic [    axi_addr_width_p-1:0] m_axi_awaddr_lo ;
logic [                     7:0] m_axi_awlen_lo  ;
logic [                     2:0] m_axi_awsize_lo ;
logic [                     1:0] m_axi_awburst_lo;
logic                            m_axi_awvalid_lo;
logic                            m_axi_awready_li;
logic [    axi_data_width_p-1:0] m_axi_wdata_lo  ;
logic [(axi_data_width_p/8)-1:0] m_axi_wstrb_lo  ;
logic                            m_axi_wlast_lo  ;
logic                            m_axi_wvalid_lo ;
logic                            m_axi_wready_li ;
logic [      axi_id_width_p-1:0] m_axi_bid_li    ;
logic [                     1:0] m_axi_bresp_li  ;
logic                            m_axi_bvalid_li ;
logic                            m_axi_bready_lo ;
logic [      axi_id_width_p-1:0] m_axi_arid_lo   ;
logic [    axi_addr_width_p-1:0] m_axi_araddr_lo ;
logic [                     7:0] m_axi_arlen_lo  ;
logic [                     2:0] m_axi_arsize_lo ;
logic [                     1:0] m_axi_arburst_lo;
logic                            m_axi_arvalid_lo;
logic                            m_axi_arready_li;
logic [      axi_id_width_p-1:0] m_axi_rid_li    ;
logic [    axi_data_width_p-1:0] m_axi_rdata_li  ;
logic [                     1:0] m_axi_rresp_li  ;
logic                            m_axi_rlast_li  ;
logic                            m_axi_rvalid_li ;
logic                            m_axi_rready_lo ;


assign cl_sh_ddr_awid    = m_axi_awid_lo;
assign cl_sh_ddr_awaddr  = m_axi_awaddr_lo;
assign cl_sh_ddr_awlen   = m_axi_awlen_lo;
assign cl_sh_ddr_awsize  = m_axi_awsize_lo;
assign cl_sh_ddr_awburst = m_axi_awburst_lo;  // Burst mode, only INCR is supported,
assign cl_sh_ddr_awvalid = m_axi_awvalid_lo;
assign cl_sh_ddr_wdata   = m_axi_wdata_lo;
assign cl_sh_ddr_wstrb   = m_axi_wstrb_lo;
assign cl_sh_ddr_wlast   = m_axi_wlast_lo;
assign cl_sh_ddr_wvalid  = m_axi_wvalid_lo;
assign cl_sh_ddr_bready  = m_axi_bready_lo;
assign cl_sh_ddr_arid    = m_axi_arid_lo;
assign cl_sh_ddr_araddr  = m_axi_araddr_lo;
assign cl_sh_ddr_arlen   = m_axi_arlen_lo;
assign cl_sh_ddr_arsize  = m_axi_arsize_lo;
assign cl_sh_ddr_arburst = m_axi_arburst_lo;  // Burst mode, only INCR is supported,
assign cl_sh_ddr_arvalid = m_axi_arvalid_lo;
assign cl_sh_ddr_rready  = m_axi_rready_lo;

assign m_axi_awready_li = sh_cl_ddr_awready;
assign m_axi_wready_li  = sh_cl_ddr_wready;
assign m_axi_bid_li     = sh_cl_ddr_bid;
assign m_axi_bresp_li   = sh_cl_ddr_bresp;
assign m_axi_bvalid_li  = sh_cl_ddr_bvalid;
assign m_axi_arready_li = sh_cl_ddr_arready;
assign m_axi_rid_li     = sh_cl_ddr_rid;
assign m_axi_rdata_li   = sh_cl_ddr_rdata;
assign m_axi_rresp_li   = sh_cl_ddr_rresp;
assign m_axi_rlast_li   = sh_cl_ddr_rlast;
assign m_axi_rvalid_li  = sh_cl_ddr_rvalid;

// NOT USED in AXI4
assign cl_sh_ddr_wid = '0;

// synopsys translate_off
initial begin
  assert (axi_id_width_p == 6)
    else begin
      $error("## manycore axi4 id width width mismatches with the shell");
      $finish();
    end
  assert (axi_addr_width_p == 64)
    else begin
      $error("## manycore axi4 address width mismatches with the shell");
      $finish();
    end
  assert (axi_data_width_p == 512)
    else begin
      $error("## manycore axi4 data width mismatches with the shell");
      $finish();
    end
end
// synopsys translate_on
