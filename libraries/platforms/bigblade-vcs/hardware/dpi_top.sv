module replicant_tb_top
  import bsg_noc_pkg::*; // {P=0, W, E, N, S}

  import bsg_manycore_pkg::*;
  import bsg_manycore_addr_pkg::*;
  import bsg_bladerunner_pkg::*;
  import bsg_bladerunner_mem_cfg_pkg::*;
  import bsg_manycore_network_cfg_pkg::*;
  import bsg_manycore_endpoint_to_fifos_pkg::*;
  ();

   // Uncomment this to enable VCD Dumping
   /*
    initial begin
    $display("[%0t] Tracing to vlt_dump.vcd...\n", $time);
    $dumpfile("dump.vcd");
    $dumpvars();
   end
    */
   initial begin
      $display("==================== BSG MACHINE SETTINGS: ====================");

      $display("[INFO][TESTBENCH] bsg_machine_noc_ruche_factor_X_gp     = %d", bsg_machine_noc_ruche_factor_X_gp);
      $display("[INFO][TESTBENCH] bsg_machine_noc_epa_width_gp          = %d", bsg_machine_noc_epa_width_gp);
      $display("[INFO][TESTBENCH] bsg_machine_noc_data_width_gp         = %d", bsg_machine_noc_data_width_gp);
      $display("[INFO][TESTBENCH] bsg_machine_noc_cfg_gp                = %s", bsg_machine_noc_cfg_gp.name());

      $display("[INFO][TESTBENCH] bsg_machine_llcache_num_cache_gp      = %d", bsg_machine_llcache_num_cache_gp);
      $display("[INFO][TESTBENCH] bsg_machine_llcache_sets_gp           = %d", bsg_machine_llcache_sets_gp);
      $display("[INFO][TESTBENCH] bsg_machine_llcache_ways_gp           = %d", bsg_machine_llcache_ways_gp);
      $display("[INFO][TESTBENCH] bsg_machine_llcache_line_words_gp     = %d", bsg_machine_llcache_line_words_gp);
      $display("[INFO][TESTBENCH] bsg_machine_llcache_words_gp          = %d", bsg_machine_llcache_words_gp);
      $display("[INFO][TESTBENCH] bsg_machine_llcache_miss_fifo_els_gp  = %d", bsg_machine_llcache_miss_fifo_els_gp);

      $display("[INFO][TESTBENCH] bsg_machine_llcache_channel_width_gp  = %d", bsg_machine_llcache_channel_width_gp);
      $display("[INFO][TESTBENCH] bsg_machine_dram_bank_words_gp        = %d", bsg_machine_dram_bank_words_gp);
      $display("[INFO][TESTBENCH] bsg_machine_dram_num_channels_gp      = %d", bsg_machine_dram_num_channels_gp);
      $display("[INFO][TESTBENCH] bsg_machine_dram_cfg_gp               = %s", bsg_machine_dram_cfg_gp.name());

      $display("[INFO][TESTBENCH] bsg_machine_num_cores_x_gp            = %d", bsg_machine_num_cores_x_gp);
      $display("[INFO][TESTBENCH] bsg_machine_num_cores_y_gp            = %d", bsg_machine_num_cores_y_gp);
   end

/*
 // TODO: 
   localparam global_epa_word_addr_width_gp = 14;
 // max EPA width on global EVA. (word addr)
    localparam max_global_x_cord_width_gp = 7;
    localparam max_global_y_cord_width_gp = 7;
 */

   localparam reset_depth_lp = 3;

   localparam bsg_machine_llcache_data_width_lp = bsg_machine_noc_data_width_gp;
   localparam bsg_machine_llcache_addr_width_lp=(bsg_machine_noc_epa_width_gp-1+`BSG_SAFE_CLOG2(bsg_machine_noc_data_width_gp>>3));

   localparam bsg_machine_wh_flit_width_lp = bsg_machine_llcache_channel_width_gp;
   localparam bsg_machine_wh_ruche_factor_lp = 2;
   localparam bsg_machine_wh_cid_width_lp = `BSG_SAFE_CLOG2(bsg_machine_wh_ruche_factor_lp);
   localparam bsg_machine_wh_len_width_lp = `BSG_SAFE_CLOG2((1+bsg_machine_llcache_line_words_gp * bsg_machine_llcache_data_width_lp)/bsg_machine_llcache_channel_width_gp);
   localparam bsg_machine_wh_cord_width_lp = bsg_machine_num_cores_y_gp;


   // Parameters defined in bsg_manycore_pkg.v
   localparam bsg_machine_noc_x_coord_width_gp = max_global_x_cord_width_gp;
   localparam bsg_machine_noc_y_coord_width_gp = max_global_y_cord_width_gp;

   // TODO: (Future) It would be awesome if the clock frequency (or
   // frequencies) were specified at the machine level.
   parameter lc_cycle_time_ps_p = 1000;

   localparam global_counter_width_lp = 64;
   // Global Counter for Profilers, Tracing, Debugging
   logic [global_counter_width_lp-1:0] global_ctr;

   logic host_clk;
   logic host_reset;

   // bsg_nonsynth_clock_gen and bsg_nonsynth_reset_gen BOTH have bit
   // inputs and outputs (they're non-synthesizable). Casting between
   // logic and bit can produce unexpected edges as logic types switch
   // from X to 0/1 at Time 0 in simulation. This means that the input
   // and outputs of both modules must have type bit, AND the wires
   // between them. Therefore, we use bit_clk and bit_reset for the
   // inputs/outputs of these modules to avoid unexpected
   // negative/positive edges and other modules can choose between bit
   // version (for non-synthesizable modules) and the logic version
   // (otherwise).
   bit   bit_clk;
   bit   bit_reset;
   logic core_clk;
   logic core_reset;
   // reset is deasserted when tag programming is done.
   logic core_reset_done_lo, core_reset_done_r;

   logic mem_clk;
   logic mem_reset;

   logic cache_clk;
   logic cache_reset;

   // Snoop wires for Print Stat
   logic                                       print_stat_v;
   logic [bsg_machine_noc_data_width_gp-1:0]   print_stat_tag;

   logic [bsg_machine_noc_x_coord_width_gp-1:0] host_x_cord_li = (bsg_machine_noc_x_coord_width_gp)'(bsg_machine_io_coord_x_gp);
   logic [bsg_machine_noc_y_coord_width_gp-1:0] host_y_cord_li = (bsg_machine_noc_y_coord_width_gp)'(bsg_machine_io_coord_y_gp);

   `declare_bsg_manycore_link_sif_s(bsg_machine_noc_epa_width_gp, bsg_machine_noc_data_width_gp, bsg_machine_noc_x_coord_width_gp, bsg_machine_noc_y_coord_width_gp);

   bsg_manycore_link_sif_s host_link_sif_li;
   bsg_manycore_link_sif_s host_link_sif_lo;

   // Trace Enable wire for runtime argument to enable tracing (+trace)
   logic                                        trace_en;
   logic                                        log_en;
   logic                                        dpi_trace_en;
   logic                                        dpi_log_en;
   logic                                        tag_done_lo;
   assign trace_en = dpi_trace_en;
   assign log_en = dpi_log_en;

   // -Verilator uses a clock generator that is controlled by C/C++
   // (bsg_nonsynth_dpi_clock_gen), whereas VCS uses the normal
   // nonsynthesizable clock generator (bsg_nonsynth_clock_gen)
`ifdef VERILATOR
   bsg_nonsynth_dpi_clock_gen
`else
   bsg_nonsynth_clock_gen
`endif
     #(.cycle_time_p(lc_cycle_time_ps_p))
   core_clk_gen
     (.o(bit_clk));
   assign core_clk = bit_clk;

   bsg_nonsynth_reset_gen
     #(
       .num_clocks_p(1)
       ,.reset_cycles_lo_p(0)
       ,.reset_cycles_hi_p(16)
       )
   reset_gen
     (
      .clk_i(bit_clk)
      ,.async_reset_o(bit_reset)
      );
   assign core_reset = bit_reset;

   bsg_nonsynth_manycore_testbench
     #(
       .num_pods_x_p(bsg_machine_num_pods_x_gp)
       ,.num_pods_y_p(bsg_machine_num_pods_y_gp)
       ,.pod_x_cord_width_p(bsg_machine_noc_pod_x_coord_width_gp)
       ,.pod_y_cord_width_p(bsg_machine_noc_pod_y_coord_width_gp)

       ,.num_tiles_x_p(bsg_machine_num_cores_x_gp)
       ,.num_tiles_y_p(bsg_machine_num_cores_y_gp)
       ,.x_cord_width_p(bsg_machine_noc_x_coord_width_gp)
       ,.y_cord_width_p(bsg_machine_noc_y_coord_width_gp)

       ,.addr_width_p(bsg_machine_noc_epa_width_gp)
       ,.data_width_p(bsg_machine_noc_data_width_gp)
       ,.dmem_size_p(bsg_machine_core_dmem_words_gp)
       ,.icache_entries_p(bsg_machine_core_icache_entries_gp)
       ,.icache_tag_width_p(bsg_machine_core_icache_tag_width_gp)

       ,.ruche_factor_X_p(bsg_machine_noc_ruche_factor_X_gp)

       ,.vcache_data_width_p(bsg_machine_llcache_data_width_lp)
       ,.vcache_size_p(bsg_machine_llcache_words_gp)
       ,.vcache_block_size_in_words_p(bsg_machine_llcache_line_words_gp)
       ,.vcache_sets_p(bsg_machine_llcache_sets_gp)
       ,.vcache_ways_p(bsg_machine_llcache_ways_gp)
       ,.vcache_dma_data_width_p(bsg_machine_llcache_channel_width_gp)
       ,.vcache_addr_width_p(bsg_machine_llcache_addr_width_lp)

       ,.wh_flit_width_p(bsg_machine_wh_flit_width_lp)
       ,.wh_ruche_factor_p(bsg_machine_wh_ruche_factor_lp)
       ,.wh_cid_width_p(bsg_machine_wh_cid_width_lp)
       ,.wh_len_width_p(bsg_machine_wh_len_width_lp)
       ,.wh_cord_width_p(bsg_machine_wh_cord_width_lp)

       ,.bsg_manycore_mem_cfg_p(bsg_machine_dram_cfg_gp)
       ,.bsg_dram_size_p(bsg_machine_dram_words_gp)

       ,.reset_depth_p(reset_depth_lp)
       )
   testbench
     (
      .clk_i(core_clk)
      ,.reset_i(core_reset)

      ,.io_link_sif_i(host_link_sif_li)
      ,.io_link_sif_o(host_link_sif_lo)

      ,.tag_done_o(core_reset_done_lo)
      );

   bsg_nonsynth_dpi_gpio
     #(
       .width_p(2)
       ,.init_o_p('0)
       ,.use_output_p('1)
       ,.debug_p('1)
       )
   trace_control
     (.gpio_o({dpi_log_en, dpi_trace_en})
      ,.gpio_i('0)
      );

   // --------------------------------------------------------------------------
   // IO Complex
   // --------------------------------------------------------------------------
   localparam dpi_fifo_width_lp = (1 << $clog2(`bsg_manycore_packet_width(bsg_machine_noc_epa_width_gp,bsg_machine_noc_data_width_gp,bsg_machine_noc_x_coord_width_gp,bsg_machine_noc_y_coord_width_gp)));
   localparam ep_fifo_els_lp = 4;
   assign host_clk = core_clk;
   assign host_reset = core_reset;

   bsg_nonsynth_dpi_manycore
     #(
       .x_cord_width_p(bsg_machine_noc_x_coord_width_gp)
       ,.y_cord_width_p(bsg_machine_noc_y_coord_width_gp)
       ,.addr_width_p(bsg_machine_noc_epa_width_gp)
       ,.data_width_p(bsg_machine_noc_data_width_gp)
       ,.ep_fifo_els_p(ep_fifo_els_lp)
       ,.dpi_fifo_els_p(bsg_machine_dpi_fifo_els_gp)
       ,.fifo_width_p(128) // It would be better to read this from somewhere
       ,.rom_els_p(bsg_machine_rom_els_gp)
       ,.rom_width_p(bsg_machine_rom_width_gp)
       ,.rom_arr_p(bsg_machine_rom_arr_gp)
       ,.max_out_credits_p(bsg_machine_io_credits_max_gp)
       )
   mc_dpi
     (
      .clk_i(host_clk)
      ,.reset_i(host_reset)

      // manycore link
      ,.link_sif_i(host_link_sif_lo)
      ,.link_sif_o(host_link_sif_li)
      ,.my_x_i(host_x_cord_li)
      ,.my_y_i(host_y_cord_li)
      );

   bsg_dff_chain
     #(
       .width_p(1)
       ,.num_stages_p(reset_depth_lp)
       )
   reset_dff
     (
      .clk_i(core_clk)
      ,.data_i(~core_reset_done_lo)
      ,.data_o(core_reset_done_r)
      );

   bsg_nonsynth_dpi_cycle_counter
     #(.width_p(global_counter_width_lp))
   ctr
     (
      .clk_i(core_clk)
      ,.reset_i(core_reset_l[reset_depth_lp-1])
      ,.ctr_r_o(global_ctr)
      );

   bsg_print_stat_snoop
     #(
       .data_width_p(bsg_machine_noc_data_width_gp)
       ,.addr_width_p(bsg_machine_noc_epa_width_gp)
       ,.x_cord_width_p(bsg_machine_noc_x_coord_width_gp)
       ,.y_cord_width_p(bsg_machine_noc_y_coord_width_gp)
       )
   print_stat_snoop
     (
      .loader_link_sif_in_i(host_link_sif_lo)
      ,.loader_link_sif_out_i(host_link_sif_li)

      ,.print_stat_v_o(print_stat_v)
      ,.print_stat_tag_o(print_stat_tag)
      );

   // In VCS, the C/C++ testbench is controlled by the
   // simulator. Therefore, we need to "call into" the C/C++ program
   // using the cosim_main function, during the initial block.
   //
   // DPI Calls in cosim_main will cause simulator time to progress.
   //
   // This mirrors the DPI functions in aws simulation
`ifndef VERILATOR
   import "DPI-C" context task cosim_main(output int unsigned exit_code, input string args);
   initial begin
      int exit_code;
      string args;
      longint t;
      $value$plusargs("c_args=%s", args);
      replicant_tb_top.cosim_main(exit_code, args);
      if(exit_code < 0)
        $display("BSG COSIM FAIL: Test failed with exit code: %d", exit_code);
      else
        $display("BSG COSIM PASS: Test passed!");
      $finish;
   end

`endif

endmodule