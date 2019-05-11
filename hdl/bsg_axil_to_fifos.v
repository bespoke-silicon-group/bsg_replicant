/*
* bsg_axil_to_fifos.v
*
* adapt axil interface to parameterized fifo interface
*
* FIFO pair configuration base address = n * 0x100
* Register offsets:
* TX_VACANCY=0xC      TX_DATA=0x10    TX_LENGTH=0x14 (store and forward mode)
* RX_OCCUPANCY=0x1C   RX_DATA=0x20    RX_LENGTH=0x24 (store and forward mode)
*
* if write to a full fifo, data will not be updated
* if read from a empty fifo, stale data will be readout
* The host should checkout the status registers before issue a AXI-Lite transaction
*
*/

`include "bsg_axi_bus_pkg.vh"
`include "bsg_defines.v"

module bsg_axil_to_fifos #(
  parameter num_2fifos_p = "inv"
  , parameter fifo_els_p = "inv"
  , localparam fifo_width_lp = 32
  , localparam fifo_ptr_width_lp = `BSG_WIDTH(fifo_els_p)
  , localparam axil_base_addr_start_lp = 32'h0000_0000
  , localparam axil_mosi_bus_width_lp = `bsg_axil_mosi_bus_width(1)
  , localparam axil_miso_bus_width_lp = `bsg_axil_miso_bus_width(1)
  , localparam config_addr_width_lp = 8
  , localparam index_addr_width_lp = (32-config_addr_width_lp)
  , localparam tx_vacancy_lp = 8'hC
  , localparam tx_data_lp = 8'h10
  , localparam tx_length_lp = 8'h14
  , localparam rx_occupancy_lp = 8'h1C
  , localparam rx_data_lp = 8'h20
  , localparam rx_length_lp = 8'h24
) (
  input                                                  clk_i
  ,
  input                                                  reset_i
  ,
  input  [axil_mosi_bus_width_lp-1:0]                    s_axil_bus_i
  ,
  output [axil_miso_bus_width_lp-1:0]                    s_axil_bus_o
  ,
  input  [          num_2fifos_p-1:0]                    fifo_v_i
  ,
  input  [          num_2fifos_p-1:0][fifo_width_lp-1:0] fifo_data_i
  ,
  output [          num_2fifos_p-1:0]                    fifo_rdy_o
  ,
  output [          num_2fifos_p-1:0]                    fifo_v_o
  ,
  output [          num_2fifos_p-1:0][fifo_width_lp-1:0] fifo_data_o
  ,
  input  [          num_2fifos_p-1:0]                    fifo_rdy_i
);

  // synopsys translate_off
  initial begin
    assert (fifo_width_lp%8 == 0)
      else begin
        $error("## axis fifo width should be multiple of 8!");
        $finish();
      end
  end
  // synopsys translate_on

  `declare_bsg_axil_bus_s(1, bsg_axil_mosi_bus_s, bsg_axil_miso_bus_s);
  bsg_axil_mosi_bus_s s_axil_bus_i_cast;
  bsg_axil_miso_bus_s s_axil_bus_o_cast;

  assign s_axil_bus_i_cast = s_axil_bus_i;
  assign s_axil_bus_o      = s_axil_bus_o_cast;


  logic [31:0] awaddr_li ;
  logic        awvalid_li;
  logic        awready_lo;
  logic [31:0] wdata_li  ;
  logic [ 3:0] wstrb_li  ;
  logic        wvalid_li ;
  logic        wready_lo ;

  logic [1:0] bresp_lo ;
  logic       bvalid_lo;
  logic       bready_li;

  logic [31:0] araddr_li ;
  logic        arvalid_li;
  logic        arready_lo;
  logic [31:0] rdata_lo  ;
  logic [ 1:0] rresp_lo  ;
  logic        rvalid_lo ;
  logic        rready_li ;

  assign awaddr_li                 = s_axil_bus_i_cast.awaddr;
  assign awvalid_li                = s_axil_bus_i_cast.awvalid;
  assign s_axil_bus_o_cast.awready = awready_lo;
  assign wdata_li                  = s_axil_bus_i_cast.wdata;
  assign wstrb_li                  = s_axil_bus_i_cast.wstrb;
  assign wvalid_li                 = s_axil_bus_i_cast.wvalid;
  assign s_axil_bus_o_cast.wready  = wready_lo;

  assign s_axil_bus_o_cast.bresp  = bresp_lo;
  assign s_axil_bus_o_cast.bvalid = bvalid_lo;
  assign bready_li                = s_axil_bus_i_cast.bready;

  assign araddr_li                 = s_axil_bus_i_cast.araddr;
  assign arvalid_li                = s_axil_bus_i_cast.arvalid;
  assign s_axil_bus_o_cast.arready = arready_lo;
  assign s_axil_bus_o_cast.rdata   = rdata_lo;
  assign s_axil_bus_o_cast.rresp   = rresp_lo;
  assign s_axil_bus_o_cast.rvalid  = rvalid_lo;
  assign rready_li                 = s_axil_bus_i_cast.rready;


// --------------------------------------------
// axil write state machine
// --------------------------------------------
  // Although the awvalid and wvalid can be asserted at the same cycle,
  // we assume they come in series events, for simplicity.
  typedef enum logic[2:0] {
    E_WR_IDLE = 0,
    E_WR_ADDR = 1,
    E_WR_DATA = 2,
    E_WR_RESP = 3
  } wr_state_e;

  wr_state_e wr_state_r, wr_state_n;

  always_ff @(posedge clk_i)
    if (reset_i)
      wr_state_r <= E_WR_IDLE;
    else
      wr_state_r <= wr_state_n;


  logic write_bresp_lo;

  always_comb begin : wr_state_control

    wr_state_n = wr_state_r;

    case (wr_state_r)

      E_WR_IDLE :
        begin
          if (awvalid_li)
            wr_state_n = E_WR_ADDR;
        end

      E_WR_ADDR :
        begin
          wr_state_n = E_WR_DATA;  // always ready to accept address
        end

      E_WR_DATA :
        begin
          if (wvalid_li & wready_lo)
            wr_state_n = E_WR_RESP;
        end

      E_WR_RESP :
        begin
          if (write_bresp_lo)
            wr_state_n = E_WR_IDLE;
        end

    endcase
  end

  logic tx_done_lo; // from tx fifo

  // waddr channel
  assign awready_lo = (wr_state_r == E_WR_ADDR);
  assign wready_lo  = ((wr_state_r == E_WR_DATA) && (tx_done_lo));

  logic [31:0] wr_addr_r;
  always_ff @(posedge clk_i) begin
    wr_addr_r <= (awvalid_li & awready_lo) ? awaddr_li : wr_addr_r;
  end

  // wdata channel
  logic [num_2fifos_p-1:0] write_to_fifo;
  logic [num_2fifos_p-1:0] write_to_base;

  logic [num_2fifos_p-1:0]                    tx_v_li, tx_r_lo;
  logic [num_2fifos_p-1:0][fifo_width_lp-1:0] tx_li  ;

  assign tx_v_li    = {num_2fifos_p{wvalid_li}} & write_to_fifo;
  assign tx_li      = {num_2fifos_p{wdata_li}};
  assign tx_done_lo = |write_to_base;  // assign tx_done_lo = |(tx_v_li & tx_r_lo);


  // outside read from fifo
  logic [num_2fifos_p-1:0]                    tx_v_lo, tx_r_li;
  logic [num_2fifos_p-1:0][fifo_width_lp-1:0] tx_lo  ;

  assign fifo_v_o    = tx_v_lo;
  assign fifo_data_o = tx_lo;
  assign tx_r_li     = fifo_rdy_i;

  wire [num_2fifos_p-1:0] tx_enqueue = tx_v_li & tx_r_lo;
  wire [num_2fifos_p-1:0] tx_dequeue = tx_r_li & tx_v_lo;

  logic [num_2fifos_p-1:0][fifo_ptr_width_lp-1:0] tx_vacancy_lo;

  for (genvar i=0; i<num_2fifos_p; i++) begin
    bsg_counter_up_down #(
      .max_val_p (fifo_els_p),
      .init_val_p(fifo_els_p),
      .max_step_p(1         )
    ) tx_vacancy_counter (      .*,
      .down_i (tx_enqueue[i]   ),
      .up_i   (tx_dequeue[i]   ),
      .count_o(tx_vacancy_lo[i])
    );

    bsg_fifo_1r1w_small #(
      .width_p           (fifo_width_lp),
      .els_p             (fifo_els_p   ),
      .ready_THEN_valid_p(0            )
    ) tx_fifo (      .*,
      .v_i    (tx_v_li[i]   ),
      .ready_o(tx_r_lo[i]   ),
      .data_i (tx_li[i]     ),
      .v_o    (tx_v_lo[i]   ),
      .data_o (tx_lo[i]     ),
      .yumi_i (tx_dequeue[i])
    );

    assign write_to_fifo[i] = (wr_addr_r == {index_addr_width_lp'(i + axil_base_addr_start_lp), config_addr_width_lp'(tx_data_lp)});
    assign write_to_base[i] = (wr_addr_r[axil_base_addr_start_lp+:index_addr_width_lp] == index_addr_width_lp'(i + axil_base_addr_start_lp));
  end

  // bus response channel
  always_comb begin : bus_response
    if ((wr_state_r == E_WR_RESP) && bready_li) begin
      bvalid_lo = '1;
    end
    else begin
      bvalid_lo = '0;
    end
  end

  assign bresp_lo       = 2'h00; // always OKAY to read
  assign write_bresp_lo = bready_li & bvalid_lo;

// --------------------------------------------
// axil read state machine
// --------------------------------------------

  typedef enum logic[2:0] {
    E_RD_IDLE = 0,
    E_RD_ADDR = 1,
    E_RD_DATA = 2
  } rd_state_e;

  rd_state_e rd_state_r, rd_state_n;


  always_ff @(posedge clk_i)
    if (reset_i)
      rd_state_r <= E_RD_IDLE;
    else
      rd_state_r <= rd_state_n;


  always_comb begin : rd_state_control

    rd_state_n = rd_state_r;

    case (rd_state_r)

      E_RD_IDLE :
        begin
          if (arvalid_li)
            rd_state_n = E_RD_ADDR;
        end

      E_RD_ADDR :
        begin
          rd_state_n = E_RD_DATA;  // always ready to accept address
        end

      E_RD_DATA :
        begin
          if (rvalid_lo & rready_li)
            rd_state_n = E_RD_IDLE;
        end

    endcase
  end


  logic rx_done_lo; // from rx fifo

  // raddr channel
  assign arready_lo = (rd_state_r == E_RD_ADDR);
  assign rvalid_lo  = ((rd_state_r == E_RD_DATA) && rx_done_lo);

  logic [31:0] rd_addr_r;
  always_ff @(posedge clk_i) begin
    rd_addr_r <= (arvalid_li & arready_lo) ? araddr_li : rd_addr_r;
  end

  // rdata channel
  logic [num_2fifos_p-1:0] read_from_fifo;
  logic [num_2fifos_p-1:0] read_from_base;

  logic [num_2fifos_p-1:0]                    rx_v_lo, rx_r_li;
  logic [num_2fifos_p-1:0][fifo_width_lp-1:0] rx_lo  ;

  logic [`BSG_SAFE_CLOG2(num_2fifos_p)-1:0] fifo_rdy_idx  ;
  logic                                     fifo_rdy_idx_v;
  if (num_2fifos_p == 1) begin : one_fifo
    assign fifo_rdy_idx[0] = read_from_fifo[0];
    assign fifo_rdy_idx_v  = read_from_fifo[0];
  end
  else begin : many_fifos
    bsg_encode_one_hot #(.width_p(num_2fifos_p)) fifo_idx_encode (
      .i(read_from_fifo)
      ,.addr_o(fifo_rdy_idx)
      ,.v_o(fifo_rdy_idx_v)
    );
  end

  logic [num_2fifos_p-1:0][fifo_width_lp-1:0] reg_lo;

  assign rx_r_li    = {num_2fifos_p{rready_li}} & read_from_fifo;
  assign rdata_lo   = fifo_rdy_idx_v ? rx_lo[fifo_rdy_idx] : reg_lo[fifo_rdy_idx];
  assign rx_done_lo = |read_from_base;

  // outside write to fifo
  logic [num_2fifos_p-1:0]                    rx_v_li, rx_r_lo;
  logic [num_2fifos_p-1:0][fifo_width_lp-1:0] rx_li  ;

  assign rx_v_li    = fifo_v_i;
  assign rx_li      = fifo_data_i;
  assign fifo_rdy_o = rx_r_lo;

  wire [num_2fifos_p-1:0] rx_enqueue = rx_v_li & rx_r_lo;
  wire [num_2fifos_p-1:0] rx_dequeue = rx_r_li & rx_v_lo;

  logic [num_2fifos_p-1:0][fifo_ptr_width_lp-1:0] rx_occupancy_lo;

  for (genvar i=0; i<num_2fifos_p; i++) begin
    bsg_counter_up_down #(
      .max_val_p (fifo_els_p),
      .init_val_p(0         ),
      .max_step_p(1         )
    ) rx_occupancy_counter (      .*,
      .down_i (rx_enqueue[i]     ),
      .up_i   (rx_dequeue[i]     ),
      .count_o(rx_occupancy_lo[i])
    );

    bsg_fifo_1r1w_small #(
      .width_p           (fifo_width_lp),
      .els_p             (fifo_els_p   ),
      .ready_THEN_valid_p(0            )
    ) rx_fifo (      .*,
      .v_i    (rx_v_li[i]   ),
      .ready_o(rx_r_lo[i]   ),
      .data_i (rx_li[i]     ),
      .v_o    (rx_v_lo[i]   ),
      .data_o (rx_lo[i]     ),
      .yumi_i (rx_dequeue[i])
    );

    assign read_from_fifo[i] = rd_addr_r == {index_addr_width_lp'(i + axil_base_addr_start_lp), config_addr_width_lp'(rx_data_lp)};
    assign read_from_base[i] = rd_addr_r[config_addr_width_lp+:index_addr_width_lp] == index_addr_width_lp'(i + axil_base_addr_start_lp);

    always_comb begin : registers
      case (rd_addr_r[0+:config_addr_width_lp])
        tx_vacancy_lp   : reg_lo[i] = fifo_width_lp'(tx_vacancy_lo[i]);
        rx_occupancy_lp : reg_lo[i] = fifo_width_lp'(rx_occupancy_lo[i]);
        default         : reg_lo[i] = fifo_width_lp'(32'hDEAD_BEEF);
      endcase
    end
  end

  assign rresp_lo = 2'h00; // always OKAY to read

endmodule // bsg_axil_to_fifos
