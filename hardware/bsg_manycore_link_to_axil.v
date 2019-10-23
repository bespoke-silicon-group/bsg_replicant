/*
*  bsg_manycore_link_to_axil.v
*
*  AXIL memory-mapped (HOST side)   <->   manycore endpoint standard (MC side)
*       _________      ______________                    __________
*  ==> | axil    | -> |  ser_i_par_o | ---------------> | manycore |
*      | to      | -> |______X2______| ---------------> | endpoint |  --> link_sif_o
*      | fifos   |     ______________      ________     | to fifos |
*      |         | <- |  par_i_ser_o | <- |rcv fifo| <- |          |  <-- link_sif_i
*      |         | <- |______X2______| <-/|___X2___| <- |          |
*      |         |                       ||        |    |          |
*      |         | <--------------------=]]        [--> |          |
*      |_________| <----------------------------------- |out credit|
*           ^                                           |__________|
*       ____|____
*      | cfg rom |
*      |_________| (bladerunner-related hardware configurations)
*
* Note:
* 1. Only num_endpoints_gp = 1 is tested, see reference 2 for detailed block diagram
* 2. The memory allocation for AXI address space is defined at bsg_manycore_link_to_axil_pkg.v
* 3. Because the piso module has conversion latency, we use the rcv fifo (as the 1st rcv_fifos shown above) to ensure
*    that the fifo always accepts the returned packets from manycore, which is the rule of attaching Master Modules
*    in the BaseJump Manycore Accelerator Network. (see reference 1 below)
*
* [1] https://docs.google.com/document/d/1-i62N72pfx2Cd_xKT3hiTuSilQnuC0ZOaSQMG8UPkto
* [2] https://docs.google.com/presentation/d/1srH52eYQnYlFdKQ5RnTwliF_OBiIewoYhc4CdA30Svo
*/

`include "bsg_defines.v"
`include "bsg_axi_bus_pkg.vh"
`include "bsg_manycore_packet.vh"
`include "bsg_bladerunner_rom_pkg.vh"
`include "bsg_manycore_link_to_axil_pkg.v"

module bsg_manycore_link_to_axil
  import bsg_bladerunner_rom_pkg::*;
  import bsg_manycore_link_to_axil_pkg::*;
#(
  // endpoint parameters
  parameter x_cord_width_p = "inv"
  , parameter y_cord_width_p = "inv"
  , parameter addr_width_p = "inv"
  , parameter data_width_p = "inv"
  , parameter max_out_credits_p = "inv"
  , parameter load_id_width_p = "inv"
  , parameter link_sif_width_lp = `bsg_manycore_link_sif_width(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p,load_id_width_p)
  , localparam num_endpoints_lp = num_endpoints_gp
) (
  input                                                clk_i
  ,input                                                reset_i
  // axil signals
  ,input                                                axil_awvalid_i
  ,input  [                31:0]                        axil_awaddr_i
  ,output                                               axil_awready_o
  ,input                                                axil_wvalid_i
  ,input  [                31:0]                        axil_wdata_i
  ,input  [                 3:0]                        axil_wstrb_i
  ,output                                               axil_wready_o
  ,output [                 1:0]                        axil_bresp_o
  ,output                                               axil_bvalid_o
  ,input                                                axil_bready_i
  ,input  [                31:0]                        axil_araddr_i
  ,input                                                axil_arvalid_i
  ,output                                               axil_arready_o
  ,output [                31:0]                        axil_rdata_o
  ,output [                 1:0]                        axil_rresp_o
  ,output                                               axil_rvalid_o
  ,input                                                axil_rready_i
  // manycore link signals
  ,input  [num_endpoints_lp-1:0][link_sif_width_lp-1:0] link_sif_i
  ,output [num_endpoints_lp-1:0][link_sif_width_lp-1:0] link_sif_o
  ,input  [num_endpoints_lp-1:0][   x_cord_width_p-1:0] my_x_i
  ,input  [num_endpoints_lp-1:0][   y_cord_width_p-1:0] my_y_i
  // print stat
  ,output [num_endpoints_lp-1:0]                        print_stat_v_o
  ,output [num_endpoints_lp-1:0][     data_width_p-1:0] print_stat_tag_o
);

  localparam num_slots_lp = num_endpoints_lp*2; // not using assertion here

  // axil parameters
  // localparam axil_mosi_bus_width_lp = `bsg_axil_mosi_bus_width(1);
  // localparam axil_miso_bus_width_lp = `bsg_axil_miso_bus_width(1);

  `declare_bsg_axil_bus_s(1, bsg_axil_mosi_bus_s, bsg_axil_miso_bus_s);
  bsg_axil_mosi_bus_s s_axil_bus_li_cast;
  bsg_axil_miso_bus_s s_axil_bus_lo_cast;

  assign s_axil_bus_li_cast.awaddr  = axil_awaddr_i;
  assign s_axil_bus_li_cast.awvalid = axil_awvalid_i;
  assign axil_awready_o             = s_axil_bus_lo_cast.awready;
  assign s_axil_bus_li_cast.wdata   = axil_wdata_i;
  assign s_axil_bus_li_cast.wstrb   = axil_wstrb_i;
  assign s_axil_bus_li_cast.wvalid  = axil_wvalid_i;
  assign axil_wready_o              = s_axil_bus_lo_cast.wready;
  assign axil_bresp_o               = s_axil_bus_lo_cast.bresp;
  assign axil_bvalid_o              = s_axil_bus_lo_cast.bvalid;
  assign s_axil_bus_li_cast.bready  = axil_bready_i;
  assign s_axil_bus_li_cast.araddr  = axil_araddr_i;
  assign s_axil_bus_li_cast.arvalid = axil_arvalid_i;
  assign axil_arready_o             = s_axil_bus_lo_cast.arready;
  assign axil_rdata_o               = s_axil_bus_lo_cast.rdata;
  assign axil_rresp_o               = s_axil_bus_lo_cast.rresp;
  assign axil_rvalid_o              = s_axil_bus_lo_cast.rvalid;
  assign s_axil_bus_li_cast.rready  = axil_rready_i;

  // ---------------------------------------------------------------------
  // bladerunner rom
  // ---------------------------------------------------------------------

  localparam lg_rom_els_lp    = `BSG_SAFE_CLOG2(rom_els_p); // TODO: _gp?
  localparam rom_addr_width_p = 32                        ;

  logic [rom_addr_width_p-1:0] rom_addr_li;
  logic [                31:0] rom_data_lo;

  bsg_bladerunner_configuration #(
    .width_p     (rom_width_p  )
    ,.addr_width_p(lg_rom_els_lp)
  ) configuration_rom (
    .addr_i(rom_addr_li[2+:lg_rom_els_lp])
    ,.data_o(rom_data_lo                  )
  );


  // ---------------------------------------------------------------------
  // memory mapped to stream packets
  // ---------------------------------------------------------------------
  logic [num_slots_lp-1:0][31:0] axil_fifos_li ;
  logic [num_slots_lp-1:0]       axil_fifos_v_li    ;
  logic [num_slots_lp-1:0]       axil_fifos_ready_lo;
  logic [num_slots_lp-1:0][31:0] axil_fifos_lo ;
  logic [num_slots_lp-1:0]       axil_fifos_v_lo    ;
  logic [num_slots_lp-1:0]       axil_fifos_ready_li;

  logic [num_endpoints_lp-1:0][31:0] out_credits_li;
  logic [    num_slots_lp-1:0][31:0] rcv_vacancy_li;

  bsg_axil_to_fifos #(
    .num_slots_p     (num_slots_lp       )
    ,.fifo_width_p    (32                 )
  ) axil_to_fifos (
    .clk_i        (clk_i              )
    ,.reset_i      (reset_i            )
    ,.s_axil_bus_i (s_axil_bus_li_cast )
    ,.s_axil_bus_o (s_axil_bus_lo_cast )
    ,.fifos_i      (axil_fifos_li      )
    ,.fifos_v_i    (axil_fifos_v_li    )
    ,.fifos_ready_o(axil_fifos_ready_lo)
    ,.fifos_o      (axil_fifos_lo      )
    ,.fifos_v_o    (axil_fifos_v_lo    )
    ,.fifos_ready_i(axil_fifos_ready_li)
    ,.rom_addr_o   (rom_addr_li        )
    ,.rom_data_i   (rom_data_lo        )
    ,.rcv_vacancy_i(rcv_vacancy_li     )
    ,.out_credits_i(out_credits_li     )
  );


  // ---------------------------------------------------------------------
  // fifo width converters
  // ---------------------------------------------------------------------

  // host to manycore upsizer

  logic [num_slots_lp-1:0]                        mcl_fifo_v_li    ;
  logic [num_slots_lp-1:0][mcl_fifo_width_gp-1:0] mcl_fifo_data_li ;
  logic [num_slots_lp-1:0]                        mcl_fifo_ready_lo;

  localparam valid_width_lp    = mcl_fifo_width_gp/32    ;
  localparam yumi_cnt_width_lp = $clog2(valid_width_lp+1);

  logic [num_slots_lp-1:0][   valid_width_lp-1:0] sipo_valid_lo   ;
  logic [num_slots_lp-1:0][yumi_cnt_width_lp-1:0] sipo_yumi_cnt_li;

  for (genvar i=0; i<num_slots_lp; i++) begin : upsizer
    bsg_serial_in_parallel_out #(
      .width_p(32                  )
      ,.els_p  (mcl_fifo_width_gp/32)
    ) sipo (
      .clk_i     (clk_i                 )
      ,.reset_i   (reset_i               )
      ,.valid_i   (axil_fifos_v_lo[i]    )
      ,.data_i    (axil_fifos_lo[i]      )
      ,.ready_o   (axil_fifos_ready_li[i])
      ,.valid_o   (sipo_valid_lo[i]      )
      ,.data_o    (mcl_fifo_data_li[i]   )
      ,.yumi_cnt_i(sipo_yumi_cnt_li[i]   )
    );
    assign mcl_fifo_v_li[i]    = &sipo_valid_lo[i];
    assign sipo_yumi_cnt_li[i] = (mcl_fifo_v_li[i] & mcl_fifo_ready_lo[i])
                                 ? (yumi_cnt_width_lp)'(valid_width_lp) :'0;
  end : upsizer


  // manycore to host downsizer

  logic [num_slots_lp-1:0]                        mcl_fifo_v_lo    ;
  logic [num_slots_lp-1:0][mcl_fifo_width_gp-1:0] mcl_fifo_data_lo ;
  logic [num_slots_lp-1:0]                        mcl_fifo_ready_li;

  logic [num_slots_lp-1:0]                        rcv_fifo_v_lo;
  logic [num_slots_lp-1:0]                        rcv_fifo_r_li;
  logic [num_slots_lp-1:0][mcl_fifo_width_gp-1:0] rcv_fifo_lo  ;

  wire [num_slots_lp-1:0] rcv_enqueue = mcl_fifo_v_lo & mcl_fifo_ready_li;
  wire [num_slots_lp-1:0] rcv_dequeue = rcv_fifo_r_li & rcv_fifo_v_lo    ;

  wire [num_slots_lp-1:0] piso_yumi_li = axil_fifos_ready_lo & axil_fifos_v_li;

  logic [num_slots_lp-1:0][`BSG_WIDTH(rcv_fifo_els_gp)-1:0] rcv_vacancy_lo;

  for (genvar i=0; i<num_slots_lp; i++) begin : dnsizer
    bsg_counter_up_down #(
      .max_val_p (rcv_fifo_els_gp)
      ,.init_val_p(rcv_fifo_els_gp)
      ,.max_step_p(1             )
    ) rcv_vacancy_cnt (
      .clk_i  (clk_i            )
      ,.reset_i(reset_i          )
      ,.down_i (rcv_enqueue[i]   )
      ,.up_i   (rcv_dequeue[i]   )
      ,.count_o(rcv_vacancy_lo[i])
    );
    assign rcv_vacancy_li[i] = 32'(rcv_vacancy_lo[i]);

    bsg_fifo_1r1w_small #(
      .width_p           (mcl_fifo_width_gp)
      ,.els_p             (rcv_fifo_els_gp  )
      ,.ready_THEN_valid_p(0                )  // for input
    ) rcv_fifo (
      .clk_i  (clk_i               )
      ,.reset_i(reset_i             )
      ,.v_i    (mcl_fifo_v_lo[i]    )
      ,.ready_o(mcl_fifo_ready_li[i])
      ,.data_i (mcl_fifo_data_lo[i] )
      ,.v_o    (rcv_fifo_v_lo[i]    )
      ,.data_o (rcv_fifo_lo[i]      )
      ,.yumi_i (rcv_dequeue[i]      )
    );

    bsg_parallel_in_serial_out #(
      .width_p(32                  )
      ,.els_p  (mcl_fifo_width_gp/32)
    ) piso (
      .clk_i  (clk_i               )
      ,.reset_i(reset_i             )
      ,.valid_i(rcv_fifo_v_lo[i]    )
      ,.data_i (rcv_fifo_lo[i]      )
      ,.ready_o(rcv_fifo_r_li[i]    )
      ,.valid_o(axil_fifos_v_li[i]  )
      ,.data_o (axil_fifos_li[i]    )
      ,.yumi_i (piso_yumi_li[i]     )
    );
  end : dnsizer


  // ---------------------------------------------------------------------
  // fifo to manycore endpoint standard
  // ---------------------------------------------------------------------
  logic [num_endpoints_lp-1:0][`BSG_WIDTH(max_out_credits_p)-1:0] out_credits_lo;

  logic [num_endpoints_lp-1:0] rcv_fifo_th_li;

  for (genvar i=0; i<num_endpoints_lp; i=i+1) begin : fifo_2_ep
    assign out_credits_li[i] = 32'(out_credits_lo[i]);
    assign rcv_fifo_th_li[i] = (rcv_vacancy_lo[i*2]<max_out_credits_p);

    bsg_manycore_endpoint_to_fifos #(
      .fifo_width_p     (mcl_fifo_width_gp)
      ,.x_cord_width_p   (x_cord_width_p   )
      ,.y_cord_width_p   (y_cord_width_p   )
      ,.addr_width_p     (addr_width_p     )
      ,.data_width_p     (data_width_p     )
      ,.max_out_credits_p(max_out_credits_p)
      ,.load_id_width_p  (load_id_width_p  )
    ) mc_ep_to_fifos (
      .clk_i           (clk_i                    )
      ,.reset_i         (reset_i                  )
      ,.fifo_v_i        (mcl_fifo_v_li[i*2+:2]    )
      ,.fifo_data_i     (mcl_fifo_data_li[i*2+:2] )
      ,.fifo_ready_o    (mcl_fifo_ready_lo[i*2+:2])
      ,.fifo_v_o        (mcl_fifo_v_lo[i*2+:2]    )
      ,.fifo_data_o     (mcl_fifo_data_lo[i*2+:2] )
      ,.fifo_ready_i    (mcl_fifo_ready_li[i*2+:2])
      ,.link_sif_i      (link_sif_i[i]            )
      ,.link_sif_o      (link_sif_o[i]            )
      ,.my_x_i          (my_x_i[i]                )
      ,.my_y_i          (my_y_i[i]                )
      ,.rcv_fifo_th_i   (rcv_fifo_th_li[i]        )
      ,.out_credits_o   (out_credits_lo[i]        )
      ,.print_stat_v_o  (print_stat_v_o[i]        )
      ,.print_stat_tag_o(print_stat_tag_o[i]      )
    );
  end : fifo_2_ep

endmodule
