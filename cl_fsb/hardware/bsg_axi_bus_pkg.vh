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
  ( slot_num_lp * \
    (3*32 + 5 + 4) \
  )

`define bsg_axil_miso_bus_width(slot_num_lp) \
  ( slot_num_lp * \
    (5 + 2*2 + 32) \
  )


`define declare_bsg_axi_bus_s(slot_num_lp, id_width_p, addr_width_p, data_width_p) \
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
} bsg_axi_mosi_bus_s; \
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
} bsg_axi_miso_bus_s

`define bsg_axi_mosi_bus_width(slot_num_lp, id_width_p, addr_width_p, data_width_p) \
( slot_num_lp * \
  (2*id_width_p + addr_width_p + 2*8 + 2*4 + 6 + data_width_p + data_width_p/8) \
)

`define bsg_axi_miso_bus_width(slot_num_lp, id_width_p, addr_width_p, data_width_p) \
( slot_num_lp * \
  (7 + 2*id_width_p + 2*2 + data_width_p) \
)


`endif
