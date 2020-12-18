/**
 *  cl_manycore_pkg.v
 *
 *  all the parameters for the CL.
 */
`ifndef CL_MANYCORE_PKG
`define CL_MANYCORE_PKG

package cl_manycore_pkg;

  `include "bsg_defines.v"
  import bsg_bladerunner_mem_cfg_pkg::*;
  import bsg_bladerunner_pkg::*;
   
  parameter addr_width_p = bsg_machine_noc_epa_width_gp;
  parameter data_width_p = bsg_machine_noc_data_width_gp;
  parameter num_tiles_x_p = bsg_machine_num_cores_x_gp;
  parameter num_tiles_y_p = bsg_machine_num_cores_y_gp;
  parameter x_cord_width_p = bsg_machine_noc_x_coord_width_gp;
  parameter y_cord_width_p = bsg_machine_noc_y_coord_width_gp;
  
endpackage

`endif
