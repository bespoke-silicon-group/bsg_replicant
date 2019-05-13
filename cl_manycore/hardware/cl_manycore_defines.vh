`ifndef CL_DEFINES
`define CL_DEFINES

//Put module name of the CL design here.  This is used to instantiate in top.sv
`define CL_NAME cl_manycore

//Highly recommeneded.  For lib FIFO block, uses less async reset (take advantage of
// FPGA flop init capability).  This will help with routing resources.
`define FPGA_LESS_RST

`define _bsg_data_end_addr 32

// Uncomment to disable Virtual JTAG
//`define DISABLE_VJTAG_DEBUG

// Define macros for the manycore hardware
`define axi4_to_sh_ddr_num 2

// Enable local vcu128 board
`define LOCAL_FPGA

`endif

