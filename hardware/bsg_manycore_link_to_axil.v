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
* This is an open-source module to bridge the manycore symmetric links with the
* AXI-Lite master interface.
* It handles the AXIL transactions differently based on the w/r address.
* Write -> tx_FIFO -> SIPO -> host_request
* Read  <- PISO <- rx_FIFO <- mc_response
*       <- PISO <- rx_FIFO <- mc_request
*       <- ROM
*       <- vacancy of tx_FIFO + SIPO
*       <- endpoint out credits
*       <- occupancy of rx_FIFO (mc_request)
* And it shall always complete the AXIL transaction if gets invalid address.
*
* Google doc:
* https://docs.google.com/document/d/1-jSBELaYREEqtGOUD4_yAnl4EJRAZ0hOJfu-_1ZQllw
*
*/

`include "bsg_manycore_packet.vh"

module bsg_manycore_link_to_axil
  import bsg_bladerunner_rom_pkg::*;
  import bsg_manycore_link_to_axil_pkg::*;
#(
  // maycore link params
  parameter fifo_width_p = mcl_fifo_width_gp
  , localparam axil_data_width_p = axil_data_width_gp
  , localparam axil_addr_width_p = axil_addr_width_gp
  // endpoint params
  , parameter x_cord_width_p = "inv"
  , parameter y_cord_width_p = "inv"
  , parameter load_id_width_p = "inv"
  , parameter addr_width_p = "inv"
  , parameter data_width_p = "inv"
   // hardcoded, matching bladerunner_configuration_rom
  , parameter max_out_credits_p = 200 
  , localparam link_sif_width_lp = `bsg_manycore_link_sif_width(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p,load_id_width_p)
) (
  input                                clk_i
  ,input                                reset_i
  // axil signals
  ,input                                axil_awvalid_i
  ,input  [      axil_addr_width_p-1:0] axil_awaddr_i
  ,output                               axil_awready_o
  ,input                                axil_wvalid_i
  ,input  [      axil_data_width_p-1:0] axil_wdata_i
  ,input  [(axil_data_width_gp>>3)-1:0] axil_wstrb_i  // unused
  ,output                               axil_wready_o
  ,output [                        1:0] axil_bresp_o
  ,output                               axil_bvalid_o
  ,input                                axil_bready_i
  ,input  [      axil_addr_width_p-1:0] axil_araddr_i
  ,input                                axil_arvalid_i
  ,output                               axil_arready_o
  ,output [      axil_data_width_p-1:0] axil_rdata_o
  ,output [                        1:0] axil_rresp_o
  ,output                               axil_rvalid_o
  ,input                                axil_rready_i
  // manycore link signals
  ,input  [      link_sif_width_lp-1:0] link_sif_i
  ,output [      link_sif_width_lp-1:0] link_sif_o
  ,input  [         x_cord_width_p-1:0] my_x_i
  ,input  [         y_cord_width_p-1:0] my_y_i
  // print stat
  ,output                               print_stat_v_o
  ,output [           data_width_p-1:0] print_stat_tag_o
);


  // Dependencies between channel handshake signals
  // -------------------------------------------------------
  // axil write channels
  // bvalid : must wait for wvalid & wready
  // bresp: must be signaled only after the write data

  // See details in ARM's DOC:
  // https://developer.arm.com/docs/ihi0022/d ,A3.3.1
  // -------------------------------------------------------


  // axil write data path
  // -----------------------

  // host request
  logic [axil_data_width_p-1:0] tx_wdata_li ;
  logic                         tx_wen_li   ;
  logic                         tx_wready_lo;

  logic awready_lo;
  logic wready_lo ;
  logic bvalid_r, bvalid_n;
  logic [1:0] bresp_lo;

  assign axil_awready_o = awready_lo;
  assign axil_wready_o  = wready_lo;
  assign axil_bvalid_o  = bvalid_r;
  assign axil_bresp_o   = bresp_lo;

  wire is_write_to_tdr = (axil_awaddr_i == mcl_fifo_base_addr_gp + mcl_ofs_tdr_gp);

  always_comb begin
    awready_lo = 1'b0;
    wready_lo  = 1'b1;

    tx_wen_li   = 1'b0;
    tx_wdata_li = '0;

    bvalid_n = bvalid_r;
    bresp_lo = axil_resp_OKAY_gp; // always OKAY even writing to the undefined address

    if (axil_awvalid_i & axil_wvalid_i) begin
      wready_lo = is_write_to_tdr ? tx_wready_lo : 1'b1;
      awready_lo = wready_lo;

      tx_wen_li   = is_write_to_tdr;
      tx_wdata_li = axil_wdata_i;
    end

    // write response occurs after
    if (axil_bready_i & axil_bvalid_o)
      bvalid_n = 1'b0;
    else
      bvalid_n = axil_wready_o & axil_wvalid_i;
  end

  always_ff @(posedge clk_i) begin
    if (reset_i)
      bvalid_r <= 1'b0;
    else
      bvalid_r <= bvalid_n;
  end


  // axil read data paths
  // -----------------------

  logic [`BSG_WIDTH(max_out_credits_p)-1:0] ep_out_credits_lo;

  // 1. mc response
  logic [axil_data_width_p-1:0] tx_rdata_lo ;
  logic                         tx_rv_lo   ;
  logic                         tx_rready_li;

  // 2. credit registers

  // host credit
  localparam host_credits_width_lp = `BSG_WIDTH(fifo_width_p/axil_data_width_p*mcl_host_credits_gp);
  logic [host_credits_width_lp-1:0] host_credits_lo;

  // rx fifo occupancy for manycore request
  localparam integer piso_els_lp = fifo_width_p/axil_data_width_p;
  localparam pkt_cnt_width_lp = `BSG_WIDTH(mcl_host_credits_gp*piso_els_lp);
  logic [pkt_cnt_width_lp-1:0] mc_req_words_lo;

  // 3. mc request
  logic [axil_data_width_p-1:0] rx_rdata_lo ;
  logic                         rx_rv_lo    ;
  logic                         rx_rready_li;

  // 4. rom
  logic [axil_addr_width_p-1:0] rx_rom_addr_li;
  logic [axil_data_width_p-1:0] rx_rom_data_lo;


  logic                         arready_lo;
  logic [axil_data_width_p-1:0] rdata_r, rdata_n;
  logic                         rvalid_r, rvalid_n;
  logic [                  1:0] rresp_lo;

  assign axil_arready_o = arready_lo;
  assign axil_rdata_o   = rdata_r;
  assign axil_rvalid_o  = rvalid_r;
  assign axil_rresp_o   = rresp_lo;

  wire is_read_credit        = (axil_araddr_i == mcl_ofs_credits_gp)                         ;
  wire is_read_rdr_rsp       = (axil_araddr_i == mcl_fifo_base_addr_gp + mcl_ofs_rdr_rsp_gp) ;
  wire is_read_tdfv_host_req = (axil_araddr_i == mcl_fifo_base_addr_gp + mcl_ofs_tdfv_req_gp);
  wire is_read_rdfo_mc_req   = (axil_araddr_i == mcl_fifo_base_addr_gp + mcl_ofs_rdfo_req_gp);
  wire is_read_rdr_req       = (axil_araddr_i == mcl_fifo_base_addr_gp + mcl_ofs_rdr_req_gp) ;
  wire is_read_rom           = (axil_araddr_i >= mcl_rom_base_addr_gp) &&
                               (axil_araddr_i < mcl_rom_base_addr_gp + (1<<$clog2(rom_els_gp*rom_width_gp/8)));

  always_comb begin

    arready_lo = 1'b0;
    rdata_n = rdata_r;
    rvalid_n = rvalid_r;

    rresp_lo = axil_resp_OKAY_gp; // always OKAY even reading from the undefined address

    // ready from data paths to read
    tx_rready_li = 1'b0;
    rx_rready_li = 1'b0;
    rx_rom_addr_li = '0;

    if (axil_arvalid_i) begin
      if (is_read_credit) begin  // always accept and return the manycore endpoint out credits
        arready_lo = 1'b1;
        rdata_n = axil_data_width_p'(ep_out_credits_lo);
      end
      else if (is_read_rdr_rsp) begin
        tx_rready_li = 1'b1;
        arready_lo = tx_rready_li & tx_rv_lo;  // accept the read address only when fifo data is valid
        rdata_n = tx_rdata_lo;
      end
      else if (is_read_tdfv_host_req) begin  // always accept and return the vacancy of host req fifo in words
        arready_lo = 1'b1;
        rdata_n = axil_data_width_p'(host_credits_lo);
      end
      else if (is_read_rdfo_mc_req) begin  // always accept and return the occupancy of rx words
        arready_lo = 1'b1;
        rdata_n = axil_data_width_p'(mc_req_words_lo);
      end
      else if (is_read_rdr_req) begin
        rx_rready_li = 1'b1;
        arready_lo = rx_rready_li & rx_rv_lo;  // accept the read address only when fifo data is valid
        rdata_n = rx_rdata_lo;
      end
      else if (is_read_rom) begin
        arready_lo = 1'b1;
        rx_rom_addr_li = (axil_araddr_i - mcl_rom_base_addr_gp);
        rdata_n = axil_data_width_p'(rx_rom_data_lo);
      end
      else begin
        arready_lo = 1'b1;
        rdata_n = axil_data_width_p'(32'hdead_beef);
      end
      // assert the rdata valid after it decides to accept the address, which
      // also means rx fifo being dequeued or the rom being read
      rvalid_n = axil_arready_o & axil_arvalid_i;
    end
  end

  // hold the rdata and rvalid until accepted by master
  always_ff @(posedge clk_i) begin
    if (reset_i | (axil_rready_i & axil_rvalid_o)) begin
      rdata_r <= '0;
      rvalid_r <= 1'b0;
    end
    else begin
      rdata_r <= rdata_n;
      rvalid_r <= rvalid_n;
    end
  end

  // ----------------------------
  // bladerunner rom
  // ----------------------------
  // The rom not necessarily in the mcl, so we put its parameters in other package.
  localparam rom_addr_width_lp = `BSG_SAFE_CLOG2(rom_els_gp);

  wire [rom_addr_width_lp-1:0] br_rom_addr_li = rx_rom_addr_li[$clog2(rom_width_gp/8)+:rom_addr_width_lp];

  logic [rom_width_gp-1:0] br_rom_data_lo;

  assign rx_rom_data_lo = axil_data_width_p'(br_rom_data_lo);

  bsg_bladerunner_configuration #(
    .width_p     (rom_width_gp     ),
    .addr_width_p(rom_addr_width_lp)
  ) configuration_rom (
    .addr_i(br_rom_addr_li),
    .data_o(br_rom_data_lo)
  );


  // --------------------------------------------
  // axil fifo data stream
  // --------------------------------------------

  // host ---packet---> mc
  logic [fifo_width_p-1:0] host_req_lo      ;
  logic                    host_req_v_lo    ;
  logic                    host_req_ready_li;

  // host <---credit--- mc
  logic [fifo_width_p-1:0] mc_rsp_li        ;
  logic                    mc_rsp_v_li      ;
  logic                    mc_rsp_ready_lo  ;

  // mc ---packet---> host
  logic [    fifo_width_p-1:0] mc_req_li      ;
  logic                        mc_req_v_li    ;
  logic                        mc_req_ready_lo;

  bsg_mcl_axil_fifos_master #(
    .fifo_width_p       (fifo_width_p       ),
    .host_credits_p     (mcl_host_credits_gp),
    .axil_data_width_p  (axil_data_width_gp )
  ) tx (
    .clk_i           (clk_i            ),
    .reset_i         (reset_i          ),

    // monitor signals
    .w_data_i        (tx_wdata_li      ),
    .w_v_i           (tx_wen_li        ),
    .w_ready_o       (tx_wready_lo     ),
    .r_data_o        (tx_rdata_lo      ),
    .r_v_o           (tx_rv_lo         ),
    .r_ready_i       (tx_rready_li     ),
    .host_req_o      (host_req_lo      ),
    .host_req_v_o    (host_req_v_lo    ),
    .host_req_ready_i(host_req_ready_li),
    .host_credits_o  (host_credits_lo  ),
    .mc_rsp_i        (mc_rsp_li        ),
    .mc_rsp_v_i      (mc_rsp_v_li      ),
    .mc_rsp_ready_o  (mc_rsp_ready_lo  )
  );


  bsg_mcl_axil_fifos_slave #(
    .fifo_width_p       (fifo_width_p       ),
    .mc_write_capacity_p(mcl_mc_write_cap_gp),
    .axil_data_width_p  (axil_data_width_p  )
  ) rx (
    .clk_i         (clk_i          ),
    .reset_i       (reset_i        ),

    // monitor signals
    .r_data_o      (rx_rdata_lo    ),
    .r_v_o         (rx_rv_lo       ),
    .r_ready_i     (rx_rready_li   ),
    .mc_req_i      (mc_req_li      ),
    .mc_req_v_i    (mc_req_v_li    ),
    .mc_req_ready_o(mc_req_ready_lo),
    .mc_req_words_o(mc_req_words_lo)
  );

  // See reference below for how to attach modules to the manycore endpoint:
  // Xie, S, Taylor, M. B. (2018). The BaseJump Manycore Accelerator Network. arXiv:1808.00650.

  // --------------------------------------------
  // fifo to manycore endpoint standard
  // --------------------------------------------

  bsg_manycore_endpoint_to_fifos #(
    .fifo_width_p     (fifo_width_p     ),
    .x_cord_width_p   (x_cord_width_p   ),
    .y_cord_width_p   (y_cord_width_p   ),
    .load_id_width_p  (load_id_width_p  ),
    .addr_width_p     (addr_width_p     ),
    .data_width_p     (data_width_p     ),
    .max_out_credits_p(max_out_credits_p)
  ) mc_ep_to_fifos (
    .clk_i           (clk_i            ),
    .reset_i         (reset_i          ),

    // fifo interface
    .mc_req_o        (mc_req_li        ),
    .mc_req_v_o      (mc_req_v_li      ),
    .mc_req_ready_i  (mc_req_ready_lo  ),

    .host_req_i      (host_req_lo      ),
    .host_req_v_i    (host_req_v_lo    ),
    .host_req_ready_o(host_req_ready_li),

    .mc_rsp_o        (mc_rsp_li        ),
    .mc_rsp_v_o      (mc_rsp_v_li      ),
    .mc_rsp_ready_i  (mc_rsp_ready_lo  ),

    // manycore link
    .link_sif_i      (link_sif_i       ),
    .link_sif_o      (link_sif_o       ),
    .my_x_i          (my_x_i           ),
    .my_y_i          (my_y_i           ),
    .out_credits_o   (ep_out_credits_lo),
    // stat log
    .print_stat_v_o  (print_stat_v_o   ),
    .print_stat_tag_o(print_stat_tag_o )
  );

endmodule
