/**
 *  cl_manycore_pkg.v
 *
 *  all the parameters for the CL.
 */
`ifndef CL_MANYCORE_PKG
`define CL_MANYCORE_PKG

package cl_manycore_pkg;

  `include "bsg_defines.v"

  parameter addr_width_p = 28;
  parameter data_width_p = 32;
  parameter num_tiles_x_p = 4;
  parameter num_tiles_y_p = 4;
  parameter x_cord_width_p = `BSG_SAFE_CLOG2(num_tiles_x_p);
  parameter y_cord_width_p = `BSG_SAFE_CLOG2(num_tiles_y_p+2);
  parameter load_id_width_p = 11;
  parameter dmem_size_p = 1024;
  parameter icache_entries_p = 1024;
  parameter icache_tag_width_p = 12;
  parameter dram_ch_addr_width_p = 27;
  parameter epa_byte_addr_width_p = 18;

  parameter num_cache_p = 4;
  parameter sets_p = 64;
  parameter ways_p = 2;
  parameter block_size_in_words_p = 16;

  parameter axi_id_width_p = 6;
  parameter axi_addr_width_p = 64;
  parameter axi_data_width_p = 512;
  parameter axi_strb_width_p = (axi_data_width_p>>3);
  parameter axi_burst_len_p = 1;

  // the max number of outstanding requests from the host endpoint to the manycore
  parameter max_out_credits_p = 16;

endpackage

`endif
