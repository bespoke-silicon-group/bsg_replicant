/**
*  bsg_manycore_endpoint_to_fifos.v
*
*  fifo <-> manycore_link_endpoint (CL)
*/

`include "bsg_manycore_packet.vh"
`include "axil_to_mcl.vh"

module bsg_manycore_endpoint_to_fifos #(
  parameter num_endpoint_p = "inv"
  , localparam fifo_width_lp = 128
  // these are endpoint parameters
  , parameter x_cord_width_p="inv"
  , parameter y_cord_width_p="inv"
  , parameter fifo_els_p = 4
  , parameter addr_width_p = "inv"
  , parameter data_width_p = "inv"
  , parameter max_out_credits_p = "inv"
  , parameter load_id_width_p = "inv"
  , localparam credits_width_lp = $clog2(max_out_credits_p+1)
  , localparam link_sif_width_lp = `bsg_manycore_link_sif_width(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p,load_id_width_p)
  , localparam packet_width_lp = `bsg_manycore_packet_width(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p,load_id_width_p)
  , localparam return_packet_width_lp =`bsg_manycore_return_packet_width(x_cord_width_p,y_cord_width_p,data_width_p,load_id_width_p)
) (
  input                                                clk_i
  ,input                                                reset_i
  // fifo to endpoint
  ,input  [num_endpoint_p*2-1:0]                        fifo_v_i
  ,input  [num_endpoint_p*2-1:0][    fifo_width_lp-1:0] fifo_data_i
  ,output [num_endpoint_p*2-1:0]                        fifo_rdy_o
  // endpoint to fifo
  ,output [num_endpoint_p*2-1:0]                        fifo_v_o
  ,output [num_endpoint_p*2-1:0][    fifo_width_lp-1:0] fifo_data_o
  ,input  [num_endpoint_p*2-1:0]                        fifo_rdy_i
  ,input  [  num_endpoint_p-1:0][link_sif_width_lp-1:0] link_sif_i
  ,output [  num_endpoint_p-1:0][link_sif_width_lp-1:0] link_sif_o
  ,input  [  num_endpoint_p-1:0][   x_cord_width_p-1:0] my_x_i
  ,input  [  num_endpoint_p-1:0][   y_cord_width_p-1:0] my_y_i
  ,output [  num_endpoint_p-1:0][ credits_width_lp-1:0] out_credits_o
);

  `declare_bsg_mcl_request_s;
  `declare_bsg_mcl_response_s;
  bsg_mcl_request_s  [num_endpoint_p-1:0] fifo_req_li, mc_req_lo;
  bsg_mcl_response_s [num_endpoint_p-1:0] fifo_rsp_li, mc_rsp_lo;

  logic [num_endpoint_p-1:0] fifo_req_v_li, fifo_req_rdy_lo;
  logic [num_endpoint_p-1:0] fifo_rsp_v_li, fifo_rsp_rdy_lo;
  logic [num_endpoint_p-1:0] mc_req_v_lo, mc_req_rdy_li;
  logic [num_endpoint_p-1:0] mc_rsp_v_lo, mc_rsp_rdy_li;

  for (genvar i=0; i<num_endpoint_p; i=i+1) begin
    // fifo request to manycore
    assign fifo_req_v_li[i] = fifo_v_i[2*i];
    assign fifo_req_li[i]  = fifo_data_i[2*i];
    assign fifo_rdy_o[2*i] = fifo_req_rdy_lo[i];

    // fifo response to manycore
    assign fifo_rsp_v_li[i]  = fifo_v_i[2*i+1];
    assign fifo_rsp_li[i]    = fifo_data_i[2*i+1];
    assign fifo_rdy_o[2*i+1] = fifo_rsp_rdy_lo[i];

    // manycore response to fifo
    assign fifo_v_o[2*i]    = mc_rsp_v_lo[i];
    assign fifo_data_o[2*i] = mc_rsp_lo[i];
    assign mc_rsp_rdy_li[i] = fifo_rdy_i[2*i];

    // manycore request to fifo
    assign fifo_v_o[2*i+1]    = mc_req_v_lo[i];
    assign fifo_data_o[2*i+1] = mc_req_lo[i];
    assign mc_req_rdy_li[i]   = fifo_rdy_i[2*i+1];
  end

  logic [num_endpoint_p-1:0]                        endpoint_in_v_lo   ;
  logic [num_endpoint_p-1:0]                        endpoint_in_yumi_li;
  logic [num_endpoint_p-1:0][     data_width_p-1:0] endpoint_in_data_lo;
  logic [num_endpoint_p-1:0][(data_width_p>>3)-1:0] endpoint_in_mask_lo;
  logic [num_endpoint_p-1:0][     addr_width_p-1:0] endpoint_in_addr_lo;
  logic [num_endpoint_p-1:0]                        endpoint_in_we_lo  ;
  logic [num_endpoint_p-1:0][   x_cord_width_p-1:0] in_src_x_cord_lo   ;
  logic [num_endpoint_p-1:0][   y_cord_width_p-1:0] in_src_y_cord_lo   ;

  logic [num_endpoint_p-1:0]                      endpoint_out_v_li     ;
  logic [num_endpoint_p-1:0][packet_width_lp-1:0] endpoint_out_packet_li;
  logic [num_endpoint_p-1:0]                      endpoint_out_ready_lo ;

  logic [num_endpoint_p-1:0][   data_width_p-1:0] returned_data_r_lo   ;
  logic [num_endpoint_p-1:0][load_id_width_p-1:0] returned_load_id_r_lo;
  logic [num_endpoint_p-1:0]                      returned_v_r_lo      ;
  logic [num_endpoint_p-1:0]                      returned_fifo_full_lo;
  logic [num_endpoint_p-1:0]                      returned_yumi_li     ;

  logic [num_endpoint_p-1:0][data_width_p-1:0] returning_data_li;
  logic [num_endpoint_p-1:0]                   returning_v_li   ;

  logic [num_endpoint_p-1:0][credits_width_lp-1:0] out_credits_lo;
  logic [num_endpoint_p-1:0][  x_cord_width_p-1:0] my_x_li       ;
  logic [num_endpoint_p-1:0][  y_cord_width_p-1:0] my_y_li       ;



  // manycore request to fifo
  logic [num_endpoint_p-1:0] timer_v_lo;
  logic [num_endpoint_p-1:0] timer_rdy_li;
  logic [num_endpoint_p-1:0] [data_width_p-1:0] timer_data_lo;
  logic [num_endpoint_p-1:0] [(data_width_p>>3)-1:0] timer_mask_lo;
  logic [num_endpoint_p-1:0] [addr_width_p-1:0] timer_addr_lo;
  logic [num_endpoint_p-1:0] timer_we_lo;
  logic [num_endpoint_p-1:0] [x_cord_width_p-1:0] timer_src_x_cord_lo;
  logic [num_endpoint_p-1:0] [y_cord_width_p-1:0] timer_src_y_cord_lo;

  assign mc_req_v_lo = timer_v_lo;
  assign timer_rdy_li = mc_req_rdy_li;
  for (genvar i=0; i<num_endpoint_p; i=i+1) begin
    assign mc_req_lo[i].padding = '0;
    assign mc_req_lo[i].addr = (32)'(timer_addr_lo[i]);
    assign mc_req_lo[i].op = (8)'(timer_we_lo[i]);
    assign mc_req_lo[i].op_ex = (8)'(timer_mask_lo[i]);
    assign mc_req_lo[i].payload.data = (32)'(timer_data_lo[i]);
    assign mc_req_lo[i].src_y_cord = (8)'(timer_src_x_cord_lo[i]);
    assign mc_req_lo[i].src_x_cord = (8)'(timer_src_y_cord_lo[i]);
    assign mc_req_lo[i].y_cord = (8)'(my_y_li[i]);
    assign mc_req_lo[i].x_cord = (8)'(my_x_li[i]);

    bsg_manycore_endpoint_request_timer #(
      .x_cord_width_p(x_cord_width_p),
      .y_cord_width_p(y_cord_width_p),
      .addr_width_p  (addr_width_p  ),
      .data_width_p  (data_width_p  )
    ) endpoint_timer (
      .clk_i       (clk_i                 ),
      .reset_i     (reset_i               ),
      .v_i         (endpoint_in_v_lo[i]   ),
      .yumi_o      (endpoint_in_yumi_li[i]),
      .data_i      (endpoint_in_data_lo[i]),
      .mask_i      (endpoint_in_mask_lo[i]),
      .addr_i      (endpoint_in_addr_lo[i]),
      .we_i        (endpoint_in_we_lo[i]  ),
      .src_x_cord_i(in_src_x_cord_lo[i]   ),
      .src_y_cord_i(in_src_y_cord_lo[i]   ),
      .v_o         (timer_v_lo[i]         ),
      .rdy_i       (timer_rdy_li[i]       ),
      .data_o      (timer_data_lo[i]      ),
      .mask_o      (timer_mask_lo[i]      ),
      .addr_o      (timer_addr_lo[i]      ),
      .we_o        (timer_we_lo[i]        ),
      .src_x_cord_o(timer_src_x_cord_lo[i]),
      .src_y_cord_o(timer_src_y_cord_lo[i])
    );
  end


  // fifo request to manycore
  assign endpoint_out_v_li = fifo_req_v_li;
  for (genvar i=0; i<num_endpoint_p; i=i+1) begin
    assign endpoint_out_packet_li[i] = {
      (addr_width_p)'(fifo_req_li[i].addr)
      ,(request_op_width_p)'(fifo_req_li[i].op)
      ,(data_width_p>>3)'(fifo_req_li[i].op_ex)
      ,(data_width_p)'(fifo_req_li[i].payload.data)
      ,(y_cord_width_p)'(fifo_req_li[i].src_y_cord)
      ,(x_cord_width_p)'(fifo_req_li[i].src_x_cord)
      ,(y_cord_width_p)'(fifo_req_li[i].y_cord)
      ,(x_cord_width_p)'(fifo_req_li[i].x_cord)
    };
		assign fifo_req_rdy_lo[i] = endpoint_out_ready_lo[i] & (out_credits_lo[i] >= '0);
  end

  // manycore response to fifo
  assign mc_rsp_v_lo = returned_v_r_lo;
  for (genvar i=0; i<num_endpoint_p; i=i+1) begin
    assign mc_rsp_lo[i].padding = '0;
		assign mc_rsp_lo[i].pkt_type = (8)'(`ePacketType_data);
    assign mc_rsp_lo[i].data = (32)'(returned_data_r_lo[i]);
    assign mc_rsp_lo[i].load_id = (32)'(returned_load_id_r_lo[i]);
    assign mc_rsp_lo[i].y_cord = (8)'(my_y_li[i]);
    assign mc_rsp_lo[i].x_cord = (8)'(my_x_li[i]);
  end
  assign returned_yumi_li = mc_rsp_rdy_li & mc_rsp_v_lo;

  // fifo response to manycore
  logic [num_endpoint_p-1:0] returning_wr_v_r;
  always_ff @(posedge clk_i)
    begin
      if(reset_i)
        returning_wr_v_r <= '0;
      else
        returning_wr_v_r <= endpoint_in_v_lo & endpoint_in_we_lo;
    end

  assign fifo_rsp_rdy_lo = ~returning_wr_v_r;
  for (genvar i=0; i<num_endpoint_p; i=i+1) begin
    assign returning_data_li[i] = returning_wr_v_r[i] ? '0 : {
      (`return_packet_type_width)'(fifo_rsp_li[i].pkt_type)
      ,(data_width_p)'(fifo_rsp_li[i].data)
      ,(load_id_width_p)'(fifo_rsp_li[i].load_id)
      ,(y_cord_width_p)'(fifo_rsp_li[i].y_cord)
      ,(x_cord_width_p)'(fifo_rsp_li[i].x_cord)
    };
  end
  assign returning_v_li = returning_wr_v_r | (fifo_rsp_v_li & fifo_rsp_rdy_lo);

  assign out_credits_o = out_credits_lo;
  assign my_x_li       = my_x_i;
  assign my_y_li       = my_y_i;

  for (genvar i=0; i<num_endpoint_p; i=i+1) begin
    bsg_manycore_endpoint_standard #(
      .x_cord_width_p   (x_cord_width_p   )
      ,.y_cord_width_p   (y_cord_width_p   )
      ,.fifo_els_p       (fifo_els_p       )
      ,.addr_width_p     (addr_width_p     )
      ,.data_width_p     (data_width_p     )
      ,.max_out_credits_p(max_out_credits_p)
      ,.load_id_width_p  (load_id_width_p  )
    ) mcl_endpoint_standard (
      .clk_i               (clk_i                    )
      ,.reset_i             (reset_i                  )

      ,.link_sif_i          (link_sif_i               )
      ,.link_sif_o          (link_sif_o               )

      // manycore packet -> fifo
      ,.in_v_o              (endpoint_in_v_lo[i]      )
      ,.in_yumi_i           (endpoint_in_yumi_li[i]   )
      ,.in_data_o           (endpoint_in_data_lo[i]   )
      ,.in_mask_o           (endpoint_in_mask_lo[i]   )
      ,.in_addr_o           (endpoint_in_addr_lo[i]   )
      ,.in_we_o             (endpoint_in_we_lo[i]     )
      ,.in_src_x_cord_o     (in_src_x_cord_lo[i]      )
      ,.in_src_y_cord_o     (in_src_y_cord_lo[i]      )

      // fifo -> manycore packet
      ,.out_v_i             (endpoint_out_v_li[i]     )
      ,.out_packet_i        (endpoint_out_packet_li[i])
      ,.out_ready_o         (endpoint_out_ready_lo[i] )

      // manycore credit -> fifo
      ,.returned_data_r_o   (returned_data_r_lo[i]    )
      ,.returned_load_id_r_o(returned_load_id_r_lo[i] )
      ,.returned_v_r_o      (returned_v_r_lo[i]       )
      ,.returned_fifo_full_o(returned_fifo_full_lo[i] )
      // always 1'b1 if returned_fifo_p is not set
      ,.returned_yumi_i     (returned_yumi_li[i]      )

      // fifo -> manycore credit
      ,.returning_data_i    (returning_data_li[i]     )
      ,.returning_v_i       (returning_v_li[i]        )

      ,.out_credits_o       (out_credits_lo[i]        )
      ,.my_x_i              (my_x_li[i]               )
      ,.my_y_i              (my_y_li[i]               )
    );
  end

endmodule
