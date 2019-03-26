/**
 *  cl_bladerunner_rom_pkg.vh
 *
 *  parameters for bladerunner rom.
 */

`ifndef BSG_BLADERUNNER_ROM_PKG
`define BSG_BLADERUNNER_ROM_PKG

`include "cl_manycore_pkg.v"

`define BSG_BLADERUNNER_ROM_FILE "bsg_bladerunner.rom"

package bsg_bladerunner_rom_pkg;

  parameter rom_width_p = 32;
  parameter rom_els_p = 12;
    
endpackage

`endif
