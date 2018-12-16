// Amazon FPGA Hardware Development Kit
//
// Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Amazon Software License (the "License"). You may not use
// this file except in compliance with the License. A copy of the License is
// located at
//
//    http://aws.amazon.com/asl/
//
// or in the "license" file accompanying this file. This file is distributed on
// an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, express or
// implied. See the License for the specific language governing permissions and
// limitations under the License.

// Copy from cl_dram_dma_pkg.sv
// Defines standard axi-4 interface (without user signal), and config bus --XL

`ifndef CL_FSB_BUS_PKG
`define CL_FSB_BUS_PKG

interface axi_bus_t #(
    parameter NUM_SLOTS=1
    ,parameter ID_WIDTH=6
    ,parameter ADDR_WIDTH=64
    ,parameter DATA_WIDTH=512
  );

  logic [  NUM_SLOTS*ID_WIDTH-1:0] awid   ; // [5:0]
  logic [NUM_SLOTS*ADDR_WIDTH-1:0] awaddr ;
  logic [         NUM_SLOTS*8-1:0] awlen  ;
  logic [         NUM_SLOTS*3-1:0] awsize ; // [2:0]
  logic [           NUM_SLOTS-1:0] awvalid;
  logic [           NUM_SLOTS-1:0] awready;

  logic [    NUM_SLOTS*ID_WIDTH-1:0] wid   ; // this is not used in cl_ports.vh
  logic [  NUM_SLOTS*DATA_WIDTH-1:0] wdata ;
  logic [NUM_SLOTS*DATA_WIDTH/8-1:0] wstrb ;
  logic [             NUM_SLOTS-1:0] wlast ;
  logic [             NUM_SLOTS-1:0] wvalid;
  logic [             NUM_SLOTS-1:0] wready;

  logic [NUM_SLOTS*ID_WIDTH-1:0] bid   ; // [5:0]
  logic [       NUM_SLOTS*2-1:0] bresp ;
  logic [         NUM_SLOTS-1:0] bvalid;
  logic [         NUM_SLOTS-1:0] bready;

  logic [  NUM_SLOTS*ID_WIDTH-1:0] arid   ; // [5:0]
  logic [NUM_SLOTS*ADDR_WIDTH-1:0] araddr ;
  logic [         NUM_SLOTS*8-1:0] arlen  ;
  logic [         NUM_SLOTS*3-1:0] arsize ;
  logic [           NUM_SLOTS-1:0] arvalid;
  logic [           NUM_SLOTS-1:0] arready;

  logic [  NUM_SLOTS*ID_WIDTH-1:0] rid   ; // [5:0]
  logic [NUM_SLOTS*DATA_WIDTH-1:0] rdata ;
  logic [         NUM_SLOTS*2-1:0] rresp ;
  logic [           NUM_SLOTS-1:0] rlast ;
  logic [           NUM_SLOTS-1:0] rvalid;
  logic [           NUM_SLOTS-1:0] rready;

  modport master (
    input awid, awaddr, awlen, awsize, awvalid, output awready,
    input wid, wdata, wstrb, wlast, wvalid, output wready,
    output bid, bresp, bvalid, input bready,
    input arid, araddr, arlen, arsize, arvalid, output arready,
    output rid, rdata, rresp, rlast, rvalid, input rready
  );

  modport slave (
    output awid, awaddr, awlen, awsize, awvalid, input awready,
    output wid, wdata, wstrb, wlast, wvalid, input wready,
    input bid, bresp, bvalid, output bready,
    output arid, araddr, arlen, arsize, arvalid, input arready,
    input rid, rdata, rresp, rlast, rvalid, output rready
  );
endinterface


interface axil_bus_t #(parameter NUM_SLOTS=1);

  logic [NUM_SLOTS*32-1:0] awaddr;
  // lo gic[2:0] wrprot;
  logic [NUM_SLOTS-1:0] awvalid;
  logic [NUM_SLOTS-1:0] awready;

  logic [NUM_SLOTS*32-1:0] wdata ;
  logic [ NUM_SLOTS*4-1:0] wstrb ;
  logic [   NUM_SLOTS-1:0] wvalid;
  logic [   NUM_SLOTS-1:0] wready;

  logic [NUM_SLOTS*2-1:0] bresp ;
  logic [  NUM_SLOTS-1:0] bvalid;
  logic [  NUM_SLOTS-1:0] bready;

  logic [NUM_SLOTS*32-1:0] araddr;
  // logic[2:0] arprot;
  logic [NUM_SLOTS-1:0] arvalid;
  logic [NUM_SLOTS-1:0] arready;

  logic [NUM_SLOTS*32-1:0] rdata ;
  logic [ NUM_SLOTS*2-1:0] rresp ;
  logic [   NUM_SLOTS-1:0] rvalid;
  logic [   NUM_SLOTS-1:0] rready;

  modport master (
    input awaddr, awvalid, output awready,
    input wdata, wstrb, wvalid, output wready,
    output bresp, bvalid, input bready,
    input araddr, arvalid, output arready,
    output rdata, rresp, rvalid, input rready
  );

  modport slave (
    output awaddr, awvalid, input awready,
    output wdata, wstrb, wvalid, input wready,
    input bresp, bvalid, output bready,
    output araddr, arvalid, input arready,
    input rdata, rresp, rvalid, output rready
  );

endinterface


interface axis_bus_t #(parameter TDATA_WIDTH=32);

  logic [  TDATA_WIDTH-1:0] txd_tdata ;
  logic                     txd_tlast ;
  logic                     txd_tvalid;
  logic                     txd_tready;
  logic [TDATA_WIDTH/8-1:0] txd_tkeep ;

  logic [  TDATA_WIDTH-1:0] rxd_tdata ;
  logic                     rxd_tlast ;
  logic                     rxd_tvalid;
  logic                     rxd_tready;
  logic [TDATA_WIDTH/8-1:0] rxd_tkeep ;

  modport master (
    input txd_tdata, txd_tlast, txd_tvalid, txd_tkeep, output txd_tready,
    output rxd_tdata, rxd_tlast, rxd_tvalid, rxd_tkeep, input rxd_tready
  );

  modport slave (
    output txd_tdata, txd_tlast, txd_tvalid, txd_tkeep, input txd_tready,
    input rxd_tdata, rxd_tlast, rxd_tvalid, rxd_tkeep, output rxd_tready
  );

endinterface


interface cfg_bus_t;

  logic [31:0] addr ;
  logic [31:0] wdata;
  logic        wr   ;
  logic        rd   ;
  logic        ack  ;
  logic [31:0] rdata;

  modport master (input addr, wdata, wr, rd, output ack, rdata);

  modport slave (output addr, wdata, wr, rd, input ack, rdata);

endinterface


interface scrb_bus_t;

  logic [63:0] addr  ;
  logic [ 2:0] state ;
  logic        enable;
  logic        done  ;

  modport master (input enable, output addr, state, done);

  modport slave (output enable, input addr, state, done);

endinterface

`endif //CL_FSB_BUS_PKG
