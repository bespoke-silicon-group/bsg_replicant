/**
 *  cl_manycore.v
 */

`include "cl_manycore_pkg.v"
import cl_manycore_pkg::*;

module cl_manycore
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
`ifdef BSG_TARGET_F1
`include "f1_bladerunner_ports.inc"
`else
`include "vcu128_bladerunner_ports.inc"
`endif

hb_mc_wrapper #(
  .axi_id_width_p  (axi_id_width_p  ),
  .axi_addr_width_p(axi_addr_width_p),
  .axi_data_width_p(axi_data_width_p)
) hb_mc_inst (
  .clk_i           (clk_i            ),
  .resetn_i        (resetn_i         ),
  .s_axil_ocl_bus_i(s_axil_ocl_li_cast),
  .s_axil_ocl_bus_o(s_axil_ocl_lo_cast),
  .s_axi_pcis_bus_i(s_axi_pcis_li_cast),
  .s_axi_pcis_bus_o(s_axi_pcis_lo_cast),
  .m_axi_ddr_bus_o (m_axi_ddr_lo_cast ),
  .m_axi_ddr_bus_i (m_axi_ddr_li_cast )
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
