/**
*  bsg_manycore_endpoint_request_timer.v
*
*  discern the bsg_print_time() request packet and forward to the host with counter value (in 2 cycles)
*  bypass other request type
*/

module bsg_manycore_endpoint_request_timer #(
  parameter x_cord_width_p = "inv"
  , parameter y_cord_width_p="inv"
  , parameter addr_width_p = "inv"
  , parameter data_width_p = "inv"
  , parameter timer_addr_p = 16'h3AB5
  , localparam timer_wdith_lp = 64
) (
  input                          clk_i
  ,input                          reset_i
  // from manycore
  ,input                          v_i
  ,output                         yumi_o
  ,input  [     data_width_p-1:0] data_i
  ,input  [(data_width_p>>3)-1:0] mask_i
  ,input  [     addr_width_p-1:0] addr_i
  ,input                          we_i
  ,input  [   x_cord_width_p-1:0] src_x_cord_i
  ,input  [   y_cord_width_p-1:0] src_y_cord_i
  // to host
  ,output                         v_o
  ,input                          rdy_i
  ,output [     data_width_p-1:0] data_o
  ,output [(data_width_p>>3)-1:0] mask_o
  ,output [     addr_width_p-1:0] addr_o
  ,output                         we_o
  ,output [   x_cord_width_p-1:0] src_x_cord_o
  ,output [   y_cord_width_p-1:0] src_y_cord_o
);


  logic [timer_wdith_lp-1:0] timer_cnt_lo;
  bsg_cycle_counter #(.width_p(timer_wdith_lp), .init_val_p(0)) timer_counter (
    .clk_i  (clk_i       ),
    .reset_i(reset_i     ),
    .ctr_r_o(timer_cnt_lo)
  );

  typedef enum logic[1:0] {
    E_EMPTY = 0,
    E_BYPASS = 1,
    E_BUBBLE = 2
  } pip_state_e;

  pip_state_e state_r, state_n;

  always_ff @(posedge clk_i) begin
    if (reset_i)
      state_r <= E_EMPTY;
    else
      state_r <= state_n;
  end

  wire is_timer_req = (addr_i == timer_addr_p) && (we_i == 1'b1);

  logic bubble_r, resume;

  always_ff @(posedge clk_i) begin
    if (reset_i || resume)
      bubble_r <= 1'b0;
    else if ((state_r == E_BUBBLE) && (rdy_i & v_o))
      bubble_r <= 1'b1;
  end

  assign resume = bubble_r & rdy_i & v_o;

  always_comb begin : state_ctrl

    state_n = state_r;
    case (state_r)

      E_EMPTY :
        if (is_timer_req & yumi_o)
          state_n = E_BUBBLE;
        else if (~is_timer_req & yumi_o)
          state_n = E_BYPASS;

      E_BYPASS :
        if (is_timer_req & yumi_o)
          state_n = E_BUBBLE;
        else if (~yumi_o & rdy_i & v_o)
          state_n = E_EMPTY;

      E_BUBBLE :
        if (resume)
          state_n = E_EMPTY;

    endcase // state_r

  end

  // ------------------------------
  // timer
  // ------------------------------

  logic [timer_wdith_lp-1:0] timer_cnt_r;

  bsg_dff_en #(.width_p(timer_wdith_lp)) timer_cnt_dff (
    .clk_i (clk_i       ),
    .data_i(timer_cnt_lo),
    .en_i  (v_i         ),
    .data_o(timer_cnt_r )
  );

  // ------------------------------
  // data path
  // ------------------------------

  // to endpoint
  assign yumi_o = v_i & ((state_r == E_EMPTY) || (rdy_i & v_o & (state_r == E_BYPASS)));

  // to host
  logic [     data_width_p-1:0] data_r      ;
  logic [(data_width_p>>3)-1:0] mask_r      ;
  logic [     addr_width_p-1:0] addr_r      ;
  logic                         we_r        ;
  logic [   x_cord_width_p-1:0] src_x_cord_r;
  logic [   y_cord_width_p-1:0] src_y_cord_r;


  always_ff @(posedge clk_i)
    if (yumi_o) begin
      data_r <= data_i;
      mask_r <= mask_i;
      addr_r <= addr_i;
      we_r   <= we_i;
      src_x_cord_r <= src_x_cord_i;
      src_y_cord_r <= src_y_cord_i;
    end

  assign data_o = (state_r == E_BUBBLE) ? (bubble_r ? data_width_p'(timer_cnt_r[timer_wdith_lp-1:32])
                                                  : data_width_p'(timer_cnt_r[31:0]))
                                                  : data_r;
  assign mask_o = (state_r == E_BUBBLE) ? (data_width_p>>3)'(-1) : mask_r;
  assign addr_o = addr_r;
  assign we_o = we_r;
  assign src_x_cord_o = src_x_cord_r;
  assign src_y_cord_o = src_y_cord_r;

  assign v_o = rdy_i & (state_r != E_EMPTY);


  // synopsys translate_off
  initial begin
    assert (timer_wdith_lp >= 32)
      else begin
        $error("## timer_wdith_lp must be no less than 32 !");
        $finish();
      end
    assert (data_width_p >= 32)
      else begin
        $error("## manycore endpoint parameter data_width_p must be no less than 32 !");
        $finish();
      end
  end
  // synopsys translate_on

endmodule
