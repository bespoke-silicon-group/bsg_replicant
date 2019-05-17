/**
*  hb_mc_wrapper.v
*
*  top level wrapper for the manycore design
*/

`include "cl_manycore_pkg.v"
import cl_manycore_pkg::*;

`include "bsg_bladerunner_rom_pkg.vh"
import bsg_bladerunner_rom_pkg::*;

`include "bsg_manycore_packet.vh"
`include "bsg_axi_bus_pkg.vh"

module hb_mc_wrapper #(
  parameter axi_id_width_p = "inv"
  , parameter axi_addr_width_p = "inv"
  , parameter axi_data_width_p = "inv"
  , localparam axil_mosi_bus_width_lp = `bsg_axil_mosi_bus_width(1)
  , localparam axil_miso_bus_width_lp = `bsg_axil_miso_bus_width(1)
  , localparam axi_mosi_bus_width_lp = `bsg_axi_mosi_bus_width(1, axi_id_width_p, axi_addr_width_p, axi_data_width_p)
  , localparam axi_miso_bus_width_lp = `bsg_axi_miso_bus_width(1, axi_id_width_p, axi_addr_width_p, axi_data_width_p)
) (
  input                               clk_i
  ,input                               resetn_i
  ,input  [axil_mosi_bus_width_lp-1:0] s_axil_ocl_bus_i
  ,output [axil_miso_bus_width_lp-1:0] s_axil_ocl_bus_o
  ,input  [ axi_mosi_bus_width_lp-1:0] s_axi_pcis_bus_i
  ,output [ axi_miso_bus_width_lp-1:0] s_axi_pcis_bus_o
  ,output [ axi_mosi_bus_width_lp-1:0] m_axi_ddr_bus_o
  ,input  [ axi_miso_bus_width_lp-1:0] m_axi_ddr_bus_i
);

  `declare_bsg_axil_bus_s(1, bsg_axil_mosi_bus_s, bsg_axil_miso_bus_s);
  bsg_axil_mosi_bus_s s_axil_ocl_bus_li_cast, m_axil_bus_lo_cast;
  bsg_axil_miso_bus_s s_axil_ocl_bus_lo_cast, m_axil_bus_li_cast;

  assign s_axil_ocl_bus_li_cast = s_axil_ocl_bus_i;
  assign s_axil_ocl_bus_o       = s_axil_ocl_bus_lo_cast;

// -------------------------------------------------
// AXI-Lite register
// -------------------------------------------------

  (* dont_touch = "true" *) logic axi_reg_rstn;
  lib_pipe #(.WIDTH(1), .STAGES(4)) AXI_REG_RST_N (
    .clk    (clk_i       )
    ,.rst_n  (1'b1        )
    ,.in_bus (resetn_i    )
    ,.out_bus(axi_reg_rstn)
  );

  axi_register_slice_light AXIL_OCL_REG_SLC (
    .aclk         (clk_i                         )
    ,.aresetn      (axi_reg_rstn                  )
    ,.s_axi_awaddr (s_axil_ocl_bus_li_cast.awaddr )
    ,.s_axi_awprot (3'h0                          )
    ,.s_axi_awvalid(s_axil_ocl_bus_li_cast.awvalid)
    ,.s_axi_awready(s_axil_ocl_bus_lo_cast.awready)
    ,.s_axi_wdata  (s_axil_ocl_bus_li_cast.wdata  )
    ,.s_axi_wstrb  (s_axil_ocl_bus_li_cast.wstrb  )
    ,.s_axi_wvalid (s_axil_ocl_bus_li_cast.wvalid )
    ,.s_axi_wready (s_axil_ocl_bus_lo_cast.wready )
    ,.s_axi_bresp  (s_axil_ocl_bus_lo_cast.bresp  )
    ,.s_axi_bvalid (s_axil_ocl_bus_lo_cast.bvalid )
    ,.s_axi_bready (s_axil_ocl_bus_li_cast.bready )
    ,.s_axi_araddr (s_axil_ocl_bus_li_cast.araddr )
    ,.s_axi_arprot (3'h0                          )
    ,.s_axi_arvalid(s_axil_ocl_bus_li_cast.arvalid)
    ,.s_axi_arready(s_axil_ocl_bus_lo_cast.arready)
    ,.s_axi_rdata  (s_axil_ocl_bus_lo_cast.rdata  )
    ,.s_axi_rresp  (s_axil_ocl_bus_lo_cast.rresp  )
    ,.s_axi_rvalid (s_axil_ocl_bus_lo_cast.rvalid )
    ,.s_axi_rready (s_axil_ocl_bus_li_cast.rready )
    ,.m_axi_awaddr (m_axil_bus_lo_cast.awaddr     )
    ,.m_axi_awprot (                              )
    ,.m_axi_awvalid(m_axil_bus_lo_cast.awvalid    )
    ,.m_axi_awready(m_axil_bus_li_cast.awready    )
    ,.m_axi_wdata  (m_axil_bus_lo_cast.wdata      )
    ,.m_axi_wstrb  (m_axil_bus_lo_cast.wstrb      )
    ,.m_axi_wvalid (m_axil_bus_lo_cast.wvalid     )
    ,.m_axi_wready (m_axil_bus_li_cast.wready     )
    ,.m_axi_bresp  (m_axil_bus_li_cast.bresp      )
    ,.m_axi_bvalid (m_axil_bus_li_cast.bvalid     )
    ,.m_axi_bready (m_axil_bus_lo_cast.bready     )
    ,.m_axi_araddr (m_axil_bus_lo_cast.araddr     )
    ,.m_axi_arprot (                              )
    ,.m_axi_arvalid(m_axil_bus_lo_cast.arvalid    )
    ,.m_axi_arready(m_axil_bus_li_cast.arready    )
    ,.m_axi_rdata  (m_axil_bus_li_cast.rdata      )
    ,.m_axi_rresp  (m_axil_bus_li_cast.rresp      )
    ,.m_axi_rvalid (m_axil_bus_li_cast.rvalid     )
    ,.m_axi_rready (m_axil_bus_lo_cast.rready     )
  );


// -----------------------------------------------------------------
// Manycore link
// -----------------------------------------------------------------

  localparam num_endpoint_lp = 1  ;
  localparam fifo_width_lp   = 128;

  localparam num_fifo_pair_lp = num_endpoint_lp*2;

  (* dont_touch = "true" *) logic axi_mcl_rstn;
  lib_pipe #(.WIDTH(1), .STAGES(4)) AXI_MCL_RST_N (
    .clk    (clk_i       ),
    .rst_n  (1'b1        ),
    .in_bus (resetn_i    ),
    .out_bus(axi_mcl_rstn)
  );

  `declare_bsg_manycore_link_sif_s(addr_width_p, data_width_p, x_cord_width_p, y_cord_width_p, load_id_width_p);

  bsg_manycore_link_sif_s loader_link_sif_lo;
  bsg_manycore_link_sif_s loader_link_sif_li;

  logic [1-1:0][x_cord_width_p-1:0] mcl_x_cord_lp = '0;
  logic [1-1:0][y_cord_width_p-1:0] mcl_y_cord_lp = '0;

  bsg_manycore_link_to_axil #(
    .axil_base_addr_p (32'h0000_0000    ),
    .x_cord_width_p   (x_cord_width_p   ),
    .y_cord_width_p   (y_cord_width_p   ),
    .addr_width_p     (addr_width_p     ),
    .data_width_p     (data_width_p     ),
    .max_out_credits_p(max_out_credits_p),
    .load_id_width_p  (load_id_width_p  )
  ) axi_to_mc (
    .clk_i           (clk_i             ),
    .reset_i         (~axi_mcl_rstn     ),
    .s_axil_mcl_bus_i(m_axil_bus_lo_cast),
    .s_axil_mcl_bus_o(m_axil_bus_li_cast),
    .link_sif_i      (loader_link_sif_lo),
    .link_sif_o      (loader_link_sif_li),
    .my_x_i          (mcl_x_cord_lp     ),
    .my_y_i          (mcl_y_cord_lp     )
  );

  // -------------------------------------------------------------------
  // -------------------------------------------------------------------
  //                            MANYCORE
  // -------------------------------------------------------------------
  // -------------------------------------------------------------------

  (* dont_touch = "true" *) logic axi_hb_mc_rstn;
  lib_pipe #(.WIDTH(1), .STAGES(4)) AXI_HB_MC_RST_N (
    .clk    (clk_i       ),
    .rst_n  (1'b1        ),
    .in_bus (resetn_i    ),
    .out_bus(axi_hb_mc_rstn)
  );

  // cache
  bsg_manycore_link_sif_s [num_cache_p-1:0] cache_link_sif_li;
  bsg_manycore_link_sif_s [num_cache_p-1:0] cache_link_sif_lo;

  logic [num_cache_p-1:0][x_cord_width_p-1:0] cache_x_lo;
  logic [num_cache_p-1:0][y_cord_width_p-1:0] cache_y_lo;

  bsg_manycore_wrapper #(
    .addr_width_p         (addr_width_p         )
    ,.data_width_p         (data_width_p         )
    ,.num_tiles_x_p        (num_tiles_x_p        )
    ,.num_tiles_y_p        (num_tiles_y_p        )
    ,.dmem_size_p          (dmem_size_p          )
    ,.icache_entries_p     (icache_entries_p     )
    ,.icache_tag_width_p   (icache_tag_width_p   )
    ,.epa_byte_addr_width_p(epa_byte_addr_width_p)
    ,.dram_ch_addr_width_p (dram_ch_addr_width_p )
    ,.load_id_width_p      (load_id_width_p      )
    ,.num_cache_p          (num_cache_p          )
  ) manycore_wrapper (
    .clk_i            (clk_i             )
    ,.reset_i          (~axi_hb_mc_rstn   )
    ,.cache_link_sif_i (cache_link_sif_li )
    ,.cache_link_sif_o (cache_link_sif_lo )
    ,.cache_x_o        (cache_x_lo        )
    ,.cache_y_o        (cache_y_lo        )
    ,.loader_link_sif_i(loader_link_sif_li)
    ,.loader_link_sif_o(loader_link_sif_lo)
  );

  `declare_bsg_axi_bus_s(1, axi_id_width_p, axi_addr_width_p, axi_data_width_p, bsg_axi_mosi_bus_s, bsg_axi_miso_bus_s);

  // -------------------------------------------------
  // cache module
  // -------------------------------------------------
  bsg_axi_mosi_bus_s m_axi_mc_lo_cast;
  bsg_axi_miso_bus_s m_axi_mc_li_cast;

  bsg_cache_wrapper_axi #(
    .num_cache_p          (num_cache_p          )
    ,.data_width_p         (data_width_p         )
    ,.addr_width_p         (addr_width_p         )
    ,.block_size_in_words_p(block_size_in_words_p)
    ,.sets_p               (sets_p               )
    ,.ways_p               (ways_p               )
    ,.axi_id_width_p       (axi_id_width_p       )
    ,.axi_addr_width_p     (axi_addr_width_p     )
    ,.axi_data_width_p     (axi_data_width_p     )
    ,.axi_burst_len_p      (axi_burst_len_p      )
    ,.x_cord_width_p       (x_cord_width_p       )
    ,.y_cord_width_p       (y_cord_width_p       )
    ,.load_id_width_p      (load_id_width_p      )
  ) cache_wrapper (
    .clk_i        (clk_i                   )
    ,.reset_i      (~axi_hb_mc_rstn         )
    ,.my_x_i       (cache_x_lo              )
    ,.my_y_i       (cache_y_lo              )
    ,.link_sif_i   (cache_link_sif_lo       )
    ,.link_sif_o   (cache_link_sif_li       )

    ,.axi_awid_o   (m_axi_mc_lo_cast.awid   )
    ,.axi_awaddr_o (m_axi_mc_lo_cast.awaddr )
    ,.axi_awlen_o  (m_axi_mc_lo_cast.awlen  )
    ,.axi_awsize_o (m_axi_mc_lo_cast.awsize )
    ,.axi_awburst_o(m_axi_mc_lo_cast.awburst)
    ,.axi_awcache_o(m_axi_mc_lo_cast.awcache)
    ,.axi_awprot_o (m_axi_mc_lo_cast.awprot )
    ,.axi_awlock_o (m_axi_mc_lo_cast.awlock )
    ,.axi_awvalid_o(m_axi_mc_lo_cast.awvalid)
    ,.axi_awready_i(m_axi_mc_li_cast.awready)

    ,.axi_wdata_o  (m_axi_mc_lo_cast.wdata  )
    ,.axi_wstrb_o  (m_axi_mc_lo_cast.wstrb  )
    ,.axi_wlast_o  (m_axi_mc_lo_cast.wlast  )
    ,.axi_wvalid_o (m_axi_mc_lo_cast.wvalid )
    ,.axi_wready_i (m_axi_mc_li_cast.wready )

    ,.axi_bid_i    (m_axi_mc_li_cast.bid    )
    ,.axi_bresp_i  (m_axi_mc_li_cast.bresp  )
    ,.axi_bvalid_i (m_axi_mc_li_cast.bvalid )
    ,.axi_bready_o (m_axi_mc_lo_cast.bready )

    ,.axi_arid_o   (m_axi_mc_lo_cast.arid   )
    ,.axi_araddr_o (m_axi_mc_lo_cast.araddr )
    ,.axi_arlen_o  (m_axi_mc_lo_cast.arlen  )
    ,.axi_arsize_o (m_axi_mc_lo_cast.arsize )
    ,.axi_arburst_o(m_axi_mc_lo_cast.arburst)
    ,.axi_arcache_o(m_axi_mc_lo_cast.arcache)
    ,.axi_arprot_o (m_axi_mc_lo_cast.arprot )
    ,.axi_arlock_o (m_axi_mc_lo_cast.arlock )
    ,.axi_arvalid_o(m_axi_mc_lo_cast.arvalid)
    ,.axi_arready_i(m_axi_mc_li_cast.arready)

    ,.axi_rid_i    (m_axi_mc_li_cast.rid    )
    ,.axi_rdata_i  (m_axi_mc_li_cast.rdata  )
    ,.axi_rresp_i  (m_axi_mc_li_cast.rresp  )
    ,.axi_rlast_i  (m_axi_mc_li_cast.rlast  )
    ,.axi_rvalid_i (m_axi_mc_li_cast.rvalid )
    ,.axi_rready_o (m_axi_mc_lo_cast.rready )
  );

  assign m_axi_mc_lo_cast.awregion = 4'b0;
  assign m_axi_mc_lo_cast.awqos    = 4'b0;

  assign m_axi_mc_lo_cast.arregion = 4'b0;
  assign m_axi_mc_lo_cast.arqos    = 4'b0;
  // -------------------------------------------------------------------
  // -------------------------------------------------------------------
  //                      END OF MANYCORE
  // -------------------------------------------------------------------
  // -------------------------------------------------------------------


  // --------------------------------------------------------------
  // AXI4 PCIS from shell
  // --------------------------------------------------------------

  bsg_axi_mosi_bus_s s_axi_pcis_li_cast;
  bsg_axi_miso_bus_s s_axi_pcis_lo_cast;

  bsg_axi_mosi_bus_s m_axi_ddr_lo_cast;
  bsg_axi_miso_bus_s m_axi_ddr_li_cast;

  assign s_axi_pcis_li_cast = s_axi_pcis_bus_i;
  assign s_axi_pcis_bus_o   = s_axi_pcis_lo_cast;

  (* dont_touch = "true" *) logic axi_mux_rstn;
  lib_pipe #(.WIDTH(1), .STAGES(4)) AXI4_MUX_RST_N (
    .clk    (clk_i        )
    ,.rst_n  (1'b1         )
    ,.in_bus (resetn_i     )
    ,.out_bus(axi_mux_rstn)
  );

  localparam slot_num_lp = 2;

  axi_mux #(
    .slot_num_p  (slot_num_lp     )
    ,.id_width_p  (axi_id_width_p  )
    ,.addr_width_p(axi_addr_width_p)
    ,.data_width_p(axi_data_width_p)
  ) axi_multiplexer (
    .clk_i      (clk_i                                 )
    ,.reset_i    (~axi_mux_rstn                         )
    ,.s_axi_mux_i({m_axi_mc_lo_cast, s_axi_pcis_li_cast})
    ,.s_axi_mux_o({m_axi_mc_li_cast, s_axi_pcis_lo_cast})
    ,.m_axi_bus_o(m_axi_ddr_lo_cast                     )
    ,.m_axi_bus_i(m_axi_ddr_li_cast                     )
  );

  // --------------------------------------------------------------
  // AXI4 DDR to the RAM
  // --------------------------------------------------------------
  assign m_axi_ddr_bus_o   = m_axi_ddr_lo_cast;
  assign m_axi_ddr_li_cast = m_axi_ddr_bus_i;

endmodule // hb_mc_wrapper
