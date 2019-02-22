/**
*  bsg_manycore_link_to_2fifo.v
*
*/

`include "bsg_manycore_packet.vh"

module bsg_manycore_link_to_2fifo #(
  addr_width_p = "inv"
  ,data_width_p = "inv"
  ,x_cord_width_p = "inv"
  ,y_cord_width_p = "inv"
  ,load_id_width_p = 11
  ,fifo_width_p = 128
  ,fifo_els_p = 4
  ,max_out_credits_p = 16
  ,link_sif_width_lp=`bsg_manycore_link_sif_width(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p,load_id_width_p)
  ,packet_width_lp = `bsg_manycore_packet_width(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p,load_id_width_p)
  ,return_packet_width_lp = `bsg_manycore_return_packet_width(x_cord_width_p,y_cord_width_p,data_width_p,load_id_width_p)
) (
  input                          clk_i
  ,input                          reset_i
  ,input  [link_sif_width_lp-1:0] link_sif_i
  ,output [link_sif_width_lp-1:0] link_sif_o
  // mcl data to fifo
  ,output [     fifo_width_p-1:0] link_data_o
  ,output                         link_v_o
  ,input                          link_yumi_i
  // mcl credit to fifo
  ,output [     fifo_width_p-1:0] link_returned_o
  ,output                         link_returned_v_o
  ,input                          link_returned_yumi_i
  // fifo data to mcl
  ,input  [     fifo_width_p-1:0] link_data_i
  ,input                          link_v_i
  ,output                         link_ready_o
  // fifo credit to mcl
  ,input  [     fifo_width_p-1:0] link_returning_i
  ,input                          link_returning_v_i
  ,output                         link_returning_ready_o
  // manycore
  ,input  [   x_cord_width_p-1:0] my_x_i
  ,input  [   y_cord_width_p-1:0] my_y_i
);

  logic                         endpoint_v_lo   ;
  logic                         endpoint_yumi_li;
  logic [     data_width_p-1:0] endpoint_data_lo;
  logic [(data_width_p>>3)-1:0] endpoint_mask_lo;
  logic [     addr_width_p-1:0] endpoint_addr_lo;
  logic                         endpoint_we_lo  ;
  logic [   x_cord_width_p-1:0] in_src_x_cord_lo;
  logic [   y_cord_width_p-1:0] in_src_y_cord_lo;

  logic                       endpoint_out_v_li     ;
  logic [packet_width_lp-1:0] endpoint_out_packet_li;
  logic                       endpoint_out_ready_lo ;

  logic [   data_width_p-1:0] returned_data_r_lo   ;
  logic [load_id_width_p-1:0] returned_load_id_r_lo;
  logic                       returned_v_r_lo      ;
  logic                       returned_fifo_full_lo;
  logic                       returned_yumi_li     ;

  logic [return_packet_width_lp-1:0] returning_data_li;
  logic                              returning_v_li   ;

  logic [$clog2(max_out_credits_p+1)-1:0] out_credits_lo;

  // instantiate endpoint_standards.
  // last one maps to tag_mem.
  //
  bsg_manycore_endpoint_standard #(
    .x_cord_width_p   (x_cord_width_p   )
    ,.y_cord_width_p   (y_cord_width_p   )
    ,.fifo_els_p       (fifo_els_p       )
    ,.data_width_p     (data_width_p     )
    ,.addr_width_p     (addr_width_p     )
    ,.max_out_credits_p(max_out_credits_p)
    ,.load_id_width_p  (load_id_width_p  )
  ) dram_endpoint_standard (
    .clk_i               (clk_i                 )
    ,.reset_i             (reset_i               )

    ,.link_sif_i          (link_sif_i            )
    ,.link_sif_o          (link_sif_o            )

    // manycore packet -> fifo
    ,.in_v_o              (endpoint_v_lo         )
    ,.in_yumi_i           (endpoint_yumi_li      )
    ,.in_data_o           (endpoint_data_lo      )
    ,.in_mask_o           (endpoint_mask_lo      )
    ,.in_addr_o           (endpoint_addr_lo      )
    ,.in_we_o             (endpoint_we_lo        )
    ,.in_src_x_cord_o     (in_src_x_cord_lo      )
    ,.in_src_y_cord_o     (in_src_y_cord_lo      )
    
    // fifo -> manycore packet
    ,.out_v_i             (endpoint_out_v_li     )
    ,.out_packet_i        (endpoint_out_packet_li)
    ,.out_ready_o         (endpoint_out_ready_lo )

    // manycore credit -> fifo
    ,.returned_data_r_o   (returned_data_r_lo    )
    ,.returned_load_id_r_o(returned_load_id_r_lo )
    ,.returned_v_r_o      (returned_v_r_lo       )
    // always 1'b1 if returned_fifo_p is not set
    ,.returned_fifo_full_o(returned_fifo_full_lo )
    ,.returned_yumi_i     (returned_yumi_li      )

    // fifo -> manycore credit
    ,.returning_data_i    (returning_data_li     )
    ,.returning_v_i       (returning_v_li        )

    ,.out_credits_o       (out_credits_lo        )
    ,.my_x_i              (my_x_i                )
    ,.my_y_i              (my_y_i                )
  );

// fifo -> manycore packet
  assign endpoint_out_v_li      = link_v_i;
  assign endpoint_out_packet_li = link_data_i[0+:packet_width_lp];
  assign link_ready_o           = endpoint_out_ready_lo;


// manycore packet -> fifo
  assign link_v_o         = endpoint_v_lo;
  assign endpoint_yumi_li = link_yumi_i;
  assign link_data_o      = (fifo_width_p)'({
      out_credits_lo // additoinal $clog2(max_out_credits_p+1) bits
      ,endpoint_addr_lo
      ,{1'b0, endpoint_we_lo} // only support remote load(0)/store(1)
      ,endpoint_mask_lo
      ,endpoint_data_lo
      ,in_src_y_cord_lo
      ,in_src_x_cord_lo
      ,my_y_i
      ,my_x_i
    });


// manycore return credit -> fifo
  assign link_returned_o = (fifo_width_p)'({
      returned_fifo_full_lo // additional 1 bit
      ,`ePacketType_data
      ,returned_data_r_lo
      ,returned_load_id_r_lo
      ,my_y_i
      ,my_x_i
    });
  assign link_returned_v_o = returned_v_r_lo;
  assign returned_yumi_li  = link_returned_yumi_i;


// fifo return credit -> manycore
  assign returning_data_li      = link_returning_i[0+:return_packet_width_lp];
  assign returning_v_li         = link_returning_v_i;
  assign link_returning_ready_o = 1'b1;


// synopsys translate_off
  always_ff @(negedge clk_i)
    begin
      assert (fifo_width_p >= return_packet_width_lp + $clog2(max_out_credits_p+1))
        else begin
          $error("## errant fifo width %d is less than the manycore returned packet width %d", fifo_width_p, return_packet_width_lp);
          $finish();
        end
      assert (fifo_width_p >= (packet_width_lp + 1))
        else begin
          $error("## errant fifo width %d is less than the manycore packet width %d", fifo_width_p, packet_width_lp);
          $finish();
        end
    end
// synopsys translate_on

endmodule
