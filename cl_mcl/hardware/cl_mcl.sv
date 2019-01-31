module cl_mcl
(
   `include "cl_ports.vh"
);

// For some silly reason, you need to leave this up here...
logic rst_main_n_sync;

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


localparam axil_slv_num_lp = 2;
localparam mcl_width_lp = 80;

// fsb slave
//-------------------------------------------------
logic [axil_slv_num_lp-1:0] mcl_v_i, mcl_v_o;
logic [(mcl_width_lp*axil_slv_num_lp)-1:0] mcl_data_i, mcl_data_o;
logic [axil_slv_num_lp-1:0] mcl_yumi_o, mcl_ready_i;

(* dont_touch = "true" *) logic axi_mcl_rstn;
lib_pipe #(.WIDTH(1), .STAGES(4)) AXI_MCL_RST_N (.clk(clk_main_a0), .rst_n(1'b1), .in_bus(rst_main_n_sync), .out_bus(axi_mcl_rstn));
axil_to_mcl #(.mcl_intf_num_p(axil_slv_num_lp)) DUT (
  .clk_i         (clk_main_a0         ),
  .reset_i       (~axi_mcl_rstn ),
  .s_axil_mcl_awvalid(sh_ocl_awvalid),
  .s_axil_mcl_awaddr (sh_ocl_awaddr ),
  .s_axil_mcl_awready(ocl_sh_awready),
  .s_axil_mcl_wvalid (sh_ocl_wvalid ),
  .s_axil_mcl_wdata  (sh_ocl_wdata  ),
  .s_axil_mcl_wstrb  (sh_ocl_wstrb  ),
  .s_axil_mcl_wready (ocl_sh_wready ),
  .s_axil_mcl_bresp  (ocl_sh_bresp  ),
  .s_axil_mcl_bvalid (ocl_sh_bvalid ),
  .s_axil_mcl_bready (sh_ocl_bready ),
  .s_axil_mcl_araddr (sh_ocl_araddr ),
  .s_axil_mcl_arvalid(sh_ocl_arvalid),
  .s_axil_mcl_arready(ocl_sh_arready),
  .s_axil_mcl_rdata  (ocl_sh_rdata  ),
  .s_axil_mcl_rresp  (ocl_sh_rresp  ),
  .s_axil_mcl_rvalid (ocl_sh_rvalid ),
  .s_axil_mcl_rready (sh_ocl_rready ),
  .mcl_v_i    (mcl_v_i    ),
  .mcl_data_i (mcl_data_i ),
  .mcl_yumi_o (mcl_yumi_o ),
  .mcl_v_o    (mcl_v_o    ),
  .mcl_data_o (mcl_data_o ),
  .mcl_ready_i(mcl_ready_i)
);

//---------------------------------------------------------------
//                    axi - fsb adapters                        |
//                                                              |
//---------------------------------------------------------------
genvar i;
for (i=0;i<axil_slv_num_lp;i=i+1)
begin: bsg_test_node_slv
  cl_simple_loopback #(
    .data_width_p(80        ),
    .mask_p      ({80{1'b1}})
  ) mcl_loopback_node (
    .clk_i  (clk_main_a0                             ),
    .reset_i(~axi_mcl_rstn                           ),
    .en_i   (1'b1                                    ),
    // input channel
    .v_i    (mcl_v_o[i]                              ),
    .data_i (mcl_data_o[mcl_width_lp*i+:mcl_width_lp]),
    .ready_o(mcl_ready_i[i]                          ),
    // output channel
    .v_o    (mcl_v_i[i]                              ),
    .data_o (mcl_data_i[mcl_width_lp*i+:mcl_width_lp]),
    .yumi_i (mcl_yumi_o[i]                           )
  );
end

endmodule
