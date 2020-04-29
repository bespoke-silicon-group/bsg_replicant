/*
 *  axil_to_mcl.v
 *  
 *  This module converts the AXIL memory-mapped interface to the manycore network interface.
 *  It also reads from the ROM in the AXIL addres space. 
 *  TODO: factor a single link out using basejump_stl.
 *          ______________     _______________                 __________
 *         | axi crossbar |==>|axi mcl adapter|--> fifo_o --> | manycore |
 *  AXIL==>|              |   |___to device___|<-- fifo_i <-- | endpoint | --> link_sif_o
 *         |              |    _______________                |          | <-- link_sif_i
 *         |              |==>|axi mcl adapter|--> fifo_o --> |          |
 *         |              |   |___to host_____|<-- fifo_i <-- |__________|
 *         |              |    ...............
 *         |______________|==>|F1 config rom  |
 *                             ```````````````
 */

`include "bsg_axi_bus_pkg.vh"
`include "bsg_defines.v"
`include "axil_to_mcl.vh"

module axil_to_mcl
  import bsg_manycore_pkg::*;
  import cl_mcl_pkg::*;
  import bsg_bladerunner_pkg::*;
  #(
  // manycore link paramters
  num_mcl_p="inv"
  , axil_bus_num_lp=num_mcl_p*2
  , fifo_width_lp=128
  // manycore paramters
  , num_tiles_x_p="inv"
  , num_tiles_y_p="inv"
  , x_cord_width_p="inv"
  , y_cord_width_p="inv"
  , fifo_els_p=4
  , addr_width_p="inv"
  , data_width_p="inv"
  , max_out_credits_p="inv"
  , rcv_fifo_els_lp=256
  , link_sif_width_lp=`bsg_manycore_link_sif_width(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p)
  , packet_width_lp=`bsg_manycore_packet_width(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p)
  , return_packet_width_lp=`bsg_manycore_return_packet_width(x_cord_width_p,y_cord_width_p,data_width_p)
) (input                                         clk_i
  , input                                         reset_i
  // axil signals
  , input                                         s_axil_mcl_awvalid
  , input  [                                31:0] s_axil_mcl_awaddr
  , output                                        s_axil_mcl_awready
  , input                                         s_axil_mcl_wvalid
  , input  [                                31:0] s_axil_mcl_wdata
  , input  [                                 3:0] s_axil_mcl_wstrb
  , output                                        s_axil_mcl_wready
  , output [                                 1:0] s_axil_mcl_bresp
  , output                                        s_axil_mcl_bvalid
  , input                                         s_axil_mcl_bready
  , input  [                                31:0] s_axil_mcl_araddr
  , input                                         s_axil_mcl_arvalid
  , output                                        s_axil_mcl_arready
  , output [                                31:0] s_axil_mcl_rdata
  , output [                                 1:0] s_axil_mcl_rresp
  , output                                        s_axil_mcl_rvalid
  , input                                         s_axil_mcl_rready
  // manycore links
  , input  [link_sif_width_lp*num_mcl_p-1:0] link_sif_i
  , output [link_sif_width_lp*num_mcl_p-1:0] link_sif_o
  , input  [   x_cord_width_p*num_mcl_p-1:0] my_x_i
  , input  [   y_cord_width_p*num_mcl_p-1:0] my_y_i

  // print stat
  , output logic [num_mcl_p-1:0] print_stat_v_o
  , output logic [num_mcl_p-1:0][data_width_p-1:0] print_stat_tag_o
);

  localparam axil_mosi_bus_width_lp = `bsg_axil_mosi_bus_width(1);
  localparam axil_miso_bus_width_lp = `bsg_axil_miso_bus_width(1);


  localparam mux_bus_num_lp = (axil_bus_num_lp+1);
  localparam axil_base_addr_width_lp = 12;

  localparam base_address_list_lp = (mux_bus_num_lp*64)'({
    axil_mon_base_addr_p
    ,axil_s_fifo_base_addr_p
    ,axil_m_fifo_base_addr_p
    });

  localparam C_NUM_MASTER_SLOTS = mux_bus_num_lp;
  localparam C_M_AXI_BASE_ADDR  = base_address_list_lp;
  localparam C_M_AXI_ADDR_WIDTH         = {C_NUM_MASTER_SLOTS{32'(axil_base_addr_width_lp)}};
  localparam C_M_AXI_WRITE_CONNECTIVITY = {C_NUM_MASTER_SLOTS{32'h0000_0001}};
  localparam C_M_AXI_READ_CONNECTIVITY  = {C_NUM_MASTER_SLOTS{32'h0000_0001}};
  localparam C_M_AXI_WRITE_ISSUING      = {C_NUM_MASTER_SLOTS{32'h0000_0001}};
  localparam C_M_AXI_READ_ISSUING       = {C_NUM_MASTER_SLOTS{32'h0000_0001}};
  localparam C_M_AXI_SECURE             = {C_NUM_MASTER_SLOTS{32'h0000_0000}};

  localparam axil_mon_base_addr_lp = base_address_list_lp[64*axil_bus_num_lp+:64];

  `declare_bsg_axil_bus_s(1, bsg_axil_mosi_bus_s, bsg_axil_miso_bus_s);
  bsg_axil_mosi_bus_s s0_axil_bus_i_cast;
  bsg_axil_miso_bus_s s0_axil_bus_o_cast;

  assign s0_axil_bus_i_cast.awvalid = s_axil_mcl_awvalid;
  assign s0_axil_bus_i_cast.awaddr  = s_axil_mcl_awaddr;
  assign s_axil_mcl_awready         = s0_axil_bus_o_cast.awready;

  assign s0_axil_bus_i_cast.wvalid = s_axil_mcl_wvalid;
  assign s0_axil_bus_i_cast.wdata  = s_axil_mcl_wdata;
  assign s0_axil_bus_i_cast.wstrb  = s_axil_mcl_wstrb;
  assign s_axil_mcl_wready         = s0_axil_bus_o_cast.wready;

  assign s_axil_mcl_bresp          = s0_axil_bus_o_cast.bresp;
  assign s_axil_mcl_bvalid         = s0_axil_bus_o_cast.bvalid;
  assign s0_axil_bus_i_cast.bready = s_axil_mcl_bready;

  assign s0_axil_bus_i_cast.araddr  = s_axil_mcl_araddr;
  assign s0_axil_bus_i_cast.arvalid = s_axil_mcl_arvalid;
  assign s_axil_mcl_arready         = s0_axil_bus_o_cast.arready;

  assign s_axil_mcl_rdata          = s0_axil_bus_o_cast.rdata;
  assign s_axil_mcl_rresp          = s0_axil_bus_o_cast.rresp;
  assign s_axil_mcl_rvalid         = s0_axil_bus_o_cast.rvalid;
  assign s0_axil_bus_i_cast.rready = s_axil_mcl_rready;


  `declare_bsg_axil_bus_s(mux_bus_num_lp, bsg_axil_mosi_busN_s, bsg_axil_miso_busN_s);
  bsg_axil_mosi_busN_s axil_mosi_busN;
  bsg_axil_miso_busN_s axil_miso_busN;

  // synopsys translate_off
   always_ff @(negedge clk_i)
   begin
         assert (mux_bus_num_lp <= 16)
         else begin
                 $error("## axil mux master slot can not exceed 16!");
                 $finish();
         end
   end
  // synopsys translate_on

  axi_crossbar_v2_1_20_axi_crossbar #(
    .C_FAMILY                   ("virtexuplus"             )
    ,.C_NUM_SLAVE_SLOTS          (1                         )
    ,.C_NUM_MASTER_SLOTS         (C_NUM_MASTER_SLOTS        )
    ,.C_AXI_ID_WIDTH             (1                         )
    ,.C_AXI_ADDR_WIDTH           (32                        )
    ,.C_AXI_DATA_WIDTH           (32                        )
    ,.C_AXI_PROTOCOL             (2                         )
    ,.C_NUM_ADDR_RANGES          (1                         )
    ,.C_M_AXI_BASE_ADDR          (C_M_AXI_BASE_ADDR         )
    ,.C_M_AXI_ADDR_WIDTH         (C_M_AXI_ADDR_WIDTH        )
    ,.C_S_AXI_BASE_ID            (32'H00000000              )
    ,.C_S_AXI_THREAD_ID_WIDTH    (32'H00000000              )
    ,.C_AXI_SUPPORTS_USER_SIGNALS(0                         )
    ,.C_AXI_AWUSER_WIDTH         (1                         )
    ,.C_AXI_ARUSER_WIDTH         (1                         )
    ,.C_AXI_WUSER_WIDTH          (1                         )
    ,.C_AXI_RUSER_WIDTH          (1                         )
    ,.C_AXI_BUSER_WIDTH          (1                         )
    ,.C_M_AXI_WRITE_CONNECTIVITY (C_M_AXI_WRITE_CONNECTIVITY)
    ,.C_M_AXI_READ_CONNECTIVITY  (C_M_AXI_READ_CONNECTIVITY )
    ,.C_R_REGISTER               (1                         )
    ,.C_S_AXI_SINGLE_THREAD      (32'H00000001              )
    ,.C_S_AXI_WRITE_ACCEPTANCE   (32'H00000001              )
    ,.C_S_AXI_READ_ACCEPTANCE    (32'H00000001              )
    ,.C_M_AXI_WRITE_ISSUING      (C_M_AXI_WRITE_ISSUING     )
    ,.C_M_AXI_READ_ISSUING       (C_M_AXI_READ_ISSUING      )
    ,.C_S_AXI_ARB_PRIORITY       (32'H00000000              )
    ,.C_M_AXI_SECURE             (C_M_AXI_SECURE            )
    ,.C_CONNECTIVITY_MODE        (0                         )
  ) axil_crossbar_mux (
    .aclk          (clk_i                     )
    ,.aresetn       (~reset_i                  )
    ,.s_axi_awid    (1'H0                      )
    ,.s_axi_awaddr  (s0_axil_bus_i_cast.awaddr )
    ,.s_axi_awlen   (8'H00                     )
    ,.s_axi_awsize  (3'H0                      )
    ,.s_axi_awburst (2'H0                      )
    ,.s_axi_awlock  (1'H0                      )
    ,.s_axi_awcache (4'H0                      )
    ,.s_axi_awprot  (3'H0                      )
    ,.s_axi_awqos   (4'H0                      )
    ,.s_axi_awuser  (1'H0                      )
    ,.s_axi_awvalid (s0_axil_bus_i_cast.awvalid)
    ,.s_axi_awready (s0_axil_bus_o_cast.awready)
    ,.s_axi_wid     (1'H0                      )
    ,.s_axi_wdata   (s0_axil_bus_i_cast.wdata  )
    ,.s_axi_wstrb   (s0_axil_bus_i_cast.wstrb  )
    ,.s_axi_wlast   (1'H1                      )
    ,.s_axi_wuser   (1'H0                      )
    ,.s_axi_wvalid  (s0_axil_bus_i_cast.wvalid )
    ,.s_axi_wready  (s0_axil_bus_o_cast.wready )
    ,.s_axi_bid     (                          )
    ,.s_axi_bresp   (s0_axil_bus_o_cast.bresp  )
    ,.s_axi_buser   (                          )
    ,.s_axi_bvalid  (s0_axil_bus_o_cast.bvalid )
    ,.s_axi_bready  (s0_axil_bus_i_cast.bready )
    ,.s_axi_arid    (1'H0                      )
    ,.s_axi_araddr  (s0_axil_bus_i_cast.araddr )
    ,.s_axi_arlen   (8'H00                     )
    ,.s_axi_arsize  (3'H0                      )
    ,.s_axi_arburst (2'H0                      )
    ,.s_axi_arlock  (1'H0                      )
    ,.s_axi_arcache (4'H0                      )
    ,.s_axi_arprot  (3'H0                      )
    ,.s_axi_arqos   (4'H0                      )
    ,.s_axi_aruser  (1'H0                      )
    ,.s_axi_arvalid (s0_axil_bus_i_cast.arvalid)
    ,.s_axi_arready (s0_axil_bus_o_cast.arready)
    ,.s_axi_rid     (                          )
    ,.s_axi_rdata   (s0_axil_bus_o_cast.rdata  )
    ,.s_axi_rresp   (s0_axil_bus_o_cast.rresp  )
    ,.s_axi_rlast   (                          )
    ,.s_axi_ruser   (                          )
    ,.s_axi_rvalid  (s0_axil_bus_o_cast.rvalid )
    ,.s_axi_rready  (s0_axil_bus_i_cast.rready )
    ,.m_axi_awid    (                          )
    ,.m_axi_awaddr  (axil_mosi_busN.awaddr     )
    ,.m_axi_awlen   (                          )
    ,.m_axi_awsize  (                          )
    ,.m_axi_awburst (                          )
    ,.m_axi_awlock  (                          )
    ,.m_axi_awcache (                          )
    ,.m_axi_awprot  (                          )
    ,.m_axi_awregion(                          )
    ,.m_axi_awqos   (                          )
    ,.m_axi_awuser  (                          )
    ,.m_axi_awvalid (axil_mosi_busN.awvalid    )
    ,.m_axi_awready (axil_miso_busN.awready    )
    ,.m_axi_wid     (                          )
    ,.m_axi_wdata   (axil_mosi_busN.wdata      )
    ,.m_axi_wstrb   (axil_mosi_busN.wstrb      )
    ,.m_axi_wlast   (                          )
    ,.m_axi_wuser   (                          )
    ,.m_axi_wvalid  (axil_mosi_busN.wvalid     )
    ,.m_axi_wready  (axil_miso_busN.wready     )
    ,.m_axi_bid     ({C_NUM_MASTER_SLOTS{1'b0}})
    ,.m_axi_bresp   (axil_miso_busN.bresp      )
    ,.m_axi_buser   ({C_NUM_MASTER_SLOTS{1'b0}})
    ,.m_axi_bvalid  (axil_miso_busN.bvalid     )
    ,.m_axi_bready  (axil_mosi_busN.bready     )
    ,.m_axi_arid    (                          )
    ,.m_axi_araddr  (axil_mosi_busN.araddr     )
    ,.m_axi_arlen   (                          )
    ,.m_axi_arsize  (                          )
    ,.m_axi_arburst (                          )
    ,.m_axi_arlock  (                          )
    ,.m_axi_arcache (                          )
    ,.m_axi_arprot  (                          )
    ,.m_axi_arregion(                          )
    ,.m_axi_arqos   (                          )
    ,.m_axi_aruser  (                          )
    ,.m_axi_arvalid (axil_mosi_busN.arvalid    )
    ,.m_axi_arready (axil_miso_busN.arready    )
    ,.m_axi_rid     ({C_NUM_MASTER_SLOTS{1'b0}})
    ,.m_axi_rdata   (axil_miso_busN.rdata      )
    ,.m_axi_rresp   (axil_miso_busN.rresp      )
    ,.m_axi_rlast   ({C_NUM_MASTER_SLOTS{1'b1}})
    ,.m_axi_ruser   ({C_NUM_MASTER_SLOTS{1'b0}})
    ,.m_axi_rvalid  (axil_miso_busN.rvalid     )
    ,.m_axi_rready  (axil_mosi_busN.rready     )
  );


  logic [mux_bus_num_lp-1:0][axil_mosi_bus_width_lp-1:0] axil_demux_mosi_bus;
  logic [mux_bus_num_lp-1:0][axil_miso_bus_width_lp-1:0] axil_demux_miso_bus;

  logic [axil_bus_num_lp-1:0][$clog2(rcv_fifo_els_lp+1)-1:0] rcv_fifo_vacancy_lo;

  for (genvar i=0; i<mux_bus_num_lp; i=i+1) begin
    always_comb begin: axil_bus_assignment
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
  logic [axil_bus_num_lp-1:0]                    fifo_v_li;
  logic [axil_bus_num_lp-1:0][fifo_width_lp-1:0] fifo_data_li;
  logic [axil_bus_num_lp-1:0]                    fifo_ready_lo;

  // to mcl slave
  logic [axil_bus_num_lp-1:0]                    fifo_v_lo;
  logic [axil_bus_num_lp-1:0][fifo_width_lp-1:0] fifo_data_lo;
  logic [axil_bus_num_lp-1:0]                    fifo_ready_li;

  for (genvar i=0; i<axil_bus_num_lp; i=i+1) begin
    s_axil_mcl_adapter #(
      .mcl_width_p      (fifo_width_lp    )
      ,.max_out_credits_p(rcv_fifo_els_lp)
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


  logic [num_mcl_p-1:0]                        endpoint_in_v_lo;
  logic [num_mcl_p-1:0]                        endpoint_in_yumi_li;
  logic [num_mcl_p-1:0][     data_width_p-1:0] endpoint_in_data_lo;
  logic [num_mcl_p-1:0][(data_width_p>>3)-1:0] endpoint_in_mask_lo;
  logic [num_mcl_p-1:0][     addr_width_p-1:0] endpoint_in_addr_lo;
  logic [num_mcl_p-1:0]                        endpoint_in_we_lo;
  logic [num_mcl_p-1:0][   x_cord_width_p-1:0] in_src_x_cord_lo;
  logic [num_mcl_p-1:0][   y_cord_width_p-1:0] in_src_y_cord_lo;

  `declare_bsg_manycore_packet_s(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p);

  logic [num_mcl_p-1:0]                      endpoint_out_v_li;
  bsg_manycore_packet_s [num_mcl_p-1:0]      endpoint_out_packet_li;
  logic [num_mcl_p-1:0]                      endpoint_out_ready_lo;

  bsg_manycore_return_packet_type_e
    [num_mcl_p-1:0] returned_pkt_type_r_lo;

  logic [num_mcl_p-1:0][bsg_manycore_reg_id_width_gp-1:0] returned_reg_id_r_lo;
  logic [num_mcl_p-1:0][   data_width_p-1:0]              returned_data_r_lo;
  logic [num_mcl_p-1:0]                                   returned_v_r_lo;
  logic [num_mcl_p-1:0]                                   returned_fifo_full_lo;
  logic [num_mcl_p-1:0]                                   returned_yumi_li;

  logic [num_mcl_p-1:0][data_width_p-1:0]    returning_data_li;
  logic [num_mcl_p-1:0]                      returning_v_li;

  logic [num_mcl_p-1:0][$clog2(max_out_credits_p+1)-1:0] out_credits_lo;

  logic [num_mcl_p-1:0][x_cord_width_p-1:0] my_x_li;
  logic [num_mcl_p-1:0][y_cord_width_p-1:0] my_y_li;

  assign my_x_li = my_x_i;
  assign my_y_li = my_y_i;

  logic [num_mcl_p-1:0] returning_wr_v_r;
  always_ff @(posedge clk_i)
  begin
        if(reset_i)
                returning_wr_v_r <= '0;
        else
                returning_wr_v_r <= endpoint_in_yumi_li & endpoint_in_we_lo;
  end

  `declare_bsg_mcl_request_s;
  `declare_bsg_mcl_response_s;
  bsg_mcl_request_s [num_mcl_p-1:0] fifo_req_cast;
  bsg_mcl_request_s [num_mcl_p-1:0] mc_req_cast;
  bsg_mcl_response_s [num_mcl_p-1:0] fifo_res_cast;
  bsg_mcl_response_s [num_mcl_p-1:0] mc_res_cast;
 
  // disable request to the manycore if:
  // 1. manycore endpoint out credits == 0
  // 2. host issues load request and the rcv fifo vacancy < max_out_credits_p (do not use 0 because of the network latency)
  logic [num_mcl_p-1:0] fifo_req_enable;

  for (genvar i=0; i<num_mcl_p; i=i+1) begin

    // fifo request to manycore
    assign fifo_req_enable[i] = !(
      (fifo_req_cast[i].op==8'(e_remote_load)) && (rcv_fifo_vacancy_lo[2*i]<max_out_credits_p)
      || (out_credits_lo[i] == '0)
    );
    assign endpoint_out_v_li[i] = fifo_v_lo[2*i] & fifo_req_enable[i];
    assign fifo_ready_li[2*i] = endpoint_out_ready_lo[i] & fifo_req_enable[i];
    assign fifo_req_cast[i]          = fifo_data_lo[2*i];

    bsg_manycore_packet_op_e endpoint_out_packet_op;
    assign endpoint_out_packet_op = bsg_manycore_packet_op_e'(fifo_req_cast[i].op);
    assign endpoint_out_packet_li[i].addr = (addr_width_p)'(fifo_req_cast[i].addr);
    assign endpoint_out_packet_li[i].op = endpoint_out_packet_op;
    assign endpoint_out_packet_li[i].op_ex = ($bits(bsg_manycore_packet_op_ex_u))'(fifo_req_cast[i].op_ex);
    assign endpoint_out_packet_li[i].src_y_cord = (y_cord_width_p)'(fifo_req_cast[i].src_y_cord);
    assign endpoint_out_packet_li[i].src_x_cord = (x_cord_width_p)'(fifo_req_cast[i].src_x_cord);
    assign endpoint_out_packet_li[i].y_cord = (y_cord_width_p)'(fifo_req_cast[i].y_cord);
    assign endpoint_out_packet_li[i].x_cord = (x_cord_width_p)'(fifo_req_cast[i].x_cord);

    always_comb begin
      if (endpoint_out_packet_op == e_remote_load) begin
        endpoint_out_packet_li[i].reg_id = fifo_req_cast[i].payload.load_id;
      end
      else begin
        endpoint_out_packet_li[i].reg_id = '0;
      end
    end
    always_comb begin
      if (endpoint_out_packet_op == e_remote_store) begin
        endpoint_out_packet_li[i].payload.data = fifo_req_cast[i].payload.data;
      end
      else begin
        endpoint_out_packet_li[i].payload.load_info_s.load_info.float_wb       = 1'b0;
        endpoint_out_packet_li[i].payload.load_info_s.load_info.icache_fetch   = 1'b0;
        endpoint_out_packet_li[i].payload.load_info_s.load_info.part_sel       = 4'b1111;
        endpoint_out_packet_li[i].payload.load_info_s.load_info.is_unsigned_op = 1'b1;
        endpoint_out_packet_li[i].payload.load_info_s.load_info.is_byte_op     = 1'b0;
        endpoint_out_packet_li[i].payload.load_info_s.load_info.is_hex_op      = 1'b0;
      end
    end

    always_ff @(negedge clk_i) begin
      assert(reset_i | !endpoint_out_v_li[i] | (endpoint_out_packet_op != e_remote_amo)) else
        $error("[BSG_ERROR][axil_to_mcl.v] remote atomic memory operations from the host are not supported");
    end

    // manycore response to fifo
    assign fifo_v_li[2*i]      = returned_v_r_lo[i];
    assign fifo_data_li[2*i]   = mc_res_cast[i];
    assign mc_res_cast[i].padding = '0;
    assign mc_res_cast[i].pkt_type = {6'b0, returned_pkt_type_r_lo[i]};
    assign mc_res_cast[i].data = (32)'(returned_data_r_lo[i]);
    assign mc_res_cast[i].load_id = (32)'(returned_reg_id_r_lo[i]);
    assign mc_res_cast[i].y_cord = (8)'(my_y_li[i]);
    assign mc_res_cast[i].x_cord = (8)'(my_x_li[i]);
    assign returned_yumi_li[i] = fifo_ready_lo[2*i] & fifo_v_li[2*i];  // must be 1 if fifo_v_li is 1

    // manycore request to fifo
    assign fifo_v_li[2*i+1]       = endpoint_in_v_lo[i];
    assign endpoint_in_yumi_li[i] = fifo_ready_lo[2*i+1] & fifo_v_li[2*i+1]; // host need to check the rcv vacancy 
    assign fifo_data_li[2*i+1]    = mc_req_cast[i];
    assign mc_req_cast[i].padding = '0;
    assign mc_req_cast[i].addr = (32)'(endpoint_in_addr_lo[i]);
    assign mc_req_cast[i].op = (8)'({1'b0, endpoint_in_we_lo[i]});
    assign mc_req_cast[i].op_ex = (8)'(endpoint_in_mask_lo[i]);
    assign mc_req_cast[i].payload.data = (32)'(endpoint_in_data_lo[i]);
    assign mc_req_cast[i].src_y_cord = (8)'(in_src_y_cord_lo[i]);
    assign mc_req_cast[i].src_x_cord = (8)'(in_src_x_cord_lo[i]);
    assign mc_req_cast[i].y_cord = (8)'(my_y_li[i]);
    assign mc_req_cast[i].x_cord = (8)'(my_x_li[i]);

    // fifo response to manycore
    assign fifo_ready_li[2*i+1] = ~returning_wr_v_r[i];
    assign fifo_res_cast[i]     = fifo_data_lo[2*i+1];
    assign returning_data_li[i] = returning_wr_v_r[i] ? '0 : (data_width_p)'(fifo_res_cast[i].data);

    assign returning_v_li[i] = returning_wr_v_r[i] | (fifo_v_lo[2*i+1] & fifo_ready_li[2*i+1]);

  end


  for (genvar i=0; i<num_mcl_p; i=i+1) begin
    bsg_manycore_endpoint_standard #(
      .x_cord_width_p   (x_cord_width_p   )
      ,.y_cord_width_p   (y_cord_width_p   )
      ,.fifo_els_p       (fifo_els_p       )
      ,.addr_width_p     (addr_width_p     )
      ,.data_width_p     (data_width_p     )
      ,.max_out_credits_p(max_out_credits_p)
    ) mcl_endpoint_standard (
      .clk_i                  (clk_i                    )
      ,.reset_i               (reset_i                  )

      ,.link_sif_i            (link_sif_i               )
      ,.link_sif_o            (link_sif_o               )

      // manycore packet -> fifo
      ,.in_v_o                (endpoint_in_v_lo[i]      )
      ,.in_data_o             (endpoint_in_data_lo[i]   )
      ,.in_mask_o             (endpoint_in_mask_lo[i]   )
      ,.in_addr_o             (endpoint_in_addr_lo[i]   )
      ,.in_we_o               (endpoint_in_we_lo[i]     )
      ,.in_load_info_o        (                         ) // not used since remote load is not supported on host interface.
      ,.in_src_x_cord_o       (in_src_x_cord_lo[i]      )
      ,.in_src_y_cord_o       (in_src_y_cord_lo[i]      )
      ,.in_yumi_i             (endpoint_in_yumi_li[i]   )

      // fifo -> manycore packet
      ,.out_v_i               (endpoint_out_v_li[i]     )
      ,.out_packet_i          (endpoint_out_packet_li[i])
      ,.out_ready_o           (endpoint_out_ready_lo[i] )

      // manycore credit -> fifo
      ,.returned_data_r_o     (returned_data_r_lo[i]    )
      ,.returned_reg_id_r_o   (returned_reg_id_r_lo[i]  )
      ,.returned_v_r_o        (returned_v_r_lo[i]       )
      ,.returned_pkt_type_r_o (returned_pkt_type_r_lo[i])
      ,.returned_fifo_full_o  (returned_fifo_full_lo[i] )
       // always 1'b1 if returned_fifo_p is not set
      ,.returned_yumi_i       (returned_yumi_li[i]      )

      // fifo -> manycore credit
      ,.returning_data_i      (returning_data_li[i]     )
      ,.returning_v_i         (returning_v_li[i]        )

      ,.out_credits_o         (out_credits_lo[i]        )
      ,.my_x_i                (my_x_li[i]               )
      ,.my_y_i                (my_y_li[i]               )
    );
  end


  for (genvar k = 0; k < num_mcl_p; k++) begin
    assign print_stat_v_o[k] = endpoint_in_v_lo[k] & endpoint_in_we_lo[k]
      & ({endpoint_in_addr_lo[k][13:0], 2'b00} == 16'h0D0C);
    assign print_stat_tag_o[k] = endpoint_in_data_lo[k];
  end


  logic [axil_base_addr_width_lp-1:0] axil_mem_addr_lo;
  logic [31:0] axil_mem_data_li;

  axil_to_mem #(
    .mem_addr_width_p(axil_base_addr_width_lp)
    ,.axil_base_addr_p(axil_mon_base_addr_lp)
    ,.axil_base_addr_width_p(axil_base_addr_width_lp)
  ) axi_mem_rw (
    .clk_i       (clk_i)
    ,.reset_i     (reset_i)
    ,.s_axil_bus_i(axil_demux_mosi_bus[axil_bus_num_lp])
    ,.s_axil_bus_o(axil_demux_miso_bus[axil_bus_num_lp])
    ,.addr_o      (axil_mem_addr_lo)
    ,.wen_o       ()
    ,.data_o      ()
    ,.ren_o       ()
    ,.data_i      (axil_mem_data_li)
    ,.done        ()
  );


  localparam lg_rom_els_lp = `BSG_SAFE_CLOG2(rom_els_p);
  // synopsys translate_off
  initial begin
    assert (lg_rom_els_lp <= axil_base_addr_width_lp)
      else begin
        $error("## rom address width can not exceed axil_base_addr!");
        $finish();
      end
  end
  // synopsys translate_on

  logic [31:0] rom_data_lo;
  bsg_bladerunner_configuration #(
    .width_p     (rom_width_p  ),
    .addr_width_p(lg_rom_els_lp)
  ) configuration_rom (
    .addr_i(axil_mem_addr_lo[2+:lg_rom_els_lp]),
    .data_o(rom_data_lo                        )
  );

  wire is_accessing_rcv_res_fifo = (axil_mem_addr_lo==axil_base_addr_width_lp'(HOST_RCV_VACANCY_MC_RES));
  wire is_accessing_rcv_req_fifo = (axil_mem_addr_lo==axil_base_addr_width_lp'(HOST_RCV_VACANCY_MC_REQ));
  wire is_accessing_host_req_credits = (axil_mem_addr_lo==axil_base_addr_width_lp'(HOST_REQ_CREDITS));

  always_comb begin
    if (is_accessing_rcv_res_fifo) begin
      axil_mem_data_li = (32)'(rcv_fifo_vacancy_lo[0]);
    end
    else if (is_accessing_rcv_req_fifo) begin
      axil_mem_data_li = (32)'(rcv_fifo_vacancy_lo[1]);
    end
    else if (is_accessing_host_req_credits) begin
      axil_mem_data_li = (32)'(out_credits_lo[0]);
    end
    else begin
      axil_mem_data_li = (32)'(rom_data_lo);
    end
  end

endmodule
