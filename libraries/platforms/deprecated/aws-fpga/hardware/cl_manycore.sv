// Copyright (c) 2019, University of Washington All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this list
// of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
//
// Neither the name of the copyright holder nor the names of its contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/**
 * This is the top-level module for the user logic in AWS (a.k.a. Custom Logic, or CL).
 *
 * The BSG Nonsynth DPI interfaces that bind to this module depend on
 * the hierarchy path that defines this module instance
 * (tb.card.fpga.CL) and sub-hierarchy that is defined below this
 * module (e.g. CL.network.manycore_wrapper.manycore). If any
 * modifications to the Hierarchy Path to the Vanilla Core Instances
 * change, the hierarchy paths in bsg_manycore_platform.cpp will need
 * to change.
 *
 */

module cl_manycore
  import bsg_manycore_pkg::*;
  import bsg_manycore_addr_pkg::*;
  import bsg_bladerunner_pkg::*;
  import bsg_bladerunner_mem_cfg_pkg::*;
  import bsg_manycore_network_cfg_pkg::*;
   (
  `include "cl_ports.vh"
    );

   // For some silly reason, you need to leave this up here...
   logic rst_main_n_sync;

`include "bsg_defines.v"
`include "cl_id_defines.vh"
`include "cl_manycore_defines.vh"

   localparam axi_id_width_lp = 6;
   localparam axi_addr_width_lp = 64;
   localparam axi_data_width_lp = 512;
   localparam axi_strb_width_lp = (axi_data_width_lp>>3);
   localparam axi_burst_len_lp = 1;

   // Width of the host packets
   localparam host_io_pkt_width_lp = 128;

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
`include "unused_dma_pcis_template.inc"
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
          pre_sync_rst_n  <= 1'b0;
          rst_main_n_sync <= 1'b0;
       end
     else
       begin
          pre_sync_rst_n  <= 1'b1;
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
   logic [5:0]  m_axi4_manycore_awid;
   logic [63:0] m_axi4_manycore_awaddr;
   logic [7:0]  m_axi4_manycore_awlen;
   logic [2:0]  m_axi4_manycore_awsize;
   logic [1:0]  m_axi4_manycore_awburst;
   logic [0:0]  m_axi4_manycore_awlock;
   logic [3:0]  m_axi4_manycore_awcache;
   logic [2:0]  m_axi4_manycore_awprot;
   logic [3:0]  m_axi4_manycore_awregion;
   logic [3:0]  m_axi4_manycore_awqos;
   logic        m_axi4_manycore_awvalid;
   logic        m_axi4_manycore_awready;

   logic [511:0] m_axi4_manycore_wdata;
   logic [63:0]  m_axi4_manycore_wstrb;
   logic         m_axi4_manycore_wlast;
   logic         m_axi4_manycore_wvalid;
   logic         m_axi4_manycore_wready;

   logic [5:0]   m_axi4_manycore_bid;
   logic [1:0]   m_axi4_manycore_bresp;
   logic         m_axi4_manycore_bvalid;
   logic         m_axi4_manycore_bready;

   logic [5:0]   m_axi4_manycore_arid;
   logic [63:0]  m_axi4_manycore_araddr;
   logic [7:0]   m_axi4_manycore_arlen;
   logic [2:0]   m_axi4_manycore_arsize;
   logic [1:0]   m_axi4_manycore_arburst;
   logic [0:0]   m_axi4_manycore_arlock;
   logic [3:0]   m_axi4_manycore_arcache;
   logic [2:0]   m_axi4_manycore_arprot;
   logic [3:0]   m_axi4_manycore_arregion;
   logic [3:0]   m_axi4_manycore_arqos;
   logic         m_axi4_manycore_arvalid;
   logic         m_axi4_manycore_arready;

   logic [5:0]   m_axi4_manycore_rid;
   logic [511:0] m_axi4_manycore_rdata;
   logic [1:0]   m_axi4_manycore_rresp;
   logic         m_axi4_manycore_rlast;
   logic         m_axi4_manycore_rvalid;
   logic         m_axi4_manycore_rready;


   //--------------------------------------------
   // AXI-Lite OCL System
   //---------------------------------------------
   axi_register_slice_light
     AXIL_OCL_REG_SLC
       (
        .aclk          (clk_main_a0),
        .aresetn       (rst_main_n_sync),
        .s_axi_awaddr  (sh_ocl_awaddr),
        .s_axi_awprot  (3'h0),
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
        .s_axi_arprot  ('0),
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
        .m_axi_arprot  (),
        .m_axi_arvalid (m_axil_ocl_arvalid),
        .m_axi_arready (m_axil_ocl_arready),
        .m_axi_rdata   (m_axil_ocl_rdata),
        .m_axi_rresp   (m_axil_ocl_rresp),
        .m_axi_rvalid  (m_axil_ocl_rvalid),
        .m_axi_rready  (m_axil_ocl_rready)
        );


   logic         core_clk;
   logic         core_reset;
   assign core_clk = clk_main_a0;
   assign core_reset = ~rst_main_n_sync;


   `declare_bsg_manycore_link_sif_s(bsg_machine_noc_epa_width_gp, bsg_machine_noc_data_width_gp, bsg_machine_noc_x_coord_width_gp, bsg_machine_noc_y_coord_width_gp);

   bsg_manycore_link_sif_s [bsg_machine_llcache_num_cache_gp-1:0] cache_link_sif_li;
   bsg_manycore_link_sif_s [bsg_machine_llcache_num_cache_gp-1:0] cache_link_sif_lo;

   logic [bsg_machine_llcache_num_cache_gp-1:0][bsg_machine_noc_x_coord_width_gp-1:0] cache_x_lo;
   logic [bsg_machine_llcache_num_cache_gp-1:0][bsg_machine_noc_y_coord_width_gp-1:0] cache_y_lo;

   bsg_manycore_link_sif_s loader_link_sif_li;
   bsg_manycore_link_sif_s loader_link_sif_lo;

   localparam logic [e_network_max_val-1:0] network_cfg_lp = (1 << bsg_machine_noc_cfg_gp);

   if(network_cfg_lp[e_network_mesh]) begin

   bsg_manycore_wrapper_mesh
     #(
       .addr_width_p(bsg_machine_noc_epa_width_gp)
       ,.data_width_p(bsg_machine_noc_data_width_gp)
       ,.num_tiles_x_p(bsg_machine_num_cores_x_gp)
       ,.num_tiles_y_p(bsg_machine_num_cores_y_gp)
       ,.dmem_words_p(bsg_machine_core_dmem_words_gp)
       ,.icache_entries_p(bsg_machine_core_icache_entries_gp)
       ,.icache_tag_width_p(bsg_machine_core_icache_tag_width_gp)
       ,.num_cache_p(bsg_machine_llcache_num_cache_gp)
       ,.vcache_size_p(bsg_machine_llcache_words_gp)
       ,.vcache_block_size_in_words_p(bsg_machine_llcache_line_words_gp)
       ,.vcache_sets_p(bsg_machine_llcache_sets_gp)
       ,.branch_trace_en_p(bsg_machine_branch_trace_en_gp)
       ,.hetero_type_vec_p(bsg_machine_hetero_type_vec_gp)
       )
   manycore_wrapper
     (
      .clk_i(core_clk)
      ,.reset_i(core_reset)

      ,.cache_link_sif_i(cache_link_sif_li)
      ,.cache_link_sif_o(cache_link_sif_lo)

      ,.cache_x_o(cache_x_lo)
      ,.cache_y_o(cache_y_lo)

      ,.loader_link_sif_i(loader_link_sif_li)
      ,.loader_link_sif_o(loader_link_sif_lo)
      );

   end else
      $error(1, "aws-fgpa and aws-vcs only suppor network_cfg_lp == e_network_mesh");


  // synopsys translate_off
  // print stat signals for vanilla_core_profiler module
  logic print_stat_v;
  logic [bsg_machine_noc_data_width_gp-1:0] print_stat_tag;

  bsg_print_stat_snoop #(
    .data_width_p(bsg_machine_noc_data_width_gp)
    ,.addr_width_p(bsg_machine_noc_epa_width_gp)
    ,.x_cord_width_p(bsg_machine_noc_x_coord_width_gp)
    ,.y_cord_width_p(bsg_machine_noc_y_coord_width_gp)
  ) print_stat_snoop0 (
    .loader_link_sif_in_i(loader_link_sif_lo)
    ,.loader_link_sif_out_i(loader_link_sif_li)

    ,.print_stat_v_o(print_stat_v)
    ,.print_stat_tag_o(print_stat_tag)
  );
  // synopsys translate_on

  ////////////////////////////////
  // Configurable Memory System //
  ////////////////////////////////
  localparam byte_offset_width_lp=`BSG_SAFE_CLOG2(bsg_machine_noc_data_width_gp>>3);
  localparam cache_addr_width_lp=(bsg_machine_noc_epa_width_gp-1+byte_offset_width_lp);

  if (bsg_machine_dram_cfg_gp == e_vcache_blocking_axi4_f1_dram
    || bsg_machine_dram_cfg_gp == e_vcache_blocking_axi4_f1_model
    || bsg_machine_dram_cfg_gp == e_vcache_non_blocking_axi4_f1_dram
    || bsg_machine_dram_cfg_gp == e_vcache_non_blocking_axi4_f1_model) begin: lv1_dma

    // for now blocking and non-blocking shares the same wire, since interface is
    // the same. But it might change in the future.

    import bsg_cache_pkg::*;
    localparam dma_pkt_width_lp = `bsg_cache_dma_pkt_width(cache_addr_width_lp);

    logic [bsg_machine_llcache_num_cache_gp-1:0][dma_pkt_width_lp-1:0] dma_pkt;
    logic [bsg_machine_llcache_num_cache_gp-1:0] dma_pkt_v_lo;
    logic [bsg_machine_llcache_num_cache_gp-1:0] dma_pkt_yumi_li;

    logic [bsg_machine_llcache_num_cache_gp-1:0][bsg_machine_llcache_channel_width_gp-1:0] dma_data_li;
    logic [bsg_machine_llcache_num_cache_gp-1:0] dma_data_v_li;
    logic [bsg_machine_llcache_num_cache_gp-1:0] dma_data_ready_lo;

    logic [bsg_machine_llcache_num_cache_gp-1:0][bsg_machine_llcache_channel_width_gp-1:0] dma_data_lo;
    logic [bsg_machine_llcache_num_cache_gp-1:0] dma_data_v_lo;
    logic [bsg_machine_llcache_num_cache_gp-1:0] dma_data_yumi_li;

  end
   // LEVEL 1
  if (bsg_machine_dram_cfg_gp == e_vcache_blocking_axi4_f1_dram ||
           bsg_machine_dram_cfg_gp == e_vcache_blocking_axi4_f1_model) begin: lv1_vcache

    for (genvar i = 0; i < bsg_machine_llcache_num_cache_gp; i++) begin: vcache

      bsg_manycore_vcache_blocking #(
        .data_width_p(bsg_machine_noc_data_width_gp)
        ,.addr_width_p(bsg_machine_noc_epa_width_gp)
        ,.block_size_in_words_p(bsg_machine_llcache_line_words_gp)
        ,.sets_p(bsg_machine_llcache_sets_gp)
        ,.ways_p(bsg_machine_llcache_ways_gp)
        ,.dma_data_width_p(bsg_machine_llcache_channel_width_gp)
        ,.x_cord_width_p(bsg_machine_noc_x_coord_width_gp)
        ,.y_cord_width_p(bsg_machine_noc_y_coord_width_gp)
      ) vcache (
        .clk_i(core_clk)
        ,.reset_i(core_reset)
        // memory systems link from bsg_manycore_wrapper
        ,.link_sif_i(cache_link_sif_lo[i])
        ,.link_sif_o(cache_link_sif_li[i])

        ,.dma_pkt_o(lv1_dma.dma_pkt[i])
        ,.dma_pkt_v_o(lv1_dma.dma_pkt_v_lo[i])
        ,.dma_pkt_yumi_i(lv1_dma.dma_pkt_yumi_li[i])

        ,.dma_data_i(lv1_dma.dma_data_li[i])
        ,.dma_data_v_i(lv1_dma.dma_data_v_li[i])
        ,.dma_data_ready_o(lv1_dma.dma_data_ready_lo[i])

        ,.dma_data_o(lv1_dma.dma_data_lo[i])
        ,.dma_data_v_o(lv1_dma.dma_data_v_lo[i])
        ,.dma_data_yumi_i(lv1_dma.dma_data_yumi_li[i])
     );

    end

    // synopsys translate_off

    bind bsg_cache vcache_profiler #(
      .data_width_p(data_width_p)
      ,.addr_width_p(addr_width_p)
      ,.header_print_p("vcache[0]")
      ,.ways_p(ways_p)
    ) vcache_prof (
      .*

      // bsg_cache_miss
      ,.chosen_way_n(miss.chosen_way_n)

      ,.global_ctr_i($root.tb.card.fpga.CL.global_ctr)
      ,.print_stat_v_i($root.tb.card.fpga.CL.print_stat_v)
      ,.print_stat_tag_i($root.tb.card.fpga.CL.print_stat_tag)
      ,.trace_en_i($root.tb.card.fpga.CL.trace_en)
    );
    // synopsys translate_on

  end // block: lv1_vcache
  else if (bsg_machine_dram_cfg_gp == e_vcache_non_blocking_axi4_f1_dram ||
           bsg_machine_dram_cfg_gp == e_vcache_non_blocking_axi4_f1_model) begin: lv1_vcache_nb

    for (genvar i = 0; i < bsg_machine_llcache_num_cache_gp; i++) begin: vcache
      bsg_manycore_vcache_non_blocking #(
        .data_width_p(bsg_machine_noc_data_width_gp)
        ,.addr_width_p(bsg_machine_noc_epa_width_gp)
        ,.block_size_in_words_p(bsg_machine_llcache_line_words_gp)
        ,.sets_p(bsg_machine_llcache_sets_gp)
        ,.ways_p(bsg_machine_llcache_ways_gp)

        ,.miss_fifo_els_p(bsg_machine_llcache_miss_fifo_els_gp)
        ,.x_cord_width_p(bsg_machine_noc_x_coord_width_gp)
        ,.y_cord_width_p(bsg_machine_noc_y_coord_width_gp)
      ) vcache_nb (
        .clk_i(core_clk)
        ,.reset_i(core_reset)

        ,.link_sif_i(cache_link_sif_lo[i])
        ,.link_sif_o(cache_link_sif_li[i])

        ,.dma_pkt_o(lv1_dma.dma_pkt[i])
        ,.dma_pkt_v_o(lv1_dma.dma_pkt_v_lo[i])
        ,.dma_pkt_yumi_i(lv1_dma.dma_pkt_yumi_li[i])

        ,.dma_data_i(lv1_dma.dma_data_li[i])
        ,.dma_data_v_i(lv1_dma.dma_data_v_li[i])
        ,.dma_data_ready_o(lv1_dma.dma_data_ready_lo[i])

        ,.dma_data_o(lv1_dma.dma_data_lo[i])
        ,.dma_data_v_o(lv1_dma.dma_data_v_lo[i])
        ,.dma_data_yumi_i(lv1_dma.dma_data_yumi_li[i])
      );

    end

    // synopsys translate_off

    bind bsg_cache_non_blocking vcache_non_blocking_profiler #(
      .data_width_p(data_width_p)
      ,.addr_width_p(addr_width_p)
      ,.sets_p(sets_p)
      ,.ways_p(ways_p)
      ,.id_width_p(id_width_p)
      ,.block_size_in_words_p(block_size_in_words_p)
    ) vcache_prof (
      .*
      ,.replacement_dirty(mhu0.replacement_dirty)
      ,.replacement_valid(mhu0.replacement_valid)
      ,.global_ctr_i($root.tb.card.fpga.CL.global_ctr)
      ,.print_stat_v_i($root.tb.card.fpga.CL.print_stat_v_lo)
      ,.print_stat_tag_i($root.tb.card.fpga.CL.print_stat_tag_lo)
    );
    // synopsys translate_on

  end


  // LEVEL 2
  //
  if (bsg_machine_dram_cfg_gp == e_vcache_blocking_axi4_f1_dram ||
      bsg_machine_dram_cfg_gp == e_vcache_blocking_axi4_f1_model ||
      bsg_machine_dram_cfg_gp == e_vcache_non_blocking_axi4_f1_dram ||
      bsg_machine_dram_cfg_gp == e_vcache_non_blocking_axi4_f1_model) begin: lv2_axi4

    logic [axi_id_width_lp-1:0] axi_awid;
    logic [axi_addr_width_lp-1:0] axi_awaddr;
    logic [7:0] axi_awlen;
    logic [2:0] axi_awsize;
    logic [1:0] axi_awburst;
    logic [3:0] axi_awcache;
    logic [2:0] axi_awprot;
    logic axi_awlock;
    logic axi_awvalid;
    logic axi_awready;

    logic [axi_data_width_lp-1:0] axi_wdata;
    logic [axi_strb_width_lp-1:0] axi_wstrb;
    logic axi_wlast;
    logic axi_wvalid;
    logic axi_wready;

    logic [axi_id_width_lp-1:0] axi_bid;
    logic [1:0] axi_bresp;
    logic axi_bvalid;
    logic axi_bready;

    logic [axi_id_width_lp-1:0] axi_arid;
    logic [axi_addr_width_lp-1:0] axi_araddr;
    logic [7:0] axi_arlen;
    logic [2:0] axi_arsize;
    logic [1:0] axi_arburst;
    logic [3:0] axi_arcache;
    logic [2:0] axi_arprot;
    logic axi_arlock;
    logic axi_arvalid;
    logic axi_arready;

    logic [axi_id_width_lp-1:0] axi_rid;
    logic [axi_data_width_lp-1:0] axi_rdata;
    logic [1:0] axi_rresp;
    logic axi_rlast;
    logic axi_rvalid;
    logic axi_rready;

    bsg_cache_to_axi_hashed #(
      .addr_width_p(cache_addr_width_lp)
      ,.block_size_in_words_p(bsg_machine_llcache_line_words_gp)
      ,.data_width_p(bsg_machine_noc_data_width_gp)
      ,.num_cache_p(bsg_machine_llcache_num_cache_gp)

      ,.axi_id_width_p(axi_id_width_lp)
      ,.axi_addr_width_p(axi_addr_width_lp)
      ,.axi_data_width_p(axi_data_width_lp)
      ,.axi_burst_len_p(axi_burst_len_lp)
    ) cache_to_axi (
      .clk_i(core_clk)
      ,.reset_i(core_reset)

      ,.dma_pkt_i(lv1_dma.dma_pkt)
      ,.dma_pkt_v_i(lv1_dma.dma_pkt_v_lo)
      ,.dma_pkt_yumi_o(lv1_dma.dma_pkt_yumi_li)

      ,.dma_data_o(lv1_dma.dma_data_li)
      ,.dma_data_v_o(lv1_dma.dma_data_v_li)
      ,.dma_data_ready_i(lv1_dma.dma_data_ready_lo)

      ,.dma_data_i(lv1_dma.dma_data_lo)
      ,.dma_data_v_i(lv1_dma.dma_data_v_lo)
      ,.dma_data_yumi_o(lv1_dma.dma_data_yumi_li)

      ,.axi_awid_o(axi_awid)
      ,.axi_awaddr_o(axi_awaddr)
      ,.axi_awlen_o(axi_awlen)
      ,.axi_awsize_o(axi_awsize)
      ,.axi_awburst_o(axi_awburst)
      ,.axi_awcache_o(axi_awcache)
      ,.axi_awprot_o(axi_awprot)
      ,.axi_awlock_o(axi_awlock)
      ,.axi_awvalid_o(axi_awvalid)
      ,.axi_awready_i(axi_awready)

      ,.axi_wdata_o(axi_wdata)
      ,.axi_wstrb_o(axi_wstrb)
      ,.axi_wlast_o(axi_wlast)
      ,.axi_wvalid_o(axi_wvalid)
      ,.axi_wready_i(axi_wready)

      ,.axi_bid_i(axi_bid)
      ,.axi_bresp_i(axi_bresp)
      ,.axi_bvalid_i(axi_bvalid)
      ,.axi_bready_o(axi_bready)

      ,.axi_arid_o(axi_arid)
      ,.axi_araddr_o(axi_araddr)
      ,.axi_arlen_o(axi_arlen)
      ,.axi_arsize_o(axi_arsize)
      ,.axi_arburst_o(axi_arburst)
      ,.axi_arcache_o(axi_arcache)
      ,.axi_arprot_o(axi_arprot)
      ,.axi_arlock_o(axi_arlock)
      ,.axi_arvalid_o(axi_arvalid)
      ,.axi_arready_i(axi_arready)

      ,.axi_rid_i(axi_rid)
      ,.axi_rdata_i(axi_rdata)
      ,.axi_rresp_i(axi_rresp)
      ,.axi_rlast_i(axi_rlast)
      ,.axi_rvalid_i(axi_rvalid)
      ,.axi_rready_o(axi_rready)
    );

  end // block: lv2_axi4

  // LEVEL 3
  //
  if (bsg_machine_dram_cfg_gp == e_vcache_blocking_axi4_f1_dram ||
      bsg_machine_dram_cfg_gp == e_vcache_blocking_axi4_f1_model ||
      bsg_machine_dram_cfg_gp == e_vcache_non_blocking_axi4_f1_dram ||
      bsg_machine_dram_cfg_gp == e_vcache_non_blocking_axi4_f1_model) begin
   // Attach cache to output DRAM

   // AXI Address Write signals
   assign m_axi4_manycore_awid          = lv2_axi4.axi_awid;
   assign m_axi4_manycore_awaddr        = lv2_axi4.axi_awaddr;
   assign m_axi4_manycore_awvalid       = lv2_axi4.axi_awvalid;
   assign lv2_axi4.axi_awready          = m_axi4_manycore_awready;
   assign m_axi4_manycore_awlen         = lv2_axi4.axi_awlen;
   assign m_axi4_manycore_awsize        = lv2_axi4.axi_awsize;
   assign m_axi4_manycore_awburst       = lv2_axi4.axi_awburst;
   assign m_axi4_manycore_awcache       = lv2_axi4.axi_awcache;
   assign m_axi4_manycore_awprot        = lv2_axi4.axi_awprot;
   assign m_axi4_manycore_awlock        = lv2_axi4.axi_awlock;
   assign m_axi4_manycore_awregion      = 4'b0;
   assign m_axi4_manycore_awqos         = 4'b0;

   // AXI Write signals
   assign m_axi4_manycore_wdata         = lv2_axi4.axi_wdata;
   assign m_axi4_manycore_wstrb         = lv2_axi4.axi_wstrb;
   assign m_axi4_manycore_wlast         = lv2_axi4.axi_wlast;
   assign m_axi4_manycore_wvalid        = lv2_axi4.axi_wvalid;
   assign lv2_axi4.axi_wready           = m_axi4_manycore_wready;

   // AXI Burst signals
   assign lv2_axi4.axi_bid              = m_axi4_manycore_bid;
   assign lv2_axi4.axi_bresp            = m_axi4_manycore_bresp;
   assign lv2_axi4.axi_bvalid           = m_axi4_manycore_bvalid;
   assign m_axi4_manycore_bready        = lv2_axi4.axi_bready;

   // AXI Address Read signals
   assign m_axi4_manycore_arid          = lv2_axi4.axi_arid;
   assign m_axi4_manycore_araddr        = lv2_axi4.axi_araddr;
   assign m_axi4_manycore_arlen         = lv2_axi4.axi_arlen;
   assign m_axi4_manycore_arsize        = lv2_axi4.axi_arsize;
   assign m_axi4_manycore_arburst       = lv2_axi4.axi_arburst;
   assign m_axi4_manycore_arcache       = lv2_axi4.axi_arcache;
   assign m_axi4_manycore_arprot        = lv2_axi4.axi_arprot;
   assign m_axi4_manycore_arlock        = lv2_axi4.axi_arlock;
   assign m_axi4_manycore_arvalid       = lv2_axi4.axi_arvalid;
   assign lv2_axi4.axi_arready          = m_axi4_manycore_arready;
   assign m_axi4_manycore_arregion      = 4'b0;
   assign m_axi4_manycore_arqos         = 4'b0;

   // AXI Read signals
   assign lv2_axi4.axi_rid              = m_axi4_manycore_rid;
   assign lv2_axi4.axi_rdata            = m_axi4_manycore_rdata;
   assign lv2_axi4.axi_rresp            = m_axi4_manycore_rresp;
   assign lv2_axi4.axi_rlast            = m_axi4_manycore_rlast;
   assign lv2_axi4.axi_rvalid           = m_axi4_manycore_rvalid;
   assign m_axi4_manycore_rready        = lv2_axi4.axi_rready;
  end

   //--------------------------------------------
   // AXI4 Manycore System
   //---------------------------------------------
   assign m_axi4_manycore_rid = sh_cl_ddr_rid;
   assign m_axi4_manycore_rdata = sh_cl_ddr_rdata;
   assign m_axi4_manycore_rresp = sh_cl_ddr_rresp;
   assign m_axi4_manycore_rlast = sh_cl_ddr_rlast;
   assign m_axi4_manycore_rvalid = sh_cl_ddr_rvalid;
   assign cl_sh_ddr_rready = m_axi4_manycore_rready;

   assign cl_sh_ddr_awid = m_axi4_manycore_awid;
   assign cl_sh_ddr_awaddr = m_axi4_manycore_awaddr;
   assign cl_sh_ddr_awlen = m_axi4_manycore_awlen;
   assign cl_sh_ddr_awsize = m_axi4_manycore_awsize;
   assign cl_sh_ddr_awburst = m_axi4_manycore_awburst;
   assign cl_sh_ddr_awlock = m_axi4_manycore_awlock;
   assign cl_sh_ddr_awcache = m_axi4_manycore_awcache;
   assign cl_sh_ddr_awprot = m_axi4_manycore_awprot;
   assign cl_sh_ddr_awregion = m_axi4_manycore_awregion;
   assign cl_sh_ddr_awqos = m_axi4_manycore_awqos;
   assign cl_sh_ddr_awvalid = m_axi4_manycore_awvalid;
   assign m_axi4_manycore_awready = sh_cl_ddr_awready;

   assign cl_sh_ddr_wdata = m_axi4_manycore_wdata;
   assign cl_sh_ddr_wstrb = m_axi4_manycore_wstrb;
   assign cl_sh_ddr_wlast = m_axi4_manycore_wlast;
   assign cl_sh_ddr_wvalid = m_axi4_manycore_wvalid;
   assign m_axi4_manycore_wready = sh_cl_ddr_wready;

   assign m_axi4_manycore_bid = sh_cl_ddr_bid;
   assign m_axi4_manycore_bresp = sh_cl_ddr_bresp;
   assign m_axi4_manycore_bvalid = sh_cl_ddr_bvalid;
   assign cl_sh_ddr_bready = m_axi4_manycore_bready;

   assign cl_sh_ddr_arid[5:0] = m_axi4_manycore_arid;
   assign cl_sh_ddr_araddr = m_axi4_manycore_araddr;
   assign cl_sh_ddr_arlen = m_axi4_manycore_arlen;
   assign cl_sh_ddr_arsize = m_axi4_manycore_arsize;
   assign cl_sh_ddr_arburst = m_axi4_manycore_arburst;
   assign cl_sh_ddr_arlock = m_axi4_manycore_arlock;
   assign cl_sh_ddr_arcache = m_axi4_manycore_arcache;
   assign cl_sh_ddr_arprot = m_axi4_manycore_arprot;
   assign cl_sh_ddr_arregion = m_axi4_manycore_arregion;
   assign cl_sh_ddr_arqos = m_axi4_manycore_arqos;
   assign cl_sh_ddr_arvalid = m_axi4_manycore_arvalid;
   assign m_axi4_manycore_arready = sh_cl_ddr_arready;


   localparam core_cycle_ctr_width_lp = 64;
   logic [core_cycle_ctr_width_lp-1:0] core_cycle_ctr_l;

   bsg_cycle_counter
     #(.width_p(core_cycle_ctr_width_lp))
   core_cc
     (.clk_i(core_clk)
      ,.reset_i(core_reset)
      ,.ctr_r_o(core_cycle_ctr_l)
      );

   // manycore link

   logic [bsg_machine_noc_x_coord_width_gp-1:0] mcl_x_cord_li = (bsg_machine_noc_x_coord_width_gp)'(bsg_machine_io_coord_x_gp);
   logic [bsg_machine_noc_y_coord_width_gp-1:0] mcl_y_cord_li = (bsg_machine_noc_y_coord_width_gp)'(bsg_machine_io_coord_y_gp);

   logic                                        print_stat_v_lo;
   logic [bsg_machine_noc_data_width_gp-1:0]    print_stat_tag_lo;

   bsg_manycore_link_sif_s axil_link_sif_li;
   bsg_manycore_link_sif_s axil_link_sif_lo;

  bsg_manycore_link_to_axil #(
    .x_cord_width_p     (bsg_machine_noc_x_coord_width_gp)
    ,.y_cord_width_p     (bsg_machine_noc_y_coord_width_gp)
    ,.addr_width_p       (bsg_machine_noc_epa_width_gp)
    ,.data_width_p       (bsg_machine_noc_data_width_gp)
    ,.host_io_pkt_width_p(host_io_pkt_width_lp)
    ,.host_io_pkts_tx_p  (bsg_machine_io_credits_max_gp)
    ,.host_io_pkts_rx_p  (bsg_machine_io_pkts_max_gp)
    ,.max_out_credits_p  (bsg_machine_io_credits_max_gp)
    ,.cycle_width_p(core_cycle_ctr_width_lp)
  ) mcl_to_axil (
    .clk_i           (clk_main_a0       )
    ,.reset_i         (~rst_main_n_sync  )
    // axil slave interface
    ,.axil_awvalid_i  (m_axil_ocl_awvalid)
    ,.axil_awaddr_i   (m_axil_ocl_awaddr )
    ,.axil_awready_o  (m_axil_ocl_awready)
    ,.axil_wvalid_i   (m_axil_ocl_wvalid )
    ,.axil_wdata_i    (m_axil_ocl_wdata  )
    ,.axil_wstrb_i    (m_axil_ocl_wstrb  )
    ,.axil_wready_o   (m_axil_ocl_wready )
    ,.axil_bresp_o    (m_axil_ocl_bresp  )
    ,.axil_bvalid_o   (m_axil_ocl_bvalid )
    ,.axil_bready_i   (m_axil_ocl_bready )
    ,.axil_araddr_i   (m_axil_ocl_araddr )
    ,.axil_arvalid_i  (m_axil_ocl_arvalid)
    ,.axil_arready_o  (m_axil_ocl_arready)
    ,.axil_rdata_o    (m_axil_ocl_rdata  )
    ,.axil_rresp_o    (m_axil_ocl_rresp  )
    ,.axil_rvalid_o   (m_axil_ocl_rvalid )
    ,.axil_rready_i   (m_axil_ocl_rready )
    // manycore link
    ,.link_sif_i      (axil_link_sif_li  )
    ,.link_sif_o      (axil_link_sif_lo  )
    ,.my_x_i          (mcl_x_cord_li     )
    ,.my_y_i          (mcl_y_cord_li     )
    ,.cycle_ctr_i       (core_cycle_ctr_l)
  );

   assign axil_link_sif_li = loader_link_sif_lo;
   assign loader_link_sif_li = axil_link_sif_lo;

   //-----------------------------------------------
   // Debug bridge, used if need Virtual JTAG
   //-----------------------------------------------
`ifndef DISABLE_VJTAG_DEBUG

   // Flop for timing global clock counter
   logic [63:0]               sh_cl_glcount0_q;

   always_ff @(posedge clk_main_a0)
     if (!rst_main_n_sync)
       sh_cl_glcount0_q <= 0;
     else
       sh_cl_glcount0_q <= sh_cl_glcount0;


   // Integrated Logic Analyzers (ILA)
   ila_0 CL_ILA_0
     (
      .clk    (clk_main_a0),
      .probe0 (m_axil_ocl_awvalid)
      ,.probe1 (64'(m_axil_ocl_awaddr))
      ,.probe2 (m_axil_ocl_awready)
      ,.probe3 (m_axil_ocl_arvalid)
      ,.probe4 (64'(m_axil_ocl_araddr))
      ,.probe5 (m_axil_ocl_arready)
      );

   ila_0 CL_ILA_1
     (
      .clk    (clk_main_a0)
      ,.probe0 (m_axil_ocl_bvalid)
      ,.probe1 (sh_cl_glcount0_q)
      ,.probe2 (m_axil_ocl_bready)
      ,.probe3 (m_axil_ocl_rvalid)
      ,.probe4 ({32'b0,m_axil_ocl_rdata[31:0]})
      ,.probe5 (m_axil_ocl_rready)
      );

   // Debug Bridge
   cl_debug_bridge CL_DEBUG_BRIDGE
     (
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

   // synopsys translate off
   logic arg_trace_en;
   logic trace_en;
   logic log_en;
   logic dpi_trace_en;
   logic dpi_log_en;
   initial begin
      assign arg_trace_en = $test$plusargs("trace");
   end

   assign trace_en = arg_trace_en | dpi_trace_en;
   assign log_en = arg_trace_en | dpi_log_en;

   bsg_nonsynth_dpi_gpio
     #(
       .width_p(2)
       ,.init_o_p('0)
       ,.use_output_p('1)
       ,.debpug_p('0)
       )
   trace_control
     (
       .gpio_o({dpi_log_en, dpi_trace_en})
       ,.gpio_i('0)
       );

   bind vanilla_core vanilla_core_trace
     #(
       .x_cord_width_p(x_cord_width_p)
       ,.y_cord_width_p(y_cord_width_p)
       ,.icache_tag_width_p(icache_tag_width_p)
       ,.icache_entries_p(icache_entries_p)
       ,.data_width_p(data_width_p)
       ,.dmem_size_p(dmem_size_p)
       )
   vtrace
     (
      .*
      ,.trace_en_i($root.tb.card.fpga.CL.log_en)
      );


   // profilers
   //
   logic [31:0] global_ctr;

   bsg_cycle_counter global_cc
     (
      .clk_i(core_clk)
      ,.reset_i(core_reset)
      ,.ctr_r_o(global_ctr)
      );


   bind vanilla_core vanilla_core_profiler
     #(
       .x_cord_width_p(x_cord_width_p)
       ,.y_cord_width_p(y_cord_width_p)
       ,.origin_x_cord_p(`BSG_MACHINE_ORIGIN_COORD_X)
       ,.origin_y_cord_p(`BSG_MACHINE_ORIGIN_COORD_Y)
       ,.icache_tag_width_p(icache_tag_width_p)
       ,.icache_entries_p(icache_entries_p)
       ,.data_width_p(data_width_p)
       )
   vcore_prof
     (
      .*
      ,.global_ctr_i($root.tb.card.fpga.CL.global_ctr)
      ,.print_stat_v_i($root.tb.card.fpga.CL.print_stat_v)
      ,.print_stat_tag_i($root.tb.card.fpga.CL.print_stat_tag)
      ,.trace_en_i($root.tb.card.fpga.CL.trace_en)
      );

   // synopsys translate on

endmodule
