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


/*
*  bsg_manycore_link_to_axil.v
*
*      AXIL memory-mapped (HOST side)   <->   manycore endpoint standard (MC side)
*
*                  axil_to_fifo_tx
*          ________________________________________        __________
*         |                                        | -->  | manycore |
*         | {mm2s}->{fifo(32)}->{sipo}             |      | endpoint |
*         |________________________________________| -->  | to fifos |
*      TX/                                                |          | ==> link_sif_o
* axil===           axil_to_fifo_rx                       |          |
*      RX\ ________________________________________       |          | <== link_sif_i
*         |                                        | <--  |          |
*         | {mm2s}<-{fifo(32)}<-{piso}<-{rcv fifo} |      |          |
*         |________________________________________| <--  |__________|
*              ^
*          ____|_______
*         | config rom |
*         |____________|(bladerunner hardware configurations)
*
*
* Note:
* 1. Because of the conversion latency of piso, we use a rcv fifo to ensure that the link
*    always accepts the response packets from manycore, which is a compulsive requirement of
*    the manycore network
*
* 2. The definition of mm2s control registers are defined at bsg_manycore_link_to_axil_pkg.v
*    a. To read the ROM, access the base address defined as rom_base_addr_gp.
*       The axil address is bytes wise, while the rom address is word wise.
*    b. To read from the rx fifo:
*       i.  poll from the Receive Data FIFO word Occupancy(RDFO) until greater than the #words of one packet
*       ii. read packet data from the Receive Destination Register(RDR).
*           If read from a empty fifo, RDFO will not be updated and you get ZEROs
*    c. To write to manycore through the tx fifo (one packet per time):
*       i.  read the Transmit Data FIFO word Vacancy(TDFV) and save the value as vacancy_before
*       ii. write words of one mc packet to the Transmit Destination Register(TDR).
*       iii.poll the TDFV as vacancy_after until vacancy_after = vacancy_before for successful writing to the mc
*
*       Note, We assume that the initial FIFO is empty, and writing to a empty FIFO will always success.
*
*/

`include "bsg_defines.v"
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
) (
  input                          clk_i
  ,input                          reset_i
  // axil signals
  ,input                          axil_awvalid_i
  ,input  [                 31:0] axil_awaddr_i
  ,output                         axil_awready_o
  ,input                          axil_wvalid_i
  ,input  [                 31:0] axil_wdata_i
  ,input  [                  3:0] axil_wstrb_i
  ,output                         axil_wready_o
  ,output [                  1:0] axil_bresp_o
  ,output                         axil_bvalid_o
  ,input                          axil_bready_i
  ,input  [                 31:0] axil_araddr_i
  ,input                          axil_arvalid_i
  ,output                         axil_arready_o
  ,output [                 31:0] axil_rdata_o
  ,output [                  1:0] axil_rresp_o
  ,output                         axil_rvalid_o
  ,input                          axil_rready_i
  // manycore link signals
  ,input  [link_sif_width_lp-1:0] link_sif_i
  ,output [link_sif_width_lp-1:0] link_sif_o
  ,input  [   x_cord_width_p-1:0] my_x_i
  ,input  [   y_cord_width_p-1:0] my_y_i
  // print stat
  ,output                         print_stat_v_o
  ,output [     data_width_p-1:0] print_stat_tag_o
);


// divided by 2 because it is credit-based flow control,
// and both req and rsp channels are buffered
localparam num_fifos_lp = mcl_num_fifos_gp/2;

localparam rom_addr_width_lp = `BSG_SAFE_CLOG2(rom_els_p*4); // TODO: _gp?

// manycore link fifo vectors
logic [mcl_fifo_width_gp-1:0] mc_req_lo        ;
logic                         mc_req_v_lo      ;
logic                         mc_req_ready_li  ;
logic [mcl_fifo_width_gp-1:0] host_rsp_li      ;
logic                         host_rsp_v_li    ;
logic                         host_rsp_ready_lo;
logic [mcl_fifo_width_gp-1:0] host_req_li      ;
logic                         host_req_v_li    ;
logic                         host_req_ready_lo;
logic [mcl_fifo_width_gp-1:0] mc_rsp_lo        ;
logic                         mc_rsp_v_lo      ;
logic                         mc_rsp_ready_li  ;

// rom data
logic [rom_addr_width_lp-1:0] rom_addr_lo;
logic [                 31:0] rom_data_li;

// credits signals
logic [                 num_fifos_lp-1:0][`BSG_WIDTH(axil_fifo_els_gp)-1:0] tx_vacancy_lo ;
logic [                 num_fifos_lp-1:0][ `BSG_WIDTH(rcv_fifo_els_gp)-1:0] rcv_vacancy_lo;
logic [`BSG_WIDTH(max_out_credits_p)-1:0]                                   out_credits_lo;

// ---------------------------------------------------------------------
// memory mapped to stream packets
// ---------------------------------------------------------------------

// host <---credit--- mc
// host <---packet--- mc
bsg_axil_to_fifos_rx #(
  .num_fifos_p      (num_fifos_lp     )
  ,.mcl_fifo_width_p (mcl_fifo_width_gp)
  ,.rom_addr_width_p (rom_addr_width_lp)
  ,.max_out_credits_p(max_out_credits_p)
) axil_rx_fifos (
  .clk_i        (clk_i                             )
  ,.reset_i      (reset_i                           )
  ,.araddr_i     (axil_araddr_i                     )
  ,.arvalid_i    (axil_arvalid_i                    )
  ,.arready_o    (axil_arready_o                    )
  ,.rdata_o      (axil_rdata_o                      )
  ,.rresp_o      (axil_rresp_o                      )
  ,.rvalid_o     (axil_rvalid_o                     )
  ,.rready_i     (axil_rready_i                     )
  ,.fifo_data_i  ({mc_req_lo, mc_rsp_lo}            )
  ,.fifo_v_i     ({mc_req_v_lo, mc_rsp_v_lo}        )
  ,.fifo_ready_o ({mc_req_ready_li, mc_rsp_ready_li})
  ,.rcv_vacancy_o(rcv_vacancy_lo                    )
  ,.tx_vacancy_i (tx_vacancy_lo                     )
  ,.mcl_credits_i(out_credits_lo                    )
  ,.rom_addr_o   (rom_addr_lo                       )
  ,.rom_data_i   (rom_data_li                       )
);

// host ---packet---> mc
// host ---credit---> mc
bsg_axil_to_fifos_tx #(
  .num_fifos_p (num_fifos_lp     )
  ,.mcl_fifo_width_p(mcl_fifo_width_gp)
) axil_tx_fifos (
  .clk_i       (clk_i                                 )
  ,.reset_i     (reset_i                               )
  ,.awaddr_i    (axil_awaddr_i                         )
  ,.awvalid_i   (axil_awvalid_i                        )
  ,.awready_o   (axil_awready_o                        )
  ,.wdata_i     (axil_wdata_i                          )
  ,.wstrb_i     (axil_wstrb_i                          )
  ,.wvalid_i    (axil_wvalid_i                         )
  ,.wready_o    (axil_wready_o                         )
  ,.bresp_o     (axil_bresp_o                          )
  ,.bvalid_o    (axil_bvalid_o                         )
  ,.bready_i    (axil_bready_i                         )
  ,.fifo_data_o ({host_rsp_li, host_req_li}            )
  ,.fifo_v_o    ({host_rsp_v_li, host_req_v_li}        )
  ,.fifo_ready_i({host_rsp_ready_lo, host_req_ready_lo})
  ,.tx_vacancy_o(tx_vacancy_lo                         )
);


// ---------------------------------------------------------------------
// bladerunner rom
// ---------------------------------------------------------------------

bsg_bladerunner_configuration #(
  .width_p     (rom_width_p        )
  ,.addr_width_p(rom_addr_width_lp-2)  // word address
) configuration_rom (
  .addr_i(rom_addr_lo[2+:rom_addr_width_lp-2])
  ,.data_o(rom_data_li                        )
);


// ---------------------------------------------------------------------
// fifo to manycore endpoint standard
// ---------------------------------------------------------------------

bsg_manycore_endpoint_to_fifos #(
  .mcl_fifo_width_p (mcl_fifo_width_gp)
  ,.x_cord_width_p   (x_cord_width_p   )
  ,.y_cord_width_p   (y_cord_width_p   )
  ,.addr_width_p     (addr_width_p     )
  ,.data_width_p     (data_width_p     )
  ,.max_out_credits_p(max_out_credits_p)
  ,.load_id_width_p  (load_id_width_p  )
) mc_ep_to_fifos (
  .clk_i               (clk_i            )
  ,.reset_i             (reset_i          )

  // fifo interface
  ,.mc_req_o            (mc_req_lo        )
  ,.mc_req_v_o          (mc_req_v_lo      )
  ,.mc_req_ready_i      (mc_req_ready_li  )
  ,.host_rsp_i          (host_rsp_li      )
  ,.host_rsp_v_i        (host_rsp_v_li    )
  ,.host_rsp_ready_o    (host_rsp_ready_lo)
  ,.host_req_i          (host_req_li      )
  ,.host_req_v_i        (host_req_v_li    )
  ,.host_req_ready_o    (host_req_ready_lo)
  ,.mc_rsp_o            (mc_rsp_lo        )
  ,.mc_rsp_v_o          (mc_rsp_v_lo      )
  ,.mc_rsp_ready_i      (mc_rsp_ready_li  )

  // manycore link
  ,.link_sif_i          (link_sif_i       )
  ,.link_sif_o          (link_sif_o       )
  ,.my_x_i              (my_x_i           )
  ,.my_y_i              (my_y_i           )

  // credit control
  ,.mc_rsp_rcv_vacancy_i(rcv_vacancy_lo[0])
  ,.out_credits_o       (out_credits_lo   )

  // stat log
  ,.print_stat_v_o      (print_stat_v_o   )
  ,.print_stat_tag_o    (print_stat_tag_o )
);

endmodule
