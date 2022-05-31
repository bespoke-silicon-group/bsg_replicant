module bsg_print_stat_snoop
  import bsg_manycore_pkg::*;
  import bsg_manycore_addr_pkg::*;
  import bsg_manycore_profile_pkg::*;
  #(parameter data_width_p="inv"
    , parameter addr_width_p="inv"
    , parameter x_cord_width_p="inv"
    , parameter y_cord_width_p="inv"
    , parameter enable_vcore_profiling_p="inv"

    , parameter link_sif_width_lp=
      `bsg_manycore_link_sif_width(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p)
  )
  (
   input clk_i
   , input reset_i
    // manycore side
   , input [link_sif_width_lp-1:0] loader_link_sif_in_i
   , input [link_sif_width_lp-1:0] loader_link_sif_out_i
   , input [31:0] global_ctr_i

   // snoop signals
   , output logic print_stat_v_o
   , output logic [data_width_p-1:0] print_stat_tag_o
  );

  localparam logfile_lp = "simple_stats.csv";

  `declare_bsg_manycore_link_sif_s(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p);

  bsg_manycore_link_sif_s loader_link_sif_in;
  bsg_manycore_link_sif_s loader_link_sif_out;

  assign loader_link_sif_in = loader_link_sif_in_i;
  assign loader_link_sif_out = loader_link_sif_out_i;


  `declare_bsg_manycore_packet_s(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p);
  bsg_manycore_packet_s fwd_pkt;
  assign fwd_pkt = loader_link_sif_in.fwd.data;


  assign print_stat_v_o = loader_link_sif_in.fwd.v
    & (fwd_pkt.addr == (bsg_print_stat_epa_gp >> 2)) & loader_link_sif_out.fwd.ready_and_rev;
  assign print_stat_tag_o = fwd_pkt.payload.data;

  bsg_manycore_vanilla_core_stat_tag_s print_stat_tag;
  assign print_stat_tag = print_stat_tag_o;

  // Only print simple stats if profiling is disabled, so that exec
  // and profile runs don't stomp on eachother.
  if(enable_vcore_profiling_p == 0) begin
    integer fd;

    always @(negedge reset_i) begin
       fd = $fopen(logfile_lp, "w");
       $fwrite(fd, "time, cycle, y, x, tag\n");
       $fclose(fd);
    end

    always @(negedge clk_i)  begin
      // stat printing
      if (~reset_i & print_stat_v_o) begin
        fd = $fopen(logfile_lp, "a");
        $fwrite(fd, "%0d,%0d,%0d,%0d,%0x\n", $time, global_ctr_i, fwd_pkt.src_y_cord, fwd_pkt.src_x_cord, print_stat_tag);
        $fclose(fd);
      end
    end
  end
endmodule
