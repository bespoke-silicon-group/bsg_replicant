// this module just loopback the received packets

module cl_simple_loopback #(
  parameter data_width_p = "inv"
  ,parameter mask_p="inv"
) (
  input                     clk_i
  ,input                     en_i
  ,input                     reset_i
  ,input                     v_i
  ,input  [data_width_p-1:0] data_i
  ,output                    ready_o
  ,output                    v_o
  ,output [data_width_p-1:0] data_o
  ,input                     yumi_i
);


  localparam debug_lp = 1;

  // synopsys translate_off
  if (debug_lp)
    begin
      always @(negedge clk_i)
        if (v_i & ready_o)
          $display("## cl_simple_loopback received %x",data_i);

        always @(negedge clk_i)
        if (v_o & yumi_i)
          $display("## cl_simple_loopback sent %x",data_o);
      end
  // synopsys translate_on

  wire                    in_fifo_v   ;
  wire [data_width_p-1:0] in_fifo_data;
  wire                    in_fifo_yumi;

  wire                    out_fifo_ready;
  logic [data_width_p-1:0] out_fifo_data ;
  wire                    out_fifo_v    ;

  bsg_two_fifo #(.width_p(data_width_p)) fifo_in (
    .clk_i  (clk_i       )
    ,.reset_i(reset_i     )
    ,.ready_o(ready_o     )
    ,.v_i    (v_i         )
    ,.data_i (data_i      )
    ,.v_o    (in_fifo_v   )
    ,.data_o (in_fifo_data)
    ,.yumi_i (in_fifo_yumi)
  );

  // client: a loopback device
  always_comb
    begin
      out_fifo_data = in_fifo_data & (mask_p);
    end

  /* begin your code here */

  // en_i is not really necessary
  // but we do it to prevent unused input
  assign out_fifo_v   = in_fifo_v & en_i;
  assign in_fifo_yumi = out_fifo_v & out_fifo_ready;


  /* end your code here */

  bsg_two_fifo #(.width_p(data_width_p)) fifo_out (
    .clk_i  (clk_i         )
    ,.reset_i(reset_i       )
    ,.ready_o(out_fifo_ready)
    ,.v_i    (out_fifo_v    )
    ,.data_i (out_fifo_data )
    ,.v_o    (v_o           )
    ,.data_o (data_o        )
    ,.yumi_i (yumi_i        )
  );

endmodule