module manycore_tb_top
  import bsg_noc_pkg::*; // {P=0, W, E, N, S}
  import cl_manycore_pkg::*;
  import bsg_manycore_pkg::*;
  import bsg_manycore_addr_pkg::*;
  import bsg_bladerunner_pkg::*;
  import bsg_bladerunner_mem_cfg_pkg::*;
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
      $display("[INFO][TESTBENCH] BSG_MACHINE_GLOBAL_X                 = %d", num_tiles_x_p);
      $display("[INFO][TESTBENCH] BSG_MACHINE_GLOBAL_Y                 = %d", num_tiles_y_p);
      $display("[INFO][TESTBENCH] BSG_MACHINE_VCACHE_SET               = %d", sets_p);
      $display("[INFO][TESTBENCH] BSG_MACHINE_VCACHE_WAY               = %d", ways_p);
      $display("[INFO][TESTBENCH] BSG_MACHINE_VCACHE_BLOCK_SIZE_WORDS  = %d", block_size_in_words_p);
      $display("[INFO][TESTBENCH] BSG_MACHINE_MAX_EPA_WIDTH            = %d", addr_width_p);
      $display("[INFO][TESTBENCH] BSG_MACHINE_MEM_CFG                  = %s", mem_cfg_p.name());
   end

   localparam dpi_fifo_width_lp = (1 << $clog2(`bsg_manycore_packet_width(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p)));
   localparam dpi_fifo_els_lp = dpi_fifo_els_gp;
   localparam ep_fifo_els_lp = 4;
   localparam async_fifo_els_lp = 16;
   localparam global_counter_width_lp = 64;
   localparam debug_lp = 0;
   localparam reset_depth_lp = 3; // This fixes assertions in mesh
   
   // TODO: (Future) It would be awesome if the clock frequency (or
   // frequencies) were specified at the machine level.
   parameter lc_cycle_time_p = 1000000;

   logic io_clk;
   logic io_reset;
   
   logic core_clk;
   logic core_reset;
   
   logic mem_clk;
   logic mem_reset;

   logic cache_clk;
   logic cache_reset;

   // TODO: (Future) Host coordinate should be a parameter
   logic [x_cord_width_p-1:0] host_x_cord_li = (x_cord_width_p)'(0);
   logic [y_cord_width_p-1:0] host_y_cord_li = (y_cord_width_p)'(1);

   logic [num_cache_p-1:0][x_cord_width_p-1:0] cache_x_lo;
   logic [num_cache_p-1:0][y_cord_width_p-1:0] cache_y_lo;

   `declare_bsg_manycore_link_sif_s(addr_width_p, data_width_p, x_cord_width_p, y_cord_width_p);

   bsg_manycore_link_sif_s mc_link_sif_li;
   bsg_manycore_link_sif_s mc_link_sif_lo;

   bsg_manycore_link_sif_s host_link_sif_li;
   bsg_manycore_link_sif_s host_link_sif_lo;

   bsg_manycore_link_sif_s [num_cache_p-1:0] cache_link_sif_li;
   bsg_manycore_link_sif_s [num_cache_p-1:0] cache_link_sif_lo;

   // Snoop wires for Print Stat
   logic                                       print_stat_v_lo;
   logic [data_width_p-1:0]                    print_stat_tag_lo;

   // Trace Enable wire for runtime argument to enable tracing (+trace)
   logic trace_en;
   logic log_en;
   logic dpi_trace_en;
   logic dpi_log_en;

   assign trace_en = dpi_trace_en;
   assign log_en = dpi_log_en;

   bsg_nonsynth_dpi_gpio
     #(
       .width_p(2)
       ,.init_o_p('0)
       ,.use_output_p('1)
       ,.debug_p('1)
       )
   trace_control
     (
       .gpio_o({dpi_log_en, dpi_trace_en})
       ,.gpio_i('0)
       );

   // Global Counter for Profilers, Tracing, Debugging
   logic [global_counter_width_lp-1:0] global_ctr;

// -Verilator uses a clock generator that is controlled by C/C++
// (bsg_nonsynth_dpi_clock_gen), whereas VCS uses the normal
// nonsynthesizable clock generator (bsg_nonsynth_clock_gen)
`ifdef VERILATOR
   bsg_nonsynth_dpi_clock_gen
`else
   bsg_nonsynth_clock_gen
`endif
     #(.cycle_time_p(lc_cycle_time_p)
       )
   core_clk_gen
     (.o(core_clk));

  bsg_nonsynth_reset_gen #(
    .num_clocks_p(1)
    ,.reset_cycles_lo_p(0)
    ,.reset_cycles_hi_p(16)
  ) reset_gen (
    .clk_i(core_clk)
    ,.async_reset_o(core_reset)
  );


   // bsg_manycore has reset_depth_lp flops that reset signal needs to
   // go through.  So we are trying to match that here.
   logic [reset_depth_lp:0]                         core_reset_l;

   always_ff @ (posedge core_clk) begin
      core_reset_l[0] <= core_reset;
   end

   for(genvar i = 1; i < reset_depth_lp; i++) begin
      always_ff @ (posedge core_clk) begin
         core_reset_l[i] <= core_reset_l[i-1];
      end
   end

   assign mem_clk = core_clk;
   assign mem_reset = core_reset_l[reset_depth_lp-1];

   // The caches have an additional reset flop, internally
   assign cache_clk = core_clk;
   assign cache_reset = core_reset_l[reset_depth_lp-2];

   bsg_cycle_counter
     #(.width_p(global_counter_width_lp))
   global_cc
     (
      .clk_i(core_clk)
      ,.reset_i(core_reset_l[reset_depth_lp-1])
      ,.ctr_r_o(global_ctr)
      );

   // --------------------------------------------------------------------------
   // IO Complex
   // --------------------------------------------------------------------------
   assign io_clk = core_clk;
   assign io_reset = core_reset;
   
   bsg_nonsynth_dpi_manycore
     #(.x_cord_width_p(x_cord_width_p)
       ,.y_cord_width_p(y_cord_width_p)
       ,.addr_width_p(addr_width_p)
       ,.data_width_p(data_width_p)
       ,.ep_fifo_els_p(ep_fifo_els_lp)
       ,.dpi_fifo_els_p(dpi_fifo_els_lp)
       ,.fifo_width_p(128) // It would be better to read this from somewhere
       ,.rom_els_p(rom_els_gp)
       ,.rom_width_p(rom_width_gp)
       ,.rom_arr_p(rom_arr_gp)
       ,.max_out_credits_p(max_out_credits_p) // from cl_manycore_pkg.sv
       )
   mc_dpi
     (.clk_i(io_clk)
      ,.reset_i(io_reset)

      // manycore link
      ,.link_sif_i(host_link_sif_li)
      ,.link_sif_o(host_link_sif_lo)
      ,.my_x_i(host_x_cord_li)
      ,.my_y_i(host_y_cord_li)
      );

   bsg_print_stat_snoop
     #(
       .data_width_p(data_width_p)
       ,.addr_width_p(addr_width_p)
       ,.x_cord_width_p(x_cord_width_p)
       ,.y_cord_width_p(y_cord_width_p)
       ) 
   print_stat_snoop
     (
      .loader_link_sif_in_i(mc_link_sif_lo)
      ,.loader_link_sif_out_i(mc_link_sif_li)

      ,.print_stat_v_o(print_stat_v_lo)
      ,.print_stat_tag_o(print_stat_tag_lo)
      );
   
   bsg_manycore_link_sif_async_buffer
     #(
       .addr_width_p(addr_width_p)
       ,.data_width_p(data_width_p)
       ,.x_cord_width_p(x_cord_width_p)
       ,.y_cord_width_p(y_cord_width_p)
       ,.fifo_els_p(async_fifo_els_lp)
       )
   async_buf
     (
      .L_clk_i(core_clk)
      ,.L_reset_i(core_reset)
      ,.L_link_sif_i(mc_link_sif_lo)
      ,.L_link_sif_o(mc_link_sif_li)

      ,.R_clk_i(io_clk)
      ,.R_reset_i(io_reset)
      ,.R_link_sif_i(host_link_sif_lo)
      ,.R_link_sif_o(host_link_sif_li)
      );


   // --------------------------------------------------------------------------
   // Manycore Instantiation
   // --------------------------------------------------------------------------

   bsg_manycore_link_sif_s [S:N][num_tiles_x_p-1:0] ver_link_sif_li;
   bsg_manycore_link_sif_s [S:N][num_tiles_x_p-1:0] ver_link_sif_lo;
   bsg_manycore_link_sif_s [num_tiles_x_p-1:0] io_link_sif_li;
   bsg_manycore_link_sif_s [num_tiles_x_p-1:0] io_link_sif_lo;
  

   // Instantiate a crossbar, or a mesh network depending on the
   // machine parameterization. The two sides of the if-statment have
   // idenitical labels to make DPI hierarchy processing easier
   if (bsg_machine_crossbar_network_gp == 1) begin: network

     bsg_manycore_top_crossbar #(
       .dmem_size_p(dmem_size_p)
       ,.icache_entries_p(icache_entries_p)
       ,.icache_tag_width_p(icache_tag_width_p)
       ,.num_tiles_x_p(num_tiles_x_p)
       ,.num_tiles_y_p(num_tiles_y_p+1)
       ,.reset_depth_p(reset_depth_lp)
       ,.data_width_p(data_width_p)
       ,.addr_width_p(addr_width_p)
       ,.vcache_size_p(vcache_size_p)
       ,.vcache_block_size_in_words_p(block_size_in_words_p)
       ,.vcache_sets_p(sets_p)
     ) manycore (
       .clk_i(core_clk)
       ,.reset_i(core_reset)

       ,.ver_link_sif_i(ver_link_sif_li)
       ,.ver_link_sif_o(ver_link_sif_lo)

       ,.io_link_sif_i(io_link_sif_li)
       ,.io_link_sif_o(io_link_sif_lo)
     );

  end else begin: network

   bsg_manycore_link_sif_s [E:W][num_tiles_y_p:0] hor_link_sif_li;
   bsg_manycore_link_sif_s [E:W][num_tiles_y_p:0] hor_link_sif_lo;

   bsg_manycore 
     #(
     .dmem_size_p(dmem_size_p)
     ,.icache_entries_p(icache_entries_p)
     ,.icache_tag_width_p(icache_tag_width_p)
     ,.num_tiles_x_p(num_tiles_x_p)
     ,.num_tiles_y_p(num_tiles_y_p+1)
     ,.reset_depth_p(reset_depth_lp)
     ,.debug_p(debug_lp)
     ,.addr_width_p(addr_width_p)
     ,.data_width_p(data_width_p)
     ,.vcache_size_p(vcache_size_p)
     ,.vcache_block_size_in_words_p(block_size_in_words_p)
     ,.vcache_sets_p(sets_p)
     ,.branch_trace_en_p(branch_trace_en_p)
   ) manycore (
     .clk_i(core_clk)
     ,.reset_i(core_reset)

     ,.hor_link_sif_i(hor_link_sif_li)
     ,.hor_link_sif_o(hor_link_sif_lo)

     ,.ver_link_sif_i(ver_link_sif_li)
     ,.ver_link_sif_o(ver_link_sif_lo)
    
     ,.io_link_sif_i(io_link_sif_li)
     ,.io_link_sif_o(io_link_sif_lo)
   );

    // Horizontal Tie-Offs
    //
    for (genvar i = 0; i < num_tiles_y_p+1; i++) begin
      bsg_manycore_link_sif_tieoff #(
        .addr_width_p(addr_width_p)
        ,.data_width_p(data_width_p)
        ,.x_cord_width_p(x_cord_width_p)
        ,.y_cord_width_p(y_cord_width_p)
      ) tieoff_w (
        .clk_i(core_clk)
        ,.reset_i(core_reset)
        ,.link_sif_i(hor_link_sif_lo[W][i])
        ,.link_sif_o(hor_link_sif_li[W][i])
      );
    end

     for (genvar i = 0; i < num_tiles_y_p+1; i++) begin
       bsg_manycore_link_sif_tieoff #(
         .addr_width_p(addr_width_p)
         ,.data_width_p(data_width_p)
         ,.x_cord_width_p(x_cord_width_p)
         ,.y_cord_width_p(y_cord_width_p)
       ) tieoff_e (
         .clk_i(core_clk)
         ,.reset_i(core_reset)
         ,.link_sif_i(hor_link_sif_lo[E][i])
         ,.link_sif_o(hor_link_sif_li[E][i])
       );
     end

  end

  // connecting link_sif to outside
  //
  //  north[0]  : victim cache 0
  //  north[1]  : victim cache 1
  //  ...
  //  x[0].y[1] : host interface
  //  x[1].y[1] : unused
  //  ...

  //  south[0] : victim cache X
  //  south[1] : victim cache X+1
  //  ...
  //
  for (genvar i = 0; i < num_tiles_x_p; i++) begin
    assign cache_link_sif_lo[i] = ver_link_sif_lo[N][i];
    assign ver_link_sif_li[N][i] = cache_link_sif_li[i];
    assign cache_x_lo[i] = (x_cord_width_p)'(i);
    assign cache_y_lo[i] = (y_cord_width_p)'(0);
  end

  for (genvar i = 0; i < num_tiles_x_p; i++) begin
    assign cache_link_sif_lo[num_tiles_x_p+i] = ver_link_sif_lo[S][i];
    assign ver_link_sif_li[S][i] = cache_link_sif_li[num_tiles_x_p+i];
    assign cache_x_lo[num_tiles_x_p+i] = (x_cord_width_p)'(i);
    assign cache_y_lo[num_tiles_x_p+i] = (y_cord_width_p)'(num_tiles_y_p+2);
  end

  // 0,1 for host io
  //
  assign mc_link_sif_lo = io_link_sif_lo[0];
  assign io_link_sif_li[0] = mc_link_sif_li;

  // Tie off remaining IO links
  for (genvar i = 1; i < num_tiles_x_p; i++) begin
    bsg_manycore_link_sif_tieoff #(
      .addr_width_p(addr_width_p)
      ,.data_width_p(data_width_p)
      ,.x_cord_width_p(x_cord_width_p)
      ,.y_cord_width_p(y_cord_width_p)
    ) tieoff_io (
      .clk_i(core_clk)
      ,.reset_i(core_reset)
      ,.link_sif_i(io_link_sif_lo[i])
      ,.link_sif_o(io_link_sif_li[i])
    );
  end

   // Manycore Profiling, Trace, and Debug Infrastructure

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
      ,.trace_en_i($root.manycore_tb_top.log_en));

   bind vanilla_core vanilla_core_profiler
     #(
       .x_cord_width_p(x_cord_width_p)
       ,.y_cord_width_p(y_cord_width_p)
       ,.origin_x_cord_p(0)
       ,.origin_y_cord_p(2)
       ,.icache_tag_width_p(icache_tag_width_p)
       ,.icache_entries_p(icache_entries_p)
       ,.data_width_p(data_width_p)
       )
   vcore_prof
     (
      .*
      ,.global_ctr_i($root.manycore_tb_top.global_ctr)
      ,.print_stat_v_i($root.manycore_tb_top.print_stat_v_lo)
      ,.print_stat_tag_i($root.manycore_tb_top.print_stat_tag_lo)
      ,.trace_en_i($root.manycore_tb_top.trace_en)
      );



   // --------------------------------------------------------------------------
   // Configurable Memory System
   // --------------------------------------------------------------------------
   localparam byte_offset_width_lp=`BSG_SAFE_CLOG2(data_width_p>>3);
   localparam cache_addr_width_lp=(addr_width_p-1+byte_offset_width_lp);

`ifdef USING_DRAMSIM3

  localparam hbm_channel_addr_width_p
    = `DRAMSIM3_MEM_PKG::channel_addr_width_p;
  localparam hbm_data_width_p
    = `DRAMSIM3_MEM_PKG::data_width_p;
  localparam hbm_num_channels_p
    = `DRAMSIM3_MEM_PKG::num_channels_p;

`else
  localparam hbm_channel_addr_width_p = 29;
  localparam hbm_data_width_p = 512;
  localparam hbm_num_channels_p = 8;
`endif

   if (mem_cfg_p == e_vcache_non_blocking_test_dramsim3_hbm2_4gb_x128
       || mem_cfg_p == e_vcache_blocking_test_dramsim3_hbm2_4gb_x128) begin: lv1_dma

      // for now blocking and non-blocking shares the same wire, since interface is
      // the same. But it might change in the future.

      import bsg_cache_pkg::*;
      localparam dma_pkt_width_lp = `bsg_cache_dma_pkt_width(cache_addr_width_lp);

      logic [num_cache_p-1:0][dma_pkt_width_lp-1:0] dma_pkt;
      logic [num_cache_p-1:0]                       dma_pkt_v_lo;
      logic [num_cache_p-1:0]                       dma_pkt_yumi_li;

      logic [num_cache_p-1:0][dma_data_width_p-1:0] dma_data_li;
      logic [num_cache_p-1:0]                       dma_data_v_li;
      logic [num_cache_p-1:0]                       dma_data_ready_lo;

      logic [num_cache_p-1:0][dma_data_width_p-1:0] dma_data_lo;
      logic [num_cache_p-1:0]                       dma_data_v_lo;
      logic [num_cache_p-1:0]                       dma_data_yumi_li;

   end


   // LEVEL 1
   if (mem_cfg_p == e_infinite_mem) begin
      // each column has a nonsynth infinite memory
      localparam infmem_els_lp = 1<<(addr_width_p-$clog2(num_cache_p));

      for (genvar i = 0; i < num_cache_p; i++) begin
         bsg_nonsynth_mem_infinite 
           #(
             .data_width_p(data_width_p)
             ,.addr_width_p(addr_width_p)
             ,.mem_els_p(infmem_els_lp)
             ,.x_cord_width_p(x_cord_width_p)
             ,.y_cord_width_p(y_cord_width_p)
             ,.id_p(i)
             ) 
         mem_infty 
            (
             .clk_i(mem_clk)
             ,.reset_i(mem_reset)
             // memory systems link from bsg_manycore_wrapper
             ,.link_sif_i(cache_link_sif_lo[i])
             ,.link_sif_o(cache_link_sif_li[i])
             // coordinates for memory system are determined by bsg_manycore_wrapper
             ,.my_x_i(cache_x_lo[i])
             ,.my_y_i(cache_y_lo[i])
             );
      end

      bind bsg_nonsynth_mem_infinite infinite_mem_profiler 
          #(
            .data_width_p(data_width_p)
            ,.addr_width_p(addr_width_p)
            ,.x_cord_width_p(x_cord_width_p)
            ,.y_cord_width_p(y_cord_width_p)
            ,.logfile_p("infinite_mem_stats.csv")
            ) 
      infinite_mem_prof 
        (
         .*
         ,.global_ctr_i($root.manycore_tb_top.global_ctr)
         ,.print_stat_v_i($root.manycore_tb_top.print_stat_v_lo)
         ,.print_stat_tag_i($root.manycore_tb_top.print_stat_tag_lo)
         );

   end else if (mem_cfg_p == e_vcache_blocking_test_dramsim3_hbm2_4gb_x128) begin: lv1_vcache

      for (genvar i = 0; i < num_cache_p; i++) begin: vcache

         bsg_manycore_vcache_blocking 
           #(
             .data_width_p(data_width_p)
             ,.addr_width_p(addr_width_p)
             ,.block_size_in_words_p(block_size_in_words_p)
             ,.sets_p(sets_p)
             ,.ways_p(ways_p)
             ,.dma_data_width_p(dma_data_width_p)
             ,.x_cord_width_p(x_cord_width_p)
             ,.y_cord_width_p(y_cord_width_p)
             ) 
         vcache 
            (
             .clk_i(cache_clk)
             ,.reset_i(cache_reset)
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

      bind bsg_cache vcache_profiler 
        #(
          .data_width_p(data_width_p)
          ,.addr_width_p(addr_width_p)
          ,.header_print_p("vcache[0]")
          ) 
      vcache_prof 
        (
         .*
         ,.global_ctr_i($root.manycore_tb_top.global_ctr)
         ,.print_stat_v_i($root.manycore_tb_top.print_stat_v_lo)
         ,.print_stat_tag_i($root.manycore_tb_top.print_stat_tag_lo)
         ,.trace_en_i($root.manycore_tb_top.trace_en)
         );

   end // block: lv1_vcache
   else if (mem_cfg_p == e_vcache_non_blocking_test_dramsim3_hbm2_4gb_x128) begin: lv1_vcache_nb

      for (genvar i = 0; i < num_cache_p; i++) begin: vcache
         bsg_manycore_vcache_non_blocking 
           #(
             .data_width_p(data_width_p)
             ,.addr_width_p(addr_width_p)
             ,.block_size_in_words_p(block_size_in_words_p)
             ,.sets_p(sets_p)
             ,.ways_p(ways_p)

             ,.miss_fifo_els_p(miss_fifo_els_p)
             ,.x_cord_width_p(x_cord_width_p)
             ,.y_cord_width_p(y_cord_width_p)
             ) 
         vcache_nb 
            (
             .clk_i(cache_clk)
             ,.reset_i(cache_reset)

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

      bind bsg_cache_non_blocking vcache_non_blocking_profiler 
        #(
          .data_width_p(data_width_p)
          ,.addr_width_p(addr_width_p)
          ,.sets_p(sets_p)
          ,.ways_p(ways_p)
          ,.id_width_p(id_width_p)
          ,.block_size_in_words_p(block_size_in_words_p)
          ) 
      vcache_prof
        (
         .clk_i(clk_i)
         ,.reset_i(reset_i)

         ,.tl_data_mem_pkt_i(tl_data_mem_pkt_lo)
         ,.tl_data_mem_pkt_v_i(tl_data_mem_pkt_v_lo)
         ,.tl_data_mem_pkt_ready_i(tl_data_mem_pkt_ready_li)

         ,.mhu_idle_i(mhu_idle)

         ,.mhu_data_mem_pkt_i(mhu_data_mem_pkt_lo)
         ,.mhu_data_mem_pkt_v_i(mhu_data_mem_pkt_v_lo)
         ,.mhu_data_mem_pkt_yumi_i(mhu_data_mem_pkt_yumi_li)

         ,.miss_fifo_data_i(miss_fifo_data_li)
         ,.miss_fifo_v_i(miss_fifo_v_li)
         ,.miss_fifo_ready_i(miss_fifo_ready_lo)

         ,.dma_pkt_i(dma_pkt_o)
         ,.dma_pkt_v_i(dma_pkt_v_o)
         ,.dma_pkt_yumi_i(dma_pkt_yumi_i)

         ,.global_ctr_i($root.manycore_tb_top.global_ctr)
         ,.print_stat_v_i($root.manycore_tb_top.print_stat_v_lo)
         ,.print_stat_tag_i($root.manycore_tb_top.print_stat_tag_lo)
         );
   end 

   if (mem_cfg_p == e_vcache_non_blocking_test_dramsim3_hbm2_4gb_x128 ||
            mem_cfg_p == e_vcache_blocking_test_dramsim3_hbm2_4gb_x128) begin: lv2_simulated_hbm

      // checks that this configuration is supported
      // we do not support having fewer caches than channels
      localparam int num_cache_per_hbm_channel_p = $floor(num_cache_p/dram_channels_used_p);
      if (num_cache_per_hbm_channel_p <= 0) begin
         $fatal(1, "dram channels (%d) must be less than or equal to l2 caches (%d)",
                dram_channels_used_p, num_cache_p);
      end
      // caches:channels must be an integral ratio
      localparam real _num_tiles_x_real_p = num_cache_p;
      if (num_cache_per_hbm_channel_p != $ceil(_num_tiles_x_real_p/dram_channels_used_p)) begin
         $fatal(1, "l2 caches (%d) must be a multiple of dram channels (%d)",
                num_cache_p, dram_channels_used_p);
      end

      localparam lg_num_cache_per_hbm_channel_p = `BSG_SAFE_CLOG2(num_cache_per_hbm_channel_p);
      localparam hbm_cache_bank_addr_width_p = hbm_channel_addr_width_p - lg_num_cache_per_hbm_channel_p;
      // DDR is unused

      logic [hbm_num_channels_p-1:0][hbm_channel_addr_width_p-1:0] hbm_ch_addr_lo;
      logic [hbm_num_channels_p-1:0]                               hbm_req_yumi_li;
      logic [hbm_num_channels_p-1:0]                               hbm_req_v_lo;
      logic [hbm_num_channels_p-1:0]                               hbm_write_not_read_lo;

      logic [hbm_num_channels_p-1:0][hbm_data_width_p-1:0]         hbm_data_lo;
      logic [hbm_num_channels_p-1:0]                               hbm_data_v_lo;
      logic [hbm_num_channels_p-1:0]                               hbm_data_yumi_li;

      logic [hbm_num_channels_p-1:0][hbm_data_width_p-1:0]         hbm_data_li;
      logic [hbm_num_channels_p-1:0][hbm_channel_addr_width_p-1:0] hbm_ch_addr_li;
      logic [hbm_num_channels_p-1:0]                               hbm_data_v_li;

      for (genvar ch_i = 0; ch_i < dram_channels_used_p; ch_i++) begin
         localparam cache_range_lo_p = ch_i * num_cache_per_hbm_channel_p;
         localparam cache_range_hi_p = (ch_i+1) * num_cache_per_hbm_channel_p - 1;

         bsg_cache_to_test_dram
           #(.num_cache_p(num_cache_per_hbm_channel_p)
             ,.data_width_p(data_width_p)
             ,.dma_data_width_p(dma_data_width_p)
             ,.addr_width_p(cache_addr_width_lp)
             ,.block_size_in_words_p(block_size_in_words_p)
             ,.cache_bank_addr_width_p(hbm_cache_bank_addr_width_p)
             ,.dram_channel_addr_width_p(hbm_channel_addr_width_p)
             ,.dram_data_width_p(hbm_data_width_p))
         cache_to_test_dram
           (.core_clk_i(core_clk)
            ,.core_reset_i(core_reset)

            ,.dma_pkt_i(lv1_dma.dma_pkt[cache_range_hi_p:cache_range_lo_p])
            ,.dma_pkt_v_i(lv1_dma.dma_pkt_v_lo[cache_range_hi_p:cache_range_lo_p])
            ,.dma_pkt_yumi_o(lv1_dma.dma_pkt_yumi_li[cache_range_hi_p:cache_range_lo_p])

            ,.dma_data_o(lv1_dma.dma_data_li[cache_range_hi_p:cache_range_lo_p])
            ,.dma_data_v_o(lv1_dma.dma_data_v_li[cache_range_hi_p:cache_range_lo_p])
            ,.dma_data_ready_i(lv1_dma.dma_data_ready_lo[cache_range_hi_p:cache_range_lo_p])

            ,.dma_data_i(lv1_dma.dma_data_lo[cache_range_hi_p:cache_range_lo_p])
            ,.dma_data_v_i(lv1_dma.dma_data_v_lo[cache_range_hi_p:cache_range_lo_p])
            ,.dma_data_yumi_o(lv1_dma.dma_data_yumi_li[cache_range_hi_p:cache_range_lo_p])

            ,.dram_clk_i(mem_clk)
            ,.dram_reset_i(mem_reset)

            ,.dram_ch_addr_o(hbm_ch_addr_lo[ch_i])
            ,.dram_req_yumi_i(hbm_req_yumi_li[ch_i])
            ,.dram_req_v_o(hbm_req_v_lo[ch_i])
            ,.dram_write_not_read_o(hbm_write_not_read_lo[ch_i])

            ,.dram_data_o(hbm_data_lo[ch_i])
            ,.dram_data_v_o(hbm_data_v_lo[ch_i])
            ,.dram_data_yumi_i(hbm_data_yumi_li[ch_i])

            ,.dram_data_i(hbm_data_li[ch_i])
            ,.dram_data_v_i(hbm_data_v_li[ch_i])
            ,.dram_ch_addr_i(hbm_ch_addr_li[ch_i])
            );
      end

      // tie-off handshake for the the unused hbm channels
      for (genvar ch_i = dram_channels_used_p; ch_i < hbm_num_channels_p; ch_i++) begin
         assign hbm_req_v_lo[ch_i]  = 1'b0;
         assign hbm_data_v_lo[ch_i] = 1'b0;
      end
      
      // assign hbm clk and reset to core for now...
   end // block: lv2_simulated_hbm

   if (mem_cfg_p == e_vcache_blocking_test_dramsim3_hbm2_4gb_x128 ||
            mem_cfg_p == e_vcache_non_blocking_test_dramsim3_hbm2_4gb_x128) begin: lv3_dramsim3

`ifdef USING_DRAMSIM3

      `declare_dramsim3_ch_addr_s_with_pkg(dram_ch_addr_dramsim3_s, `DRAMSIM3_MEM_PKG);
      dram_ch_addr_dramsim3_s [hbm_num_channels_p-1:0] dramsim3_ch_addr_lo;
      dram_ch_addr_dramsim3_s [hbm_num_channels_p-1:0] dramsim3_ch_addr_li;

      typedef struct packed {
         // the cache id is in the MSBs from cach_to_test_dram
         // we map each cache to its own banks (if there less caches than banks)
         logic [`dramsim3_bg_width_pkg(`DRAMSIM3_MEM_PKG)-1:0] bg;
         logic [`dramsim3_ba_width_pkg(`DRAMSIM3_MEM_PKG)-1:0] ba;
         logic [`dramsim3_ro_width_pkg(`DRAMSIM3_MEM_PKG)-1:0] ro;
         logic [`dramsim3_co_width_pkg(`DRAMSIM3_MEM_PKG)-1:0] co;
         logic [`dramsim3_byte_offset_width_pkg(`DRAMSIM3_MEM_PKG)-1:0] byte_offset;
      } dram_ch_addr_cache_to_test_dram_s;

      dram_ch_addr_cache_to_test_dram_s [hbm_num_channels_p-1:0] hbm_ch_addr_lo_cast;
      dram_ch_addr_cache_to_test_dram_s [hbm_num_channels_p-1:0] hbm_ch_addr_li_cast;

      assign hbm_ch_addr_lo_cast = lv2_simulated_hbm.hbm_ch_addr_lo;
      assign lv2_simulated_hbm.hbm_ch_addr_li = hbm_ch_addr_li_cast;

      for (genvar i = 0; i < hbm_num_channels_p; i++) begin
         // mapping cache_to_test_dram => dramsim3
         assign dramsim3_ch_addr_li[i].bg          = hbm_ch_addr_lo_cast[i].bg;
         assign dramsim3_ch_addr_li[i].ba          = hbm_ch_addr_lo_cast[i].ba;
         assign dramsim3_ch_addr_li[i].ro          = hbm_ch_addr_lo_cast[i].ro;
         assign dramsim3_ch_addr_li[i].co          = hbm_ch_addr_lo_cast[i].co;
         assign dramsim3_ch_addr_li[i].byte_offset = hbm_ch_addr_lo_cast[i].byte_offset;
         // mapping dramsim3 => cache_to_test_dram
         assign hbm_ch_addr_li_cast[i].bg          = dramsim3_ch_addr_lo[i].bg;
         assign hbm_ch_addr_li_cast[i].ba          = dramsim3_ch_addr_lo[i].ba;
         assign hbm_ch_addr_li_cast[i].ro          = dramsim3_ch_addr_lo[i].ro;
         assign hbm_ch_addr_li_cast[i].co          = dramsim3_ch_addr_lo[i].co;
         assign hbm_ch_addr_li_cast[i].byte_offset = dramsim3_ch_addr_lo[i].byte_offset;
      end

      bsg_nonsynth_dramsim3
        #(.channel_addr_width_p(`DRAMSIM3_MEM_PKG::channel_addr_width_p)
          ,.data_width_p(`DRAMSIM3_MEM_PKG::data_width_p)
          ,.num_channels_p(`DRAMSIM3_MEM_PKG::num_channels_p)
          ,.num_columns_p(`DRAMSIM3_MEM_PKG::num_columns_p)
          ,.address_mapping_p(`DRAMSIM3_MEM_PKG::address_mapping_p)
          ,.size_in_bits_p(`DRAMSIM3_MEM_PKG::size_in_bits_p)
          ,.config_p(`DRAMSIM3_MEM_PKG::config_p)
          //,.debug_p(1)
          ,.init_mem_p(1))
      dram
        (.clk_i(mem_clk)
         ,.reset_i(mem_reset)

         ,.v_i(lv2_simulated_hbm.hbm_req_v_lo)
         ,.write_not_read_i(lv2_simulated_hbm.hbm_write_not_read_lo)
         ,.ch_addr_i(dramsim3_ch_addr_li)
         ,.yumi_o(lv2_simulated_hbm.hbm_req_yumi_li)

         ,.data_v_i(lv2_simulated_hbm.hbm_data_v_lo)
         ,.data_i(lv2_simulated_hbm.hbm_data_lo)
         ,.data_yumi_o(lv2_simulated_hbm.hbm_data_yumi_li)

         ,.data_o(lv2_simulated_hbm.hbm_data_li)
         ,.data_v_o(lv2_simulated_hbm.hbm_data_v_li)
         ,.read_done_ch_addr_o(dramsim3_ch_addr_lo));
`endif

   end

   // Instantiate a counter to track execution time on the
   // manycore. 
   bsg_nonsynth_dpi_cycle_counter
     #(.width_p(64), // 64-bit counter
       .debug_p(0))
   ctr
     (.clk_i(core_clk)
      ,.reset_i(core_reset));  

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
      manycore_tb_top.cosim_main(exit_code, args);
      if(exit_code < 0) 
          $display("BSG COSIM FAIL: Test failed with exit code: %d", exit_code);
      else 
          $display("BSG COSIM PASS: Test passed!");
      $finish;
   end
`endif

endmodule
