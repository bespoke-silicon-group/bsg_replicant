/**
 *  cl_manycore_pkg.v
 *
 *  all the parameters for the CL.
 */
`ifndef CL_MANYCORE_PKG
`define CL_MANYCORE_PKG

package cl_manycore_pkg;

  `include "bsg_defines.v"
  `include "f1_parameters.vh"
  import bsg_bladerunner_mem_cfg_pkg::*;
  
  parameter bsg_bladerunner_mem_cfg_e mem_cfg_p = `CL_MANYCORE_MEM_CFG;
  parameter addr_width_p = `CL_MANYCORE_MAX_EPA_WIDTH;
  parameter data_width_p = `CL_MANYCORE_DATA_WIDTH;
  parameter num_tiles_x_p = `CL_MANYCORE_DIM_X;
  parameter num_tiles_y_p = `CL_MANYCORE_DIM_Y;
  parameter x_cord_width_p = `BSG_SAFE_CLOG2(num_tiles_x_p);
  parameter y_cord_width_p = `BSG_SAFE_CLOG2(num_tiles_y_p+2);
  parameter dmem_size_p = 1024;
  parameter icache_entries_p = 1024;
  parameter icache_tag_width_p = 12;
  parameter dram_ch_addr_width_p = 27;
  parameter epa_byte_addr_width_p = 18;
  parameter branch_trace_en_p = `CL_MANYCORE_BRANCH_TRACE_EN;

  parameter num_cache_p = `CL_MANYCORE_DIM_X;
  parameter sets_p = `CL_MANYCORE_VCACHE_SETS;
  parameter ways_p = `CL_MANYCORE_VCACHE_WAYS;
  parameter block_size_in_words_p = `CL_MANYCORE_VCACHE_BLOCK_SIZE_WORDS;
  parameter vcache_size_p = sets_p * ways_p * block_size_in_words_p;
  parameter miss_fifo_els_p = `CL_MANYCORE_VCACHE_MISS_FIFO_ELS;

  parameter axi_id_width_p = 6;
  parameter axi_addr_width_p = 64;
  parameter axi_data_width_p = 512;
  parameter axi_strb_width_p = (axi_data_width_p>>3);
  parameter axi_burst_len_p = 1;

  // the max number of outstanding requests from the host endpoint to the manycore
  parameter max_out_credits_p = 16;

endpackage

`endif
