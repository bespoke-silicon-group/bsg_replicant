/**
*  axi4_mux.v
*
*/

`include "bsg_axi_bus_pkg.vh"

module axi4_mux #(
	parameter slot_num_p = "inv"
	, parameter id_width_p = "inv"
	, parameter addr_width_p = "inv"
	, parameter data_width_p = "inv"
	, localparam axi4_mosi_bus_width_lp = `bsg_axi4_mosi_bus_width(1, id_width_p, addr_width_p, data_width_p)
	, localparam axi4_miso_bus_width_lp = `bsg_axi4_miso_bus_width(1, id_width_p, addr_width_p, data_width_p)
) (
	input                                                           clk_i
	,input                                                           reset_i
	,input  [            slot_num_p-1:0][axi4_mosi_bus_width_lp-1:0] s_axi4_mux_i
	,output [            slot_num_p-1:0][axi4_miso_bus_width_lp-1:0] s_axi4_mux_o
	,output [axi4_mosi_bus_width_lp-1:0]                             m_axi4_bus_o
	,input  [axi4_miso_bus_width_lp-1:0]                             m_axi4_bus_i
);

//---------------------------------------------
// Concatenated Signals
//---------------------------------------------
	`declare_bsg_axi4_bus_s(slot_num_p, id_width_p, addr_width_p, data_width_p, bsg_axi4_mosi_mux_s, bsg_axi4_miso_mux_s);
	bsg_axi4_mosi_mux_s s_axi4_mux_i_cast;
	bsg_axi4_miso_mux_s s_axi4_mux_o_cast;

	logic [slot_num_p-1:0][axi4_mosi_bus_width_lp-1:0] s_axi4_mux_li;
	logic [slot_num_p-1:0][axi4_miso_bus_width_lp-1:0] s_axi4_mux_lo;

	assign s_axi4_mux_li = s_axi4_mux_i;
	assign s_axi4_mux_o  = s_axi4_mux_lo;

	for (genvar i=0; i<slot_num_p; i=i+1) begin
		always_comb begin: axi4_mux_assignment
			{s_axi4_mux_i_cast.awid[i*id_width_p+:id_width_p]
				,s_axi4_mux_i_cast.awaddr[i*addr_width_p+:addr_width_p]
				,s_axi4_mux_i_cast.awlen[i*8+:8]
				,s_axi4_mux_i_cast.awsize[i*3+:3]
				,s_axi4_mux_i_cast.awburst[i*2+:2]
				,s_axi4_mux_i_cast.awlock[i]
				,s_axi4_mux_i_cast.awcache[i*4+:4]
				,s_axi4_mux_i_cast.awprot[i*3+:3]
				,s_axi4_mux_i_cast.awqos[i*4+:4]
				,s_axi4_mux_i_cast.awregion[i*4+:4]
				,s_axi4_mux_i_cast.awvalid[i]

				,s_axi4_mux_i_cast.wdata[i*data_width_p+:data_width_p]
				,s_axi4_mux_i_cast.wstrb[i*(data_width_p/8)+:(data_width_p/8)]
				,s_axi4_mux_i_cast.wlast[i]
				,s_axi4_mux_i_cast.wvalid[i]

				,s_axi4_mux_i_cast.bready[i]

				,s_axi4_mux_i_cast.arid[i*id_width_p+:id_width_p]
				,s_axi4_mux_i_cast.araddr[i*addr_width_p+:addr_width_p]
				,s_axi4_mux_i_cast.arlen[i*8+:8]
				,s_axi4_mux_i_cast.arsize[i*3+:3]
				,s_axi4_mux_i_cast.arburst[i*2+:2]
				,s_axi4_mux_i_cast.arlock[i]
				,s_axi4_mux_i_cast.arcache[i*4+:4]
				,s_axi4_mux_i_cast.arprot[i*3+:3]
				,s_axi4_mux_i_cast.arqos[i*4+:4]
				,s_axi4_mux_i_cast.arregion[i*4+:4]
				,s_axi4_mux_i_cast.arvalid[i]

				,s_axi4_mux_i_cast.rready[i]
			} = s_axi4_mux_li[i];

			s_axi4_mux_lo[i] = {s_axi4_mux_o_cast.awready[i]
				,s_axi4_mux_o_cast.wready[i]

				,s_axi4_mux_o_cast.bid[i*id_width_p+:id_width_p]
				,s_axi4_mux_o_cast.bresp[i*2+:2]
				,s_axi4_mux_o_cast.bvalid[i]

				,s_axi4_mux_o_cast.arready[i]

				,s_axi4_mux_o_cast.rid[i*id_width_p+:id_width_p]
				,s_axi4_mux_o_cast.rdata[i*data_width_p+:data_width_p]
				,s_axi4_mux_o_cast.rresp[i*2+:2]
				,s_axi4_mux_o_cast.rlast[i]
				,s_axi4_mux_o_cast.rvalid[i]
			};
		end
	end

	`declare_bsg_axi4_bus_s(1, id_width_p, addr_width_p, data_width_p, bsg_axi4_mosi_bus_s, bsg_axi4_miso_bus_s);
	bsg_axi4_mosi_bus_s m_axi4_bus_o_cast;
	bsg_axi4_miso_bus_s m_axi4_bus_i_cast;

	assign m_axi4_bus_o      = m_axi4_bus_o_cast;
	assign m_axi4_bus_i_cast = m_axi4_bus_i;

	axi_crossbar_v2_1_18_axi_crossbar #(
		.C_FAMILY                   ("virtexuplus"                        ),
		.C_NUM_SLAVE_SLOTS          (slot_num_p                           ),
		.C_NUM_MASTER_SLOTS         (1                                    ),
		.C_AXI_ID_WIDTH             (id_width_p                           ),
		.C_AXI_ADDR_WIDTH           (addr_width_p                         ),
		.C_AXI_DATA_WIDTH           (data_width_p                         ),
		.C_AXI_PROTOCOL             (0                                    ), // 0 is AXI4 Full
		.C_NUM_ADDR_RANGES          (1                                    ),
		.C_M_AXI_BASE_ADDR          (128'H00000000000000000000000000000000),
		.C_M_AXI_ADDR_WIDTH         (64'H0000004000000040                 ),
		.C_S_AXI_BASE_ID            (64'H0000000000000000                 ),
		.C_S_AXI_THREAD_ID_WIDTH    (64'H0000000500000005                 ),
		.C_AXI_SUPPORTS_USER_SIGNALS(0                                    ),
		.C_AXI_AWUSER_WIDTH         (1                                    ),
		.C_AXI_ARUSER_WIDTH         (1                                    ),
		.C_AXI_WUSER_WIDTH          (1                                    ),
		.C_AXI_RUSER_WIDTH          (1                                    ),
		.C_AXI_BUSER_WIDTH          (1                                    ),
		.C_M_AXI_WRITE_CONNECTIVITY (32'H00000003                         ),
		.C_M_AXI_READ_CONNECTIVITY  (32'H00000003                         ),
		.C_R_REGISTER               (0                                    ),
		.C_S_AXI_SINGLE_THREAD      (64'H0000000000000000                 ),
		.C_S_AXI_WRITE_ACCEPTANCE   (64'H0000000200000002                 ),
		.C_S_AXI_READ_ACCEPTANCE    (64'H0000000200000002                 ),
		.C_M_AXI_WRITE_ISSUING      (32'H00000004                         ),
		.C_M_AXI_READ_ISSUING       (32'H00000004                         ),
		.C_S_AXI_ARB_PRIORITY       (64'H0000000000000000                 ),
		.C_M_AXI_SECURE             (32'H00000000                         ),
		.C_CONNECTIVITY_MODE        (0                                    )
	) xbar (
		.aclk          (clk_i                     ),
		.aresetn       (~reset_i                  ),
		.s_axi_awid    (s_axi4_mux_i_cast.awid    ),
		.s_axi_awaddr  (s_axi4_mux_i_cast.awaddr  ),
		.s_axi_awlen   (s_axi4_mux_i_cast.awlen   ),
		.s_axi_awsize  (s_axi4_mux_i_cast.awsize  ),
		.s_axi_awburst (s_axi4_mux_i_cast.awburst ),
		.s_axi_awlock  (s_axi4_mux_i_cast.awlock  ),
		.s_axi_awcache (s_axi4_mux_i_cast.awcache ),
		.s_axi_awprot  (s_axi4_mux_i_cast.awprot  ),
		.s_axi_awqos   (s_axi4_mux_i_cast.awqos   ),
		.s_axi_awuser  ('0                        ),
		.s_axi_awvalid (s_axi4_mux_i_cast.awvalid ),
		.s_axi_awready (s_axi4_mux_o_cast.awready ),
		.s_axi_wid     ('0                        ),
		.s_axi_wdata   (s_axi4_mux_i_cast.wdata   ),
		.s_axi_wstrb   (s_axi4_mux_i_cast.wstrb   ),
		.s_axi_wlast   (s_axi4_mux_i_cast.wlast   ),
		.s_axi_wuser   ('0                        ),
		.s_axi_wvalid  (s_axi4_mux_i_cast.wvalid  ),
		.s_axi_wready  (s_axi4_mux_o_cast.wready  ),
		.s_axi_bid     (s_axi4_mux_o_cast.bid     ),
		.s_axi_bresp   (s_axi4_mux_o_cast.bresp   ),
		.s_axi_buser   (                          ),
		.s_axi_bvalid  (s_axi4_mux_o_cast.bvalid  ),
		.s_axi_bready  (s_axi4_mux_i_cast.bready  ),
		.s_axi_arid    (s_axi4_mux_i_cast.arid    ),
		.s_axi_araddr  (s_axi4_mux_i_cast.araddr  ),
		.s_axi_arlen   (s_axi4_mux_i_cast.arlen   ),
		.s_axi_arsize  (s_axi4_mux_i_cast.arsize  ),
		.s_axi_arburst (s_axi4_mux_i_cast.arburst ),
		.s_axi_arlock  (s_axi4_mux_i_cast.arlock  ),
		.s_axi_arcache (s_axi4_mux_i_cast.arcache ),
		.s_axi_arprot  (s_axi4_mux_i_cast.arprot  ),
		.s_axi_arqos   (s_axi4_mux_i_cast.arqos   ),
		.s_axi_aruser  ('0                        ),
		.s_axi_arvalid (s_axi4_mux_i_cast.arvalid ),
		.s_axi_arready (s_axi4_mux_o_cast.arready ),
		.s_axi_rid     (s_axi4_mux_o_cast.rid     ),
		.s_axi_rdata   (s_axi4_mux_o_cast.rdata   ),
		.s_axi_rresp   (s_axi4_mux_o_cast.rresp   ),
		.s_axi_rlast   (s_axi4_mux_o_cast.rlast   ),
		.s_axi_ruser   (                          ),
		.s_axi_rvalid  (s_axi4_mux_o_cast.rvalid  ),
		.s_axi_rready  (s_axi4_mux_i_cast.rready  ),
		// master
		.m_axi_awid    (m_axi4_bus_o_cast.awid    ),
		.m_axi_awaddr  (m_axi4_bus_o_cast.awaddr  ),
		.m_axi_awlen   (m_axi4_bus_o_cast.awlen   ),
		.m_axi_awsize  (m_axi4_bus_o_cast.awsize  ),
		.m_axi_awburst (m_axi4_bus_o_cast.awburst ),
		.m_axi_awlock  (m_axi4_bus_o_cast.awlock  ),
		.m_axi_awcache (m_axi4_bus_o_cast.awcache ),
		.m_axi_awprot  (m_axi4_bus_o_cast.awprot  ),
		.m_axi_awregion(m_axi4_bus_o_cast.awregion),
		.m_axi_awqos   (m_axi4_bus_o_cast.awqos   ),
		.m_axi_awuser  (                          ),
		.m_axi_awvalid (m_axi4_bus_o_cast.awvalid ),
		.m_axi_awready (m_axi4_bus_i_cast.awready ),
		.m_axi_wid     (                          ),
		.m_axi_wdata   (m_axi4_bus_o_cast.wdata   ),
		.m_axi_wstrb   (m_axi4_bus_o_cast.wstrb   ),
		.m_axi_wlast   (m_axi4_bus_o_cast.wlast   ),
		.m_axi_wuser   (                          ),
		.m_axi_wvalid  (m_axi4_bus_o_cast.wvalid  ),
		.m_axi_wready  (m_axi4_bus_i_cast.wready  ),
		.m_axi_bid     (m_axi4_bus_i_cast.bid     ),
		.m_axi_bresp   (m_axi4_bus_i_cast.bresp   ),
		.m_axi_buser   ('0                        ),
		.m_axi_bvalid  (m_axi4_bus_i_cast.bvalid  ),
		.m_axi_bready  (m_axi4_bus_o_cast.bready  ),
		.m_axi_arid    (m_axi4_bus_o_cast.arid    ),
		.m_axi_araddr  (m_axi4_bus_o_cast.araddr  ),
		.m_axi_arlen   (m_axi4_bus_o_cast.arlen   ),
		.m_axi_arsize  (m_axi4_bus_o_cast.arsize  ),
		.m_axi_arburst (m_axi4_bus_o_cast.arburst ),
		.m_axi_arlock  (m_axi4_bus_o_cast.arlock  ),
		.m_axi_arcache (m_axi4_bus_o_cast.arcache ),
		.m_axi_arprot  (m_axi4_bus_o_cast.arprot  ),
		.m_axi_arregion(m_axi4_bus_o_cast.arregion),
		.m_axi_arqos   (m_axi4_bus_o_cast.arqos   ),
		.m_axi_aruser  (                          ),
		.m_axi_arvalid (m_axi4_bus_o_cast.arvalid ),
		.m_axi_arready (m_axi4_bus_i_cast.arready ),
		.m_axi_rid     (m_axi4_bus_i_cast.rid     ),
		.m_axi_rdata   (m_axi4_bus_i_cast.rdata   ),
		.m_axi_rresp   (m_axi4_bus_i_cast.rresp   ),
		.m_axi_rlast   (m_axi4_bus_i_cast.rlast   ),
		.m_axi_ruser   ('0                        ),
		.m_axi_rvalid  (m_axi4_bus_i_cast.rvalid  ),
		.m_axi_rready  (m_axi4_bus_o_cast.rready  )
	);

endmodule
