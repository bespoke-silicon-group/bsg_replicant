/**
 *  bsg_manycore_wrapper_crossbar.v
 */

module bsg_manycore_wrapper_crossbar
  import bsg_manycore_pkg::*;
  import bsg_noc_pkg::*;
  #(parameter addr_width_p="inv" // in words
    , parameter data_width_p="inv"

    , parameter num_tiles_x_p="inv"
    , parameter num_tiles_y_p="inv"

    , parameter dmem_size_p="inv"
    , parameter icache_entries_p="inv"
    , parameter icache_tag_width_p="inv"
    , parameter vcache_size_p="inv"
    , parameter vcache_block_size_in_words_p="inv"
    , parameter vcache_sets_p="inv"

    , parameter num_cache_p="inv"
 
    , parameter x_cord_width_lp=`BSG_SAFE_CLOG2(num_tiles_x_p)
    , parameter y_cord_width_lp=`BSG_SAFE_CLOG2(num_tiles_y_p+3)
  
    , parameter link_sif_width_lp=
      `bsg_manycore_link_sif_width(addr_width_p,data_width_p,x_cord_width_lp,y_cord_width_lp)
  )
  (
    input clk_i
    , input reset_i

    , input [num_cache_p-1:0][link_sif_width_lp-1:0] cache_link_sif_i
    , output logic [num_cache_p-1:0][link_sif_width_lp-1:0] cache_link_sif_o

    , output logic [num_cache_p-1:0][x_cord_width_lp-1:0] cache_x_o
    , output logic [num_cache_p-1:0][y_cord_width_lp-1:0] cache_y_o

    , input [link_sif_width_lp-1:0] loader_link_sif_i
    , output logic [link_sif_width_lp-1:0] loader_link_sif_o
  );

  // manycore
  //
  `declare_bsg_manycore_link_sif_s(addr_width_p,data_width_p,x_cord_width_lp,y_cord_width_lp);

  bsg_manycore_link_sif_s [S:N][num_tiles_x_p-1:0] ver_link_sif_li;
  bsg_manycore_link_sif_s [S:N][num_tiles_x_p-1:0] ver_link_sif_lo;
  bsg_manycore_link_sif_s [num_tiles_x_p-1:0] io_link_sif_li;
  bsg_manycore_link_sif_s [num_tiles_x_p-1:0] io_link_sif_lo;
  
    bsg_manycore_top_crossbar #(
      .dmem_size_p(dmem_size_p)
      ,.icache_entries_p(icache_entries_p)
      ,.icache_tag_width_p(icache_tag_width_p)
      ,.vcache_size_p(vcache_size_p)
      ,.vcache_block_size_in_words_p(vcache_block_size_in_words_p)
      ,.vcache_sets_p(vcache_sets_p)
      ,.data_width_p(data_width_p)
      ,.addr_width_p(addr_width_p)
      ,.num_tiles_x_p(num_tiles_x_p)
      ,.num_tiles_y_p(num_tiles_y_p+1)
    ) manycore (
      .clk_i(clk_i)
      ,.reset_i(reset_i)

      ,.ver_link_sif_i(ver_link_sif_li)
      ,.ver_link_sif_o(ver_link_sif_lo)

      ,.io_link_sif_i(io_link_sif_li)
      ,.io_link_sif_o(io_link_sif_lo)
    );

  // connecting link_sif to outside
  //
  //  north[0]  : victim cache 0
  //  north[1]  : victim cache 1
  //  ...
  //  x[0].y[1] : host interface
  //  x[1].y[1] : unused
  //  ...

  //  south[0] : victim cache X
  //  south[1] : victim cache X+1
  //  ...
  //
  for (genvar i = 0; i < num_tiles_x_p; i++) begin
    assign cache_link_sif_o[i] = ver_link_sif_lo[N][i];
    assign ver_link_sif_li[N][i] = cache_link_sif_i[i];
    assign cache_x_o[i] = (x_cord_width_lp)'(i);
    assign cache_y_o[i] = (y_cord_width_lp)'(0);
  end

  for (genvar i = 0; i < num_tiles_x_p; i++) begin
    assign cache_link_sif_o[num_tiles_x_p+i] = ver_link_sif_lo[S][i];
    assign ver_link_sif_li[S][i] = cache_link_sif_i[num_tiles_x_p+i];
    assign cache_x_o[num_tiles_x_p+i] = (x_cord_width_lp)'(i);
    assign cache_y_o[num_tiles_x_p+i] = (y_cord_width_lp)'(num_tiles_y_p+2);
  end

  // 0,1 for host io
  //
  assign loader_link_sif_o = io_link_sif_lo[0];
  assign io_link_sif_li[0] = loader_link_sif_i;


  // tie-off
  //
  for (genvar i = 1; i < num_tiles_x_p; i++) begin
    bsg_manycore_link_sif_tieoff #(
      .addr_width_p(addr_width_p)
      ,.data_width_p(data_width_p)
      ,.x_cord_width_p(x_cord_width_lp)
      ,.y_cord_width_p(y_cord_width_lp)
    ) tieoff_io (
      .clk_i(clk_i)
      ,.reset_i(reset_i)
      ,.link_sif_i(io_link_sif_lo[i])
      ,.link_sif_o(io_link_sif_li[i])
    );
  end

endmodule
