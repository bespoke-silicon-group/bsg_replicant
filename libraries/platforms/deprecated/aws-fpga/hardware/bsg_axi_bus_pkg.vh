// Copyright (c) 2019, University of Washington All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list
// of conditions and the following disclaimer.
// 
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
// 
// Neither the name of the copyright holder nor the names of its contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

`ifndef BSG_AXI_BUS_PKG_VH
`define BSG_AXI_BUS_PKG_VH

`define declare_bsg_axil_bus_s(slot_num_lp, mosi_struct_name, miso_struct_name) \
typedef struct packed { \
  logic [slot_num_lp*32-1:0] awaddr ; \
  logic [   slot_num_lp-1:0] awvalid; \
  logic [slot_num_lp*32-1:0] wdata  ; \
  logic [ slot_num_lp*4-1:0] wstrb  ; \
  logic [   slot_num_lp-1:0] wvalid ; \
  \
  logic [slot_num_lp-1:0] bready; \
  \
  logic [slot_num_lp*32-1:0] araddr ; \
  logic [   slot_num_lp-1:0] arvalid; \
  logic [   slot_num_lp-1:0] rready ; \
} mosi_struct_name; \
\
typedef struct packed { \
  logic [slot_num_lp-1:0] awready; \
  logic [slot_num_lp-1:0] wready ; \
  \
  logic [slot_num_lp*2-1:0] bresp ; \
  logic [  slot_num_lp-1:0] bvalid; \
  \
  logic [   slot_num_lp-1:0] arready; \
  logic [slot_num_lp*32-1:0] rdata  ; \
  logic [ slot_num_lp*2-1:0] rresp  ; \
  logic [   slot_num_lp-1:0] rvalid ; \
  \
} miso_struct_name

`define bsg_axil_mosi_bus_width(slot_num_lp) \
( slot_num_lp * (3*32 + 5 + 4) \
)

`define bsg_axil_miso_bus_width(slot_num_lp) \
( slot_num_lp * (5 + 2*2 + 32) \
)


`define declare_bsg_axi_bus_s(slot_num_lp, id_width_p, addr_width_p, data_width_p, mosi_struct_name, miso_struct_name) \
typedef struct packed { \
  logic [    slot_num_lp*id_width_p-1:0] awid   ; \
  logic [  slot_num_lp*addr_width_p-1:0] awaddr ; \
  logic [             slot_num_lp*8-1:0] awlen  ; \
  logic [             slot_num_lp*3-1:0] awsize ; \
  logic [               slot_num_lp-1:0] awvalid; \
  logic [    slot_num_lp*id_width_p-1:0] wid    ; \
  logic [  slot_num_lp*data_width_p-1:0] wdata  ; \
  logic [slot_num_lp*data_width_p/8-1:0] wstrb  ; \
  logic [               slot_num_lp-1:0] wlast  ; \
  logic [               slot_num_lp-1:0] wvalid ; \
  \
  logic [slot_num_lp-1:0] bready; \
  \
  logic [  slot_num_lp*id_width_p-1:0] arid   ; \
  logic [slot_num_lp*addr_width_p-1:0] araddr ; \
  logic [           slot_num_lp*8-1:0] arlen  ; \
  logic [           slot_num_lp*3-1:0] arsize ; \
  logic [             slot_num_lp-1:0] arvalid; \
  logic [             slot_num_lp-1:0] rready ; \
} mosi_struct_name; \
\
typedef struct packed { \
  logic [slot_num_lp-1:0] awready; \
  logic [slot_num_lp-1:0] wready ; \
  \
  logic [slot_num_lp*id_width_p-1:0] bid   ; \
  logic [         slot_num_lp*2-1:0] bresp ; \
  logic [           slot_num_lp-1:0] bvalid; \
  logic [           slot_num_lp-1:0] bready; \
  \
  logic [             slot_num_lp-1:0] arready; \
  logic [  slot_num_lp*id_width_p-1:0] rid    ; \
  logic [slot_num_lp*data_width_p-1:0] rdata  ; \
  logic [           slot_num_lp*2-1:0] rresp  ; \
  logic [             slot_num_lp-1:0] rlast  ; \
  logic [             slot_num_lp-1:0] rvalid ; \
} miso_struct_name

`define bsg_axi_mosi_bus_width(slot_num_lp, id_width_p, addr_width_p, data_width_p) \
( slot_num_lp * \
  (3*id_width_p + 2*addr_width_p + 2*8 + 2*3 + 6 + data_width_p + data_width_p/8) \
)

`define bsg_axi_miso_bus_width(slot_num_lp, id_width_p, addr_width_p, data_width_p) \
( slot_num_lp * \
  (7 + 2*id_width_p + 2*2 + data_width_p) \
)


`define declare_bsg_axis_bus_s(data_width_p, mosi_struct_name, miso_struct_name) \
typedef struct packed { \
  logic [  data_width_p-1:0] txd_tdata ; \
  logic [data_width_p/8-1:0] txd_tkeep ; \
  logic                      txd_tlast ; \
  logic                      txd_tvalid; \
  \
  logic rxd_tready; \
} mosi_struct_name; \
\
typedef struct packed { \
  logic txd_tready; \
  logic [  data_width_p-1:0] rxd_tdata ; \
  logic [data_width_p/8-1:0] rxd_tkeep ; \
  logic                      rxd_tlast ; \
  logic                      rxd_tvalid; \
} miso_struct_name

`define bsg_axis_bus_width(data_width_p) \
  (data_width_p + 3 + data_width_p/8)

`endif
