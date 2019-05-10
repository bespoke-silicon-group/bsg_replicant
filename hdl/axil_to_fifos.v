/**
*  axil_to_fifos.v
*
*/

`include "bsg_axi_bus_pkg.vh"
`include "bsg_defines.v"
`include "axil_to_mcl.vh"

import cl_mcl_pkg::*;

module axil_to_fifos #(
  // manycore link paramters
  parameter num_fifo_pair_p = "inv"
  , parameter fifo_width_p = "inv"
  , parameter axil_base_addr_width_p = "inv"
  , parameter max_out_credits_p = "inv"
  , localparam mux_bus_num_lp = (num_fifo_pair_p+1)
  , localparam credits_width_lp = $clog2(max_out_credits_p+1)
  , localparam axil_mosi_bus_width_lp = `bsg_axil_mosi_bus_width(1)
  , localparam axil_miso_bus_width_lp = `bsg_axil_miso_bus_width(1)
  , localparam axil_mon_base_addr_lp  =  axil_mon_base_addr_p
  , localparam axil_s_fifo_base_addr_lp = axil_s_fifo_base_addr_p
  , localparam axil_m_fifo_base_addr_lp = axil_m_fifo_base_addr_p
  , localparam lg_rom_els_lp = `BSG_SAFE_CLOG2(rom_els_p)
) (
  input                                                 clk_i
  ,
  input                                                 reset_i
  // axil signals
  ,
  input  [axil_mosi_bus_width_lp-1:0]                   s_axil_bus_i
  ,
  output [axil_miso_bus_width_lp-1:0]                   s_axil_bus_o
  ,
  input  [      credits_width_lp-1:0]                   endpoint_credits_i
  // fifo signals
  ,
  input  [       num_fifo_pair_p-1:0]                   fifo_v_i
  ,
  input  [       num_fifo_pair_p-1:0][fifo_width_p-1:0] fifo_data_i
  ,
  output [       num_fifo_pair_p-1:0]                   fifo_rdy_o
  // to mcl slave
  ,
  output [       num_fifo_pair_p-1:0]                   fifo_v_o
  ,
  output [       num_fifo_pair_p-1:0][fifo_width_p-1:0] fifo_data_o
  ,
  input  [       num_fifo_pair_p-1:0]                   fifo_rdy_i
);

  // synopsys translate_off
  initial begin
    assert (mux_bus_num_lp <= 16)
      else begin
        $error("## axil mux master slot can not exceed 16!");
        $finish();
      end
    assert (fifo_width_p%8 == 0)
      else begin
        $error("## axis fifo width should be multiple of 8!");
        $finish();
      end
  end

  initial begin
    assert (lg_rom_els_lp <= axil_base_addr_width_p)
      else begin
        $error("## rom address width can not exceed axil_base_addr!");
        $finish();
      end
  end
  // synopsys translate_on

  `declare_bsg_axil_bus_s(1, bsg_axil_mosi_bus_s, bsg_axil_miso_bus_s);
  bsg_axil_mosi_bus_s s_axil_bus_i_cast;
  bsg_axil_miso_bus_s s_axil_bus_o_cast;

  assign s_axil_bus_i_cast = s_axil_bus_i;
  assign s_axil_bus_o      = s_axil_bus_o_cast;

  // ---------------------------------------------------------------------------------
  // AXIL crossbar
  // ---------------------------------------------------------------------------------
  `declare_bsg_axil_bus_s(mux_bus_num_lp, bsg_axil_mosi_demux_s, bsg_axil_miso_demux_s);
  bsg_axil_mosi_demux_s m_axil_demux_lo_cast;
  bsg_axil_miso_demux_s m_axil_demux_li_cast;

  localparam C_NUM_MASTER_SLOTS = mux_bus_num_lp;
  localparam C_M_AXI_BASE_ADDR = (mux_bus_num_lp*64)'({
      axil_mon_base_addr_lp, axil_s_fifo_base_addr_lp, axil_m_fifo_base_addr_lp
    });

  localparam C_M_AXI_ADDR_WIDTH = {C_NUM_MASTER_SLOTS{32'(axil_base_addr_width_p)}};

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
    .aclk          (clk_i                       ),
    .aresetn       (~reset_i                    ),
    .s_axi_awid    (1'H0                        ),
    .s_axi_awaddr  (s_axil_bus_i_cast.awaddr    ),
    .s_axi_awlen   (8'H00                       ),
    .s_axi_awsize  (3'H0                        ),
    .s_axi_awburst (2'H0                        ),
    .s_axi_awlock  (1'H0                        ),
    .s_axi_awcache (4'H0                        ),
    .s_axi_awprot  (3'H0                        ),
    .s_axi_awqos   (4'H0                        ),
    .s_axi_awuser  (1'H0                        ),
    .s_axi_awvalid (s_axil_bus_i_cast.awvalid   ),
    .s_axi_awready (s_axil_bus_o_cast.awready   ),
    .s_axi_wid     (1'H0                        ),
    .s_axi_wdata   (s_axil_bus_i_cast.wdata     ),
    .s_axi_wstrb   (s_axil_bus_i_cast.wstrb     ),
    .s_axi_wlast   (1'H1                        ),
    .s_axi_wuser   (1'H0                        ),
    .s_axi_wvalid  (s_axil_bus_i_cast.wvalid    ),
    .s_axi_wready  (s_axil_bus_o_cast.wready    ),
    .s_axi_bid     (                            ),
    .s_axi_bresp   (s_axil_bus_o_cast.bresp     ),
    .s_axi_buser   (                            ),
    .s_axi_bvalid  (s_axil_bus_o_cast.bvalid    ),
    .s_axi_bready  (s_axil_bus_i_cast.bready    ),
    .s_axi_arid    (1'H0                        ),
    .s_axi_araddr  (s_axil_bus_i_cast.araddr    ),
    .s_axi_arlen   (8'H00                       ),
    .s_axi_arsize  (3'H0                        ),
    .s_axi_arburst (2'H0                        ),
    .s_axi_arlock  (1'H0                        ),
    .s_axi_arcache (4'H0                        ),
    .s_axi_arprot  (3'H0                        ),
    .s_axi_arqos   (4'H0                        ),
    .s_axi_aruser  (1'H0                        ),
    .s_axi_arvalid (s_axil_bus_i_cast.arvalid   ),
    .s_axi_arready (s_axil_bus_o_cast.arready   ),
    .s_axi_rid     (                            ),
    .s_axi_rdata   (s_axil_bus_o_cast.rdata     ),
    .s_axi_rresp   (s_axil_bus_o_cast.rresp     ),
    .s_axi_rlast   (                            ),
    .s_axi_ruser   (                            ),
    .s_axi_rvalid  (s_axil_bus_o_cast.rvalid    ),
    .s_axi_rready  (s_axil_bus_i_cast.rready    ),
    .m_axi_awid    (                            ),
    .m_axi_awaddr  (m_axil_demux_lo_cast.awaddr ),
    .m_axi_awlen   (                            ),
    .m_axi_awsize  (                            ),
    .m_axi_awburst (                            ),
    .m_axi_awlock  (                            ),
    .m_axi_awcache (                            ),
    .m_axi_awprot  (                            ),
    .m_axi_awregion(                            ),
    .m_axi_awqos   (                            ),
    .m_axi_awuser  (                            ),
    .m_axi_awvalid (m_axil_demux_lo_cast.awvalid),
    .m_axi_awready (m_axil_demux_li_cast.awready),
    .m_axi_wid     (                            ),
    .m_axi_wdata   (m_axil_demux_lo_cast.wdata  ),
    .m_axi_wstrb   (m_axil_demux_lo_cast.wstrb  ),
    .m_axi_wlast   (                            ),
    .m_axi_wuser   (                            ),
    .m_axi_wvalid  (m_axil_demux_lo_cast.wvalid ),
    .m_axi_wready  (m_axil_demux_li_cast.wready ),
    .m_axi_bid     ({C_NUM_MASTER_SLOTS{1'b0}}  ),
    .m_axi_bresp   (m_axil_demux_li_cast.bresp  ),
    .m_axi_buser   ({C_NUM_MASTER_SLOTS{1'b0}}  ),
    .m_axi_bvalid  (m_axil_demux_li_cast.bvalid ),
    .m_axi_bready  (m_axil_demux_lo_cast.bready ),
    .m_axi_arid    (                            ),
    .m_axi_araddr  (m_axil_demux_lo_cast.araddr ),
    .m_axi_arlen   (                            ),
    .m_axi_arsize  (                            ),
    .m_axi_arburst (                            ),
    .m_axi_arlock  (                            ),
    .m_axi_arcache (                            ),
    .m_axi_arprot  (                            ),
    .m_axi_arregion(                            ),
    .m_axi_arqos   (                            ),
    .m_axi_aruser  (                            ),
    .m_axi_arvalid (m_axil_demux_lo_cast.arvalid),
    .m_axi_arready (m_axil_demux_li_cast.arready),
    .m_axi_rid     ({C_NUM_MASTER_SLOTS{1'b0}}  ),
    .m_axi_rdata   (m_axil_demux_li_cast.rdata  ),
    .m_axi_rresp   (m_axil_demux_li_cast.rresp  ),
    .m_axi_rlast   ({C_NUM_MASTER_SLOTS{1'b1}}  ),
    .m_axi_ruser   ({C_NUM_MASTER_SLOTS{1'b0}}  ),
    .m_axi_rvalid  (m_axil_demux_li_cast.rvalid ),
    .m_axi_rready  (m_axil_demux_lo_cast.rready )
  );

  // master axil interface un-casting
  logic [mux_bus_num_lp-1:0][axil_mosi_bus_width_lp-1:0] axil_mosi_demux_bus;
  logic [mux_bus_num_lp-1:0][axil_miso_bus_width_lp-1:0] axil_miso_demux_bus;

  for (genvar i=0; i<mux_bus_num_lp; i=i+1) begin
    always_comb begin: axil_bus_assignment
      axil_mosi_demux_bus[i] = {
        m_axil_demux_lo_cast.awaddr[32*i+:32]
        ,m_axil_demux_lo_cast.awvalid[i]
        ,m_axil_demux_lo_cast.wdata[32*i+:32]
        ,m_axil_demux_lo_cast.wstrb[4*i+:4]
        ,m_axil_demux_lo_cast.wvalid[i]
        ,m_axil_demux_lo_cast.bready[i]
        ,m_axil_demux_lo_cast.araddr[32*i+:32]
        ,m_axil_demux_lo_cast.arvalid[i]
        ,m_axil_demux_lo_cast.rready[i]
      };

      {
        m_axil_demux_li_cast.awready[i]
        ,m_axil_demux_li_cast.wready[i]
        ,m_axil_demux_li_cast.bresp[2*i+:2]
        ,m_axil_demux_li_cast.bvalid[i]
        ,m_axil_demux_li_cast.arready[i]
        ,m_axil_demux_li_cast.rdata[32*i+:32]
        ,m_axil_demux_li_cast.rresp[2*i+:2]
        ,m_axil_demux_li_cast.rvalid[i]
      } = axil_miso_demux_bus[i];
    end
  end

  wire [num_fifo_pair_p-1:0][credits_width_lp-1:0] rcv_fifo_vacancy_lo;

  for (genvar i=0; i<num_fifo_pair_p; i=i+1) begin
    axil_to_axis #(
      .mcl_width_p      (fifo_width_p     ),
      .max_out_credits_p(max_out_credits_p)
    ) s_axil_to_fifos (
      .clk_i           (clk_i                 ),
      .reset_i         (reset_i               ),
      .s_axil_mcl_bus_i(axil_mosi_demux_bus[i]),
      .s_axil_mcl_bus_o(axil_miso_demux_bus[i]),
      .mcl_v_i         (fifo_v_i[i]           ),
      .mcl_data_i      (fifo_data_i[i]        ),
      .mcl_r_o         (fifo_rdy_o[i]       ),
      .mcl_v_o         (fifo_v_o[i]           ),
      .mcl_data_o      (fifo_data_o[i]        ),
      .mcl_r_i         (fifo_rdy_i[i]       ),
      .rcv_vacancy_o   (rcv_fifo_vacancy_lo[i])
    );
  end

  logic [axil_base_addr_width_p-1:0] axil_mem_addr_lo;
  logic [                      31:0] axil_mem_data_li;

  axil_to_mem #(
    .mem_addr_width_p      (axil_base_addr_width_p),
    .axil_base_addr_p      (axil_mon_base_addr_lp ),
    .axil_base_addr_width_p(axil_base_addr_width_p)
  ) axi_mem_rw (
    .clk_i       (clk_i                               ),
    .reset_i     (reset_i                             ),
    .s_axil_bus_i(axil_mosi_demux_bus[num_fifo_pair_p]),
    .s_axil_bus_o(axil_miso_demux_bus[num_fifo_pair_p]),
    .addr_o      (axil_mem_addr_lo                    ),
    .wen_o       (                                    ),
    .data_o      (                                    ),
    .ren_o       (                                    ),
    .data_i      (axil_mem_data_li                    ),
    .done        (                                    )
  );


  logic [31:0] rom_data_lo;
  bsg_bladerunner_configuration #(
    .width_p     (rom_width_p  ),
    .addr_width_p(lg_rom_els_lp)
  ) configuration_rom (
    .addr_i(axil_mem_addr_lo[2+:lg_rom_els_lp]),
    .data_o(rom_data_lo                       )
  );

  wire is_accessing_rcv_req_fifo     = (axil_mem_addr_lo==axil_base_addr_width_p'(HOST_RCV_VACANCY_MC_REQ));
  wire is_accessing_rcv_res_fifo     = (axil_mem_addr_lo==axil_base_addr_width_p'(HOST_RCV_VACANCY_MC_RES));
  wire is_accessing_host_req_credits = (axil_mem_addr_lo==axil_base_addr_width_p'(HOST_REQ_CREDITS))       ;

  always_comb begin
    if (is_accessing_rcv_res_fifo) begin
      axil_mem_data_li = (32)'(rcv_fifo_vacancy_lo[0]);
    end
    else if (is_accessing_rcv_req_fifo) begin
      axil_mem_data_li = (32)'(rcv_fifo_vacancy_lo[1]);
    end
    else if (is_accessing_host_req_credits) begin
      axil_mem_data_li = (32)'(endpoint_credits_i);
    end
    else begin
      axil_mem_data_li = (32)'(rom_data_lo);
    end
  end

endmodule
