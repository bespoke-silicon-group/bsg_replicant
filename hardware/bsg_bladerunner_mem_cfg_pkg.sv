/**
 *  bsg_bladerunner_mem_cfg_pkg.sv
 */

package bsg_bladerunner_mem_cfg_pkg;

`include "bsg_defines.sv"

   localparam max_cfgs = 128;
   localparam lg_max_cfgs = `BSG_SAFE_CLOG2(max_cfgs); 

   // Bladerunner Memory Configuration enum
   // 
   // The enum naming convention describes which memory system is being used.
   // It roughly divides into three levels of hierarchy.
   //
   // e_{cache/block_mem}_{interface}_{backend_memory}
   //    
   //
   // LEVEL 1) What is attached to manycore link on the south side. This could be the
   //          last level of hierarchy, if it's block mem, or infinite memory, for
   //          example.
   //          - e_vcache_blocking_*
   //          - e_vcache_non_blocking_*
   //          - e_infinite_memory (No further levels)
   //
   // LEVEL 2) What interface does cache DMA interface converts to.
   //          - dma_ (no interface conversion)
   //          - axi4_ (convert to axi4)
   //          - dmc_ (convert to bsg_dmc interface)
   //          - aib_ (convert to AIB interface)
   //          - test_ (Converts to abstract DRAM Simulation Interface using bsg_cache_to_test_dram)
   //
   // LEVEL 3) What is being used as the last main memory.
   //          - nonsynth_mem 
   //          - lpddr4 (ex. micron sim model)
   //          - f1_ddr
   //          - dramsim3_hbm2_4gb_x128 (DRAM Sim 3 HBM 2 Model)

   typedef enum bit [lg_max_cfgs-1:0] {

   // LEVEL 1) zero-latency, infinite capacity block mem.
   //          (uses associative array)
   e_infinite_mem
    
   // LEVEL 1) bsg_manycore_vcache (blocking)
   // LEVEL 2) bsg_cache_to_axi
   // LEVEl 3) AWS F1 Interface

   // e_vcache_blocking_axi4_f1_dram directs simulation to use the
   // slower, but more accurate, DDR Model. The default is
   // e_vcache_blocking_axi4_f1_model uses an (infinite) AXI memory
   // model with low (1-2 cycle) latency in simulation. 
   //                                      
   // Either value produces the correct DDR4 Module in F1
   // implemenation
   , e_vcache_blocking_axi4_f1_dram
   , e_vcache_blocking_axi4_f1_model

   // LEVEL 1) bsg_manycore_vcache (blocking)
   // LEVEL 2) bsg_cache_to_axi
   // LEVEl 3) AWS F1 Interface (See comment above)
   , e_vcache_non_blocking_axi4_f1_dram
   , e_vcache_non_blocking_axi4_f1_model

   // LEVEL 1) bsg_manycore_vcache (non-blocking)
   // LEVEL 2) bsg_cache_to_test_dram
   // LEVEL 3) bsg_nonsynth_dramsim3
   , e_vcache_non_blocking_test_dramsim3_hbm2_4gb_x128

   // LEVEL 1) bsg_manycore_vcache (blocking)
   // LEVEL 2) bsg_cache_to_test_dram
   // LEVEL 3) bsg_nonsynth_dramsim3
   , e_vcache_blocking_test_dramsim3_hbm2_4gb_x128
  } bsg_bladerunner_mem_cfg_e;

endpackage
