`ifndef CL_MANYCORE_DEFINES
`define CL_MANYCORE_DEFINES

//Put module name of the CL design here.  This is used to instantiate in top.sv
`define CL_NAME cl_manycore

//Highly recommeneded.  For lib FIFO block, uses less async reset (take advantage of
// FPGA flop init capability).  This will help with routing resources.
`define FPGA_LESS_RST

// Uncomment to disable Virtual JTAG
//`define DISABLE_VJTAG_DEBUG

`endif
