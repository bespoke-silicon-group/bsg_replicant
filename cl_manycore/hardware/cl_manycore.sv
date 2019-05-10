/**
 *  cl_manycore.v
 */

module cl_manycore
  import cl_manycore_pkg::*;
  (
    `include "cl_ports.vh"
  );


// For some silly reason, you need to leave this up here...
logic rst_main_n_sync;

`include "cl_id_defines.vh"
`include "cl_manycore_defines.vh"

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


// -------------------------------------------------------
// HammerBlader Manycore wrapper
// -------------------------------------------------------
`include "f1_bladerunner_ports.inc"

hb_mc_wrapper #(
  .axi_id_width_p  (axi_id_width_p  ),
  .axi_addr_width_p(axi_addr_width_p),
  .axi_data_width_p(axi_data_width_p)
) hb_mc_inst (
  .clk_i           (clk_i           ),
  .resetn_i        (resetn_i        ),
  // axi-lite interface
  .s_axil_awaddr_i (s_axil_awaddr_li ),
  .s_axil_awvalid_i(s_axil_awvalid_li),
  .s_axil_awready_o(s_axil_awready_lo),
  .s_axil_wdata_i  (s_axil_wdata_li  ),
  .s_axil_wstrb_i  (s_axil_wstrb_li  ),
  .s_axil_wvalid_i (s_axil_wvalid_li ),
  .s_axil_wready_o (s_axil_wready_lo ),
  .s_axil_bresp_o  (s_axil_bresp_lo  ),
  .s_axil_bvalid_o (s_axil_bvalid_lo ),
  .s_axil_bready_i (s_axil_bready_li ),
  .s_axil_araddr_i (s_axil_araddr_li ),
  .s_axil_arvalid_i(s_axil_arvalid_li),
  .s_axil_arready_o(s_axil_arready_lo),
  .s_axil_rdata_o  (s_axil_rdata_lo  ),
  .s_axil_rresp_o  (s_axil_rresp_lo  ),
  .s_axil_rvalid_o (s_axil_rvalid_lo ),
  .s_axil_rready_i (s_axil_rready_li ),
  // axi4 interface
  .s_axi_awid_i    (s_axi_awid_li    ),
  .s_axi_awaddr_i  (s_axi_awaddr_li  ),
  .s_axi_awlen_i   (s_axi_awlen_li   ),
  .s_axi_awsize_i  (s_axi_awsize_li  ),
  .s_axi_awburst_i (s_axi_awburst_li ),
  .s_axi_awvalid_i (s_axi_awvalid_li ),
  .s_axi_awready_o (s_axi_awready_lo ),
  .s_axi_wdata_i   (s_axi_wdata_li   ),
  .s_axi_wstrb_i   (s_axi_wstrb_li   ),
  .s_axi_wlast_i   (s_axi_wlast_li   ),
  .s_axi_wvalid_i  (s_axi_wvalid_li  ),
  .s_axi_wready_o  (s_axi_wready_lo  ),
  .s_axi_bid_o     (s_axi_bid_lo     ),
  .s_axi_bresp_o   (s_axi_bresp_lo   ),
  .s_axi_bvalid_o  (s_axi_bvalid_lo  ),
  .s_axi_bready_i  (s_axi_bready_li  ),
  .s_axi_arid_i    (s_axi_arid_li    ),
  .s_axi_araddr_i  (s_axi_araddr_li  ),
  .s_axi_arlen_i   (s_axi_arlen_li   ),
  .s_axi_arsize_i  (s_axi_arsize_li  ),
  .s_axi_arburst_i (s_axi_arburst_li ),
  .s_axi_arvalid_i (s_axi_arvalid_li ),
  .s_axi_arready_o (s_axi_arready_lo ),
  .s_axi_rid_o     (s_axi_rid_lo     ),
  .s_axi_rdata_o   (s_axi_rdata_lo   ),
  .s_axi_rresp_o   (s_axi_rresp_lo   ),
  .s_axi_rlast_o   (s_axi_rlast_lo   ),
  .s_axi_rvalid_o  (s_axi_rvalid_lo  ),
  .s_axi_rready_i  (s_axi_rready_li  ),
  // dram axi4 interface
  .m_axi_awid_o    (m_axi_awid_lo    ),
  .m_axi_awaddr_o  (m_axi_awaddr_lo  ),
  .m_axi_awlen_o   (m_axi_awlen_lo   ),
  .m_axi_awsize_o  (m_axi_awsize_lo  ),
  .m_axi_awburst_o (m_axi_awburst_lo ),
  .m_axi_awvalid_o (m_axi_awvalid_lo ),
  .m_axi_awready_i (m_axi_awready_li ),
  .m_axi_wdata_o   (m_axi_wdata_lo   ),
  .m_axi_wstrb_o   (m_axi_wstrb_lo   ),
  .m_axi_wlast_o   (m_axi_wlast_lo   ),
  .m_axi_wvalid_o  (m_axi_wvalid_lo  ),
  .m_axi_wready_i  (m_axi_wready_li  ),
  .m_axi_bid_i     (m_axi_bid_li     ),
  .m_axi_bresp_i   (m_axi_bresp_li   ),
  .m_axi_bvalid_i  (m_axi_bvalid_li  ),
  .m_axi_bready_o  (m_axi_bready_lo  ),
  .m_axi_arid_o    (m_axi_arid_lo    ),
  .m_axi_araddr_o  (m_axi_araddr_lo  ),
  .m_axi_arlen_o   (m_axi_arlen_lo   ),
  .m_axi_arsize_o  (m_axi_arsize_lo  ),
  .m_axi_arburst_o (m_axi_arburst_lo ),
  .m_axi_arvalid_o (m_axi_arvalid_lo ),
  .m_axi_arready_i (m_axi_arready_li ),
  .m_axi_rid_i     (m_axi_rid_li     ),
  .m_axi_rdata_i   (m_axi_rdata_li   ),
  .m_axi_rresp_i   (m_axi_rresp_li   ),
  .m_axi_rlast_i   (m_axi_rlast_li   ),
  .m_axi_rvalid_i  (m_axi_rvalid_li  ),
  .m_axi_rready_o  (m_axi_rready_lo  )
);

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
                .probe0 (sh_ocl_awvalid)
                ,.probe1 (64'(sh_ocl_awaddr))
                ,.probe2 (ocl_sh_awready)
                ,.probe3 (m_axil_ocl_arvalid)
                ,.probe4 (64'(sh_ocl_wdata))
                ,.probe5 (ocl_sh_wready)
                );

ila_0 CL_ILA_1 (
              .clk    (clk_main_a0)
              ,.probe0 (sh_ocl_arvalid)
              ,.probe1 (sh_cl_glcount0_q)
              ,.probe2 (ocl_sh_arready)
              ,.probe3 (ocl_sh_rvalid)
              ,.probe4 (64'(s_axil_rdata_lo))
              ,.probe5 (sh_ocl_rready)
               );

// Debug Bridge
cl_debug_bridge CL_DEBUG_BRIDGE (
     .clk(clk_main_a0)
     ,.S_BSCAN_drck(drck)
     ,.S_BSCAN_shift(shift)
     ,.S_BSCAN_tdi(tdi)
     ,.S_BSCAN_update(update)
     ,.S_BSCAN_sel(sel)
     ,.S_BSCAN_tdo(tdo)
     ,.S_BSCAN_tms(tms)
     ,.S_BSCAN_tck(tck)
     ,.S_BSCAN_runtest(runtest)
     ,.S_BSCAN_reset(reset)
     ,.S_BSCAN_capture(capture)
     ,.S_BSCAN_bscanid_en(bscanid_en)
);

`endif //  `ifndef DISABLE_VJTAG_DEBUG
endmodule
