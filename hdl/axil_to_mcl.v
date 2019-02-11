/**
 *  axil_to_mcl.v
 *
 */

`include "bsg_axi_bus_pkg.vh"
`include "bsg_defines.v"

module axil_to_mcl #(
  mcl_tile_num_p = "inv"
  ,num_tiles_x_p = "inv"
  ,num_tiles_y_p = "inv"
  ,mcl_link_num_lp = mcl_tile_num_p*2
  ,fifo_width_lp = 128
  ,addr_width_p   = "inv"
  ,data_width_p = "inv"
  ,x_cord_width_p = "inv"
  ,y_cord_width_p = "inv"
  ,load_id_width_p = "inv"
  ,fifo_els_p = 4
  ,max_out_credits_p ="inv"
  ,link_sif_width_lp=`bsg_manycore_link_sif_width(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p,load_id_width_p)
  ,packet_width_lp = `bsg_manycore_packet_width(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p,load_id_width_p)
  ,return_packet_width_lp = `bsg_manycore_return_packet_width(x_cord_width_p,y_cord_width_p,data_width_p,load_id_width_p)
) (
  input                                         clk_i
  ,input                                         reset_i
  // axil signals
  ,input                                         s_axil_mcl_awvalid
  ,input  [                                31:0] s_axil_mcl_awaddr
  ,output                                        s_axil_mcl_awready
  ,input                                         s_axil_mcl_wvalid
  ,input  [                                31:0] s_axil_mcl_wdata
  ,input  [                                 3:0] s_axil_mcl_wstrb
  ,output                                        s_axil_mcl_wready
  ,output [                                 1:0] s_axil_mcl_bresp
  ,output                                        s_axil_mcl_bvalid
  ,input                                         s_axil_mcl_bready
  ,input  [                                31:0] s_axil_mcl_araddr
  ,input                                         s_axil_mcl_arvalid
  ,output                                        s_axil_mcl_arready
  ,output [                                31:0] s_axil_mcl_rdata
  ,output [                                 1:0] s_axil_mcl_rresp
  ,output                                        s_axil_mcl_rvalid
  ,input                                         s_axil_mcl_rready
  // manycore links
  ,input  [link_sif_width_lp*mcl_tile_num_p-1:0] link_sif_i
  ,output [link_sif_width_lp*mcl_tile_num_p-1:0] link_sif_o
  ,input  [   x_cord_width_p*mcl_tile_num_p-1:0] my_x_i
  ,input  [   y_cord_width_p*mcl_tile_num_p-1:0] my_y_i
);

parameter axil_mosi_bus_width_lp = `bsg_axil_mosi_bus_width(1);
parameter axil_miso_bus_width_lp = `bsg_axil_miso_bus_width(1);

`declare_bsg_axil_bus_s(1, bsg_axil_mosi_bus_s, bsg_axil_miso_bus_s);
bsg_axil_mosi_bus_s s0_axil_bus_i_cast;
bsg_axil_miso_bus_s s0_axil_bus_o_cast;

assign s0_axil_bus_i_cast.awvalid = s_axil_mcl_awvalid;
assign s0_axil_bus_i_cast.awaddr  = s_axil_mcl_awaddr;
assign s_axil_mcl_awready             = s0_axil_bus_o_cast.awready;

assign s0_axil_bus_i_cast.wvalid = s_axil_mcl_wvalid;
assign s0_axil_bus_i_cast.wdata  = s_axil_mcl_wdata;
assign s0_axil_bus_i_cast.wstrb  = s_axil_mcl_wstrb;
assign s_axil_mcl_wready             = s0_axil_bus_o_cast.wready;

assign s_axil_mcl_bresp              = s0_axil_bus_o_cast.bresp;
assign s_axil_mcl_bvalid             = s0_axil_bus_o_cast.bvalid;
assign s0_axil_bus_i_cast.bready = s_axil_mcl_bready;

assign s0_axil_bus_i_cast.araddr  = s_axil_mcl_araddr;
assign s0_axil_bus_i_cast.arvalid = s_axil_mcl_arvalid;
assign s_axil_mcl_arready             = s0_axil_bus_o_cast.arready;

assign s_axil_mcl_rdata              = s0_axil_bus_o_cast.rdata;
assign s_axil_mcl_rresp              = s0_axil_bus_o_cast.rresp;
assign s_axil_mcl_rvalid             = s0_axil_bus_o_cast.rvalid;
assign s0_axil_bus_i_cast.rready = s_axil_mcl_rready;

localparam mux_bus_num_lp = (mcl_link_num_lp+1);

`declare_bsg_axil_bus_s(mux_bus_num_lp, bsg_axil_mosi_busN_s, bsg_axil_miso_busN_s);
bsg_axil_mosi_busN_s axil_mosi_busN;
bsg_axil_miso_busN_s axil_miso_busN;


//-------------------------------------------------
// TODO: find a way to avoid this list
// crossbar, support 1 to [1:16] now
localparam BASE_ADDRESS_LIST = {
  64'h00000000_00000F00, 64'h00000000_00000E00, 64'h00000000_00000D00 ,64'h00000000_00000C00,
  64'h00000000_00000B00, 64'h00000000_00000A00, 64'h00000000_00000900 ,64'h00000000_00000800,
  64'h00000000_00000700, 64'h00000000_00000600, 64'h00000000_00000500, 64'h00000000_00000400, 
  64'h00000000_00000300, 64'h00000000_00000200, 64'h00000000_00000100, 64'h00000000_00000000};
localparam C_NUM_MASTER_SLOTS = mux_bus_num_lp;
localparam C_M_AXI_BASE_ADDR  = BASE_ADDRESS_LIST[64*C_NUM_MASTER_SLOTS-1:0];
localparam C_M_AXI_ADDR_WIDTH         = {C_NUM_MASTER_SLOTS{32'h0000_0008}};
localparam C_M_AXI_WRITE_CONNECTIVITY = {C_NUM_MASTER_SLOTS{32'h0000_0001}};
localparam C_M_AXI_READ_CONNECTIVITY  = {C_NUM_MASTER_SLOTS{32'h0000_0001}};
localparam C_M_AXI_WRITE_ISSUING      = {C_NUM_MASTER_SLOTS{32'h0000_0001}};
localparam C_M_AXI_READ_ISSUING       = {C_NUM_MASTER_SLOTS{32'h0000_0001}};
localparam C_M_AXI_SECURE             = {C_NUM_MASTER_SLOTS{32'h0000_0000}};

axi_crossbar_v2_1_18_axi_crossbar #(
  .C_FAMILY                   ("virtexuplus"             ),
  .C_NUM_SLAVE_SLOTS          (1                         ),
  .C_NUM_MASTER_SLOTS         (C_NUM_MASTER_SLOTS        ),
  .C_AXI_ID_WIDTH             (1                         ),
  .C_AXI_ADDR_WIDTH           (32                        ),
  .C_AXI_DATA_WIDTH           (32                        ),
  .C_AXI_PROTOCOL             (2                         ),
  .C_NUM_ADDR_RANGES          (1                         ),
  .C_M_AXI_BASE_ADDR          (C_M_AXI_BASE_ADDR         ),
  .C_M_AXI_ADDR_WIDTH         (C_M_AXI_ADDR_WIDTH        ),
  .C_S_AXI_BASE_ID            (32'H00000000              ),
  .C_S_AXI_THREAD_ID_WIDTH    (32'H00000000              ),
  .C_AXI_SUPPORTS_USER_SIGNALS(0                         ),
  .C_AXI_AWUSER_WIDTH         (1                         ),
  .C_AXI_ARUSER_WIDTH         (1                         ),
  .C_AXI_WUSER_WIDTH          (1                         ),
  .C_AXI_RUSER_WIDTH          (1                         ),
  .C_AXI_BUSER_WIDTH          (1                         ),
  .C_M_AXI_WRITE_CONNECTIVITY (C_M_AXI_WRITE_CONNECTIVITY),
  .C_M_AXI_READ_CONNECTIVITY  (C_M_AXI_READ_CONNECTIVITY ),
  .C_R_REGISTER               (1                         ),
  .C_S_AXI_SINGLE_THREAD      (32'H00000001              ),
  .C_S_AXI_WRITE_ACCEPTANCE   (32'H00000001              ),
  .C_S_AXI_READ_ACCEPTANCE    (32'H00000001              ),
  .C_M_AXI_WRITE_ISSUING      (C_M_AXI_WRITE_ISSUING     ),
  .C_M_AXI_READ_ISSUING       (C_M_AXI_READ_ISSUING      ),
  .C_S_AXI_ARB_PRIORITY       (32'H00000000              ),
  .C_M_AXI_SECURE             (C_M_AXI_SECURE            ),
  .C_CONNECTIVITY_MODE        (0                         )
) axil_crossbar_mux (
  .aclk          (clk_i                     ),
  .aresetn       (~reset_i                  ),
  .s_axi_awid    (1'H0                      ),
  .s_axi_awaddr  (s0_axil_bus_i_cast.awaddr ),
  .s_axi_awlen   (8'H00                     ),
  .s_axi_awsize  (3'H0                      ),
  .s_axi_awburst (2'H0                      ),
  .s_axi_awlock  (1'H0                      ),
  .s_axi_awcache (4'H0                      ),
  .s_axi_awprot  (3'H0                      ),
  .s_axi_awqos   (4'H0                      ),
  .s_axi_awuser  (1'H0                      ),
  .s_axi_awvalid (s0_axil_bus_i_cast.awvalid),
  .s_axi_awready (s0_axil_bus_o_cast.awready),
  .s_axi_wid     (1'H0                      ),
  .s_axi_wdata   (s0_axil_bus_i_cast.wdata  ),
  .s_axi_wstrb   (s0_axil_bus_i_cast.wstrb  ),
  .s_axi_wlast   (1'H1                      ),
  .s_axi_wuser   (1'H0                      ),
  .s_axi_wvalid  (s0_axil_bus_i_cast.wvalid ),
  .s_axi_wready  (s0_axil_bus_o_cast.wready ),
  .s_axi_bid     (                          ),
  .s_axi_bresp   (s0_axil_bus_o_cast.bresp  ),
  .s_axi_buser   (                          ),
  .s_axi_bvalid  (s0_axil_bus_o_cast.bvalid ),
  .s_axi_bready  (s0_axil_bus_i_cast.bready ),
  .s_axi_arid    (1'H0                      ),
  .s_axi_araddr  (s0_axil_bus_i_cast.araddr ),
  .s_axi_arlen   (8'H00                     ),
  .s_axi_arsize  (3'H0                      ),
  .s_axi_arburst (2'H0                      ),
  .s_axi_arlock  (1'H0                      ),
  .s_axi_arcache (4'H0                      ),
  .s_axi_arprot  (3'H0                      ),
  .s_axi_arqos   (4'H0                      ),
  .s_axi_aruser  (1'H0                      ),
  .s_axi_arvalid (s0_axil_bus_i_cast.arvalid),
  .s_axi_arready (s0_axil_bus_o_cast.arready),
  .s_axi_rid     (                          ),
  .s_axi_rdata   (s0_axil_bus_o_cast.rdata  ),
  .s_axi_rresp   (s0_axil_bus_o_cast.rresp  ),
  .s_axi_rlast   (                          ),
  .s_axi_ruser   (                          ),
  .s_axi_rvalid  (s0_axil_bus_o_cast.rvalid ),
  .s_axi_rready  (s0_axil_bus_i_cast.rready ),
  .m_axi_awid    (                          ),
  .m_axi_awaddr  (axil_mosi_busN.awaddr     ),
  .m_axi_awlen   (                          ),
  .m_axi_awsize  (                          ),
  .m_axi_awburst (                          ),
  .m_axi_awlock  (                          ),
  .m_axi_awcache (                          ),
  .m_axi_awprot  (                          ),
  .m_axi_awregion(                          ),
  .m_axi_awqos   (                          ),
  .m_axi_awuser  (                          ),
  .m_axi_awvalid (axil_mosi_busN.awvalid    ),
  .m_axi_awready (axil_miso_busN.awready    ),
  .m_axi_wid     (                          ),
  .m_axi_wdata   (axil_mosi_busN.wdata      ),
  .m_axi_wstrb   (axil_mosi_busN.wstrb      ),
  .m_axi_wlast   (                          ),
  .m_axi_wuser   (                          ),
  .m_axi_wvalid  (axil_mosi_busN.wvalid     ),
  .m_axi_wready  (axil_miso_busN.wready     ),
  .m_axi_bid     ({C_NUM_MASTER_SLOTS{1'b0}}),
  .m_axi_bresp   (axil_miso_busN.bresp      ),
  .m_axi_buser   ({C_NUM_MASTER_SLOTS{1'b0}}),
  .m_axi_bvalid  (axil_miso_busN.bvalid     ),
  .m_axi_bready  (axil_mosi_busN.bready     ),
  .m_axi_arid    (                          ),
  .m_axi_araddr  (axil_mosi_busN.araddr     ),
  .m_axi_arlen   (                          ),
  .m_axi_arsize  (                          ),
  .m_axi_arburst (                          ),
  .m_axi_arlock  (                          ),
  .m_axi_arcache (                          ),
  .m_axi_arprot  (                          ),
  .m_axi_arregion(                          ),
  .m_axi_arqos   (                          ),
  .m_axi_aruser  (                          ),
  .m_axi_arvalid (axil_mosi_busN.arvalid    ),
  .m_axi_arready (axil_miso_busN.arready    ),
  .m_axi_rid     ({C_NUM_MASTER_SLOTS{1'b0}}),
  .m_axi_rdata   (axil_miso_busN.rdata      ),
  .m_axi_rresp   (axil_miso_busN.rresp      ),
  .m_axi_rlast   ({C_NUM_MASTER_SLOTS{1'b1}}),
  .m_axi_ruser   ({C_NUM_MASTER_SLOTS{1'b0}}),
  .m_axi_rvalid  (axil_miso_busN.rvalid     ),
  .m_axi_rready  (axil_mosi_busN.rready     )
);


logic [mux_bus_num_lp-1:0][axil_mosi_bus_width_lp-1:0] axil_demux_mosi_bus;
logic [mux_bus_num_lp-1:0][axil_miso_bus_width_lp-1:0] axil_demux_miso_bus;

logic [mcl_link_num_lp-1:0][$clog2(max_out_credits_p+1)-1:0] rcv_fifo_vacancy_lo;

genvar i;
// TODO: find a better way to read field out of the structure
for (i=0; i<mux_bus_num_lp; i=i+1)
begin
  always_comb
  begin
    axil_demux_mosi_bus[i] = {
        axil_mosi_busN.awaddr[32*i+:32]
        ,axil_mosi_busN.awvalid[i]
        ,axil_mosi_busN.wdata[32*i+:32]
        ,axil_mosi_busN.wstrb[4*i+:4]
        ,axil_mosi_busN.wvalid[i]
        ,axil_mosi_busN.bready[i]
        ,axil_mosi_busN.araddr[32*i+:32]
        ,axil_mosi_busN.arvalid[i]
        ,axil_mosi_busN.rready[i]
    };

    {
        axil_miso_busN.awready[i]
        ,axil_miso_busN.wready[i]
        ,axil_miso_busN.bresp[2*i+:2]
        ,axil_miso_busN.bvalid[i]
        ,axil_miso_busN.arready[i]
        ,axil_miso_busN.rdata[32*i+:32]
        ,axil_miso_busN.rresp[2*i+:2]
        ,axil_miso_busN.rvalid[i]
    } = axil_demux_miso_bus[i];
    end
  end


  // from mcl signals
  logic [mcl_link_num_lp-1:0]                    fifo_v_li    ;
  logic [mcl_link_num_lp-1:0][fifo_width_lp-1:0] fifo_data_li ;
  logic [mcl_link_num_lp-1:0]                    fifo_ready_lo;

  // to mcl slave
  logic [mcl_link_num_lp-1:0]                    fifo_v_lo    ;
  logic [mcl_link_num_lp-1:0][fifo_width_lp-1:0] fifo_data_lo ;
  logic [mcl_link_num_lp-1:0]                    fifo_ready_li;

for (i=0; i<mcl_link_num_lp; i=i+1)
begin
  s_axil_mcl_adapter #(
    .mcl_width_p      (fifo_width_lp    ),
    .max_out_credits_p(max_out_credits_p)
  ) s_axil_to_mcl (
    .clk_i           (clk_i                 )
    ,.reset_i         (reset_i               )
    ,.s_axil_mcl_bus_i(axil_demux_mosi_bus[i])
    ,.s_axil_mcl_bus_o(axil_demux_miso_bus[i])
    ,.mcl_v_i         (fifo_v_li[i]          )
    ,.mcl_data_i      (fifo_data_li[i]       )
    ,.mcl_r_o         (fifo_ready_lo[i]      )
    ,.mcl_v_o         (fifo_v_lo[i]          )
    ,.mcl_data_o      (fifo_data_lo[i]       )
    ,.mcl_r_i         (fifo_ready_li[i]      )
    ,.rcv_vacancy_o   (rcv_fifo_vacancy_lo[i])
  );
end


logic [mcl_tile_num_p-1:0]                        endpoint_in_v_lo   ;
logic [mcl_tile_num_p-1:0]                        endpoint_in_yumi_li;
logic [mcl_tile_num_p-1:0][     data_width_p-1:0] endpoint_in_data_lo;
logic [mcl_tile_num_p-1:0][(data_width_p>>3)-1:0] endpoint_in_mask_lo;
logic [mcl_tile_num_p-1:0][     addr_width_p-1:0] endpoint_in_addr_lo;
logic [mcl_tile_num_p-1:0]                        endpoint_in_we_lo  ;
logic [mcl_tile_num_p-1:0][   x_cord_width_p-1:0] in_src_x_cord_lo;
logic [mcl_tile_num_p-1:0][   y_cord_width_p-1:0] in_src_y_cord_lo;

logic [mcl_tile_num_p-1:0]                      endpoint_out_v_li     ;
logic [mcl_tile_num_p-1:0][packet_width_lp-1:0] endpoint_out_packet_li;
logic [mcl_tile_num_p-1:0]                      endpoint_out_ready_lo ;

logic [mcl_tile_num_p-1:0][   data_width_p-1:0] returned_data_r_lo   ;
logic [mcl_tile_num_p-1:0][load_id_width_p-1:0] returned_load_id_r_lo;
logic [mcl_tile_num_p-1:0]                      returned_v_r_lo      ;
logic [mcl_tile_num_p-1:0]                      returned_fifo_full_lo;
logic [mcl_tile_num_p-1:0]                      returned_yumi_li     ;

logic [mcl_tile_num_p-1:0][return_packet_width_lp-1:0] returning_data_li;
logic [mcl_tile_num_p-1:0]                             returning_v_li   ;

logic [mcl_tile_num_p-1:0][$clog2(max_out_credits_p+1)-1:0] out_credits_lo;

logic [mcl_tile_num_p-1:0][x_cord_width_p-1:0] my_x_li;
logic [mcl_tile_num_p-1:0][y_cord_width_p-1:0] my_y_li;

assign my_x_li = my_x_i;
assign my_y_li = my_y_i;


for (i=0; i<mcl_tile_num_p; i=i+1)
begin

  // fifo to manycore
  assign {returning_v_li[i], endpoint_out_v_li[i]} = fifo_v_lo[2*i+:2];
  assign {returning_data_li[i], endpoint_out_packet_li[i]} = {
    (return_packet_width_lp)'(fifo_data_lo[2*i+1]),
    (packet_width_lp)'(fifo_data_lo[2*i])
  };

  assign fifo_ready_li[2*i+:2] = {1'b1, endpoint_out_ready_lo[i]};


  // manycore to fifo
  assign fifo_data_li[2*i] = (fifo_width_lp)'({
    returned_fifo_full_lo // additional 1 bit
    ,`ePacketType_data
    ,returned_data_r_lo
    ,returned_load_id_r_lo
    ,my_y_i
    ,my_x_i
  });

  assign fifo_data_li[2*i+1] = (fifo_width_lp)'({
    endpoint_in_addr_lo
    ,{1'b0, endpoint_in_we_lo} // only support remote load(0)/store(1)
    ,endpoint_in_mask_lo
    ,endpoint_in_data_lo
    ,in_src_y_cord_lo
    ,in_src_x_cord_lo
    ,my_y_i
    ,my_x_i
  });

  assign fifo_v_li[2*i+:2] = {endpoint_in_v_lo[i], returned_v_r_lo[i]};
  assign {endpoint_in_yumi_li[i], returned_yumi_li[i]} = fifo_ready_lo[2*i+:2] & fifo_v_li[2*i+:2];

end


for (i=0; i<mcl_tile_num_p; i=i+1)
begin
  bsg_manycore_endpoint_standard #(
    .x_cord_width_p   (x_cord_width_p   ),
    .y_cord_width_p   (y_cord_width_p   ),
    .fifo_els_p       (fifo_els_p       ),
    .data_width_p     (data_width_p     ),
    .addr_width_p     (addr_width_p     ),
    .max_out_credits_p(max_out_credits_p),
    .load_id_width_p  (load_id_width_p  )
  ) dram_endpoint_standard (
    .clk_i               (clk_i                    ),
    .reset_i             (reset_i                  ),
    
    .link_sif_i          (link_sif_i               ),
    .link_sif_o          (link_sif_o               ),
    
    // manycore packet -> fifo
    .in_v_o              (endpoint_in_v_lo[i]      ),
    .in_yumi_i           (endpoint_in_yumi_li[i]   ),
    .in_data_o           (endpoint_in_data_lo[i]   ),
    .in_mask_o           (endpoint_in_mask_lo[i]   ),
    .in_addr_o           (endpoint_in_addr_lo[i]   ),
    .in_we_o             (endpoint_in_we_lo[i]     ),
    .in_src_x_cord_o     (in_src_x_cord_lo[i]      ),
    .in_src_y_cord_o     (in_src_y_cord_lo[i]      ),
    
    // fifo -> manycore packet
    .out_v_i             (endpoint_out_v_li[i]     ),
    .out_packet_i        (endpoint_out_packet_li[i]),
    .out_ready_o         (endpoint_out_ready_lo[i] ),
    
    // manycore credit -> fifo
    .returned_data_r_o   (returned_data_r_lo[i]    ),
    .returned_load_id_r_o(returned_load_id_r_lo[i] ),
    .returned_v_r_o      (returned_v_r_lo[i]       ),
    .returned_fifo_full_o(returned_fifo_full_lo[i] ), // always 1'b1 if returned_fifo_p is not set
    .returned_yumi_i     (returned_yumi_li[i]      ),
    
    // fifo -> manycore credit
    .returning_data_i    (returning_data_li[i]     ),
    .returning_v_i       (returning_v_li[i]        ),
    
    .out_credits_o       (out_credits_lo[i]        ),
    .my_x_i              (my_x_li[i]               ),
    .my_y_i              (my_y_li[i]               )
  );
end


localparam axil_mon_base_addr = BASE_ADDRESS_LIST[64*mcl_link_num_lp+:64];
wire axil_mon_wen_lo, axil_mon_ren_lo;
wire [7:0] mon_addr_li;
logic [31:0] mon_data_lo;

  axil_to_mem #(
    .mem_addr_width_p      (8                 ),
    .axil_base_addr_p      (axil_mon_base_addr),
    .axil_base_addr_width_p(8                 )
  ) axi_mem_rw (
    .clk_i       (clk_i                               ),
    .reset_i     (reset_i                             ),
    .s_axil_bus_i(axil_demux_mosi_bus[mcl_link_num_lp]),
    .s_axil_bus_o(axil_demux_miso_bus[mcl_link_num_lp]),
    .addr_o      (mon_addr_li                         ),
    .wen_o       (axil_mon_wen_lo                     ),
    .data_o      (                                    ),
    .ren_o       (axil_mon_ren_lo                     ),
    .data_i      (mon_data_lo                         ),
    .done        (                                    )
  );

always_comb
begin
  if (mon_addr_li<8'h10)
    mon_data_lo = (32)'(rcv_fifo_vacancy_lo[mon_addr_li]);
  else if (mon_addr_li<8'h20)
    mon_data_lo = (32)'(out_credits_lo[mon_addr_li[3:0]]);
  else if (mon_addr_li==8'h20)
    mon_data_lo = (32)'(num_tiles_x_p);
  else if (mon_addr_li==8'h24)
    mon_data_lo = (32)'(num_tiles_y_p);
  else
    mon_data_lo = 32'hFFFF_FFFF;
end


// synopsys translate_off
  always_ff @(negedge clk_i)
    begin
      assert (fifo_width_lp >= return_packet_width_lp + $clog2(max_out_credits_p+1))
        else begin
          $error("## errant fifo width %d is less than the manycore returned packet width %d", fifo_width_lp, return_packet_width_lp);
          $finish();
        end
      assert (fifo_width_lp >= (packet_width_lp + 1))
        else begin
          $error("## errant fifo width %d is less than the manycore packet width %d", fifo_width_lp, packet_width_lp);
          $finish();
        end
    end
// synopsys translate_on

endmodule
