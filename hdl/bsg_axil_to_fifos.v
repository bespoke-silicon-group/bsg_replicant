/*
* bsg_axil_to_fifos.v
*
* adapt axil interface to parameterized fifo interface
*
* Note:
* This adapter is similar to the Xilinx axi_fifo_mm_s IP working on cut-though mode. (No TLR is used)
* Config addresses are base_address + n * 0x100, where n=range(num_slots_p)
* raise DECERR if access address is out of range
*
* The host AXI-Lite write and read examples:
* To write to a FIFO,
* 1. set the isr[27] to 0
* 2. write words to base_addr + ofs_tdr_lp
* 3. read the isr and check [27]=1 for successful write (all the tx FIFO data has been dequeued). check again or reset if fail.
* Read the transmit vacancy register at base_addr + ofs_tdfv_lp to get the current tx FIFO status
* Note: if write to a full fifo, data will not be updated.
*
* To read from a FIFO,
* 1. read the Receive Length Reigster, RLR = fifo_width_lp/8 * fifo_rlr_words_lp if rx_FIFO >= fifo_rlr_words_lp, else 0
* 2. read RLR data from Receive Destination Register RDR,
* if read from a empty fifo, you will get stale data
*/

`include "bsg_defines.v"
`include "bsg_axi_bus_pkg.vh"
`include "axil_to_mcl.vh"

module bsg_axil_to_fifos 
  import cl_mcl_pkg::*;
#(
  parameter axil_base_addr_p = "inv"
  , parameter num_slots_p = 2
  , parameter fifo_els_p = "inv"
  , parameter fifo_width_lp = axil_data_width_lp
  , parameter axil_mosi_bus_width_lp = `bsg_axil_mosi_bus_width(1)
  , parameter axil_miso_bus_width_lp = `bsg_axil_miso_bus_width(1)
) (
  input                                                  clk_i
  ,input                                                  reset_i
  ,input  [axil_mosi_bus_width_lp-1:0]                    s_axil_bus_i
  ,output [axil_miso_bus_width_lp-1:0]                    s_axil_bus_o
  ,input  [           num_slots_p-1:0]                    fifo_v_i
  ,input  [           num_slots_p-1:0][fifo_width_lp-1:0] fifo_data_i
  ,output [           num_slots_p-1:0]                    fifo_rdy_o
  ,output [           num_slots_p-1:0]                    fifo_v_o
  ,output [           num_slots_p-1:0][fifo_width_lp-1:0] fifo_data_o
  ,input  [           num_slots_p-1:0]                    fifo_rdy_i
  ,output [                      31:0]                    rom_addr_o
  ,input  [                      31:0]                    rom_data_i
  ,input  [           num_slots_p-1:0][fifo_width_lp-1:0] rcv_vacancy_i
  ,input  [         num_slots_p/2-1:0][fifo_width_lp-1:0] mc_out_credits_i
);

  localparam fifo_rlr_words_lp = 4; // retuen RLR is fixed, 4x4 bytes

  localparam fifo_ptr_width_lp = `BSG_WIDTH(fifo_els_p);

  localparam index_addr_width_lp  = (axil_addr_width_lp-base_addr_width_lp);

  localparam ofs_isr_lp  = 8'h0 ; // Interrupt Status Register
  localparam ofs_tdfv_lp = 8'hC ; // Transmit FIFO Vacancy
  localparam ofs_tdr_lp  = 8'h10; // Transmit Length Register
  localparam ofs_rdfo_lp = 8'h1C; // Receive Data FIFO occupancy
  localparam ofs_rdr_lp  = 8'h20; // Receive Data Destination Register
  localparam ofs_rlr_lp  = 8'h24; // Receive Length Register

  localparam ofs_rsp_rcv_vacancy_lp = base_addr_width_lp'(HOST_RCV_VACANCY_MC_RES_p);
  localparam ofs_req_rcv_vacancy_lp = base_addr_width_lp'(HOST_RCV_VACANCY_MC_REQ_p);
  localparam ofs_mc_out_credits_lp  = base_addr_width_lp'(HOST_REQ_CREDITS_p)       ;

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

  wire tx_done_lo = 1'b1; // always ready for the write

  logic [num_slots_p-1:0] write_to_base;
  logic [num_slots_p-1:0] write_to_fifo;
  logic [num_slots_p-1:0] write_to_isr ;

  // waddr channel
  assign awready_lo = (wr_state_r == E_WR_ADDR);
  assign wready_lo  = ((wr_state_r == E_WR_DATA) && (tx_done_lo));

  logic [31:0] wr_addr_r;
  always_ff @(posedge clk_i) begin
    wr_addr_r <= (awvalid_li & awready_lo) ? awaddr_li : wr_addr_r;
  end

  // write response channel
  always_ff @(posedge clk_i) begin
    if ((wr_state_r == E_WR_DATA) || (wr_state_r == E_WR_RESP))
      bresp_lo <= |write_to_base ? '0 : 2'b11;  // OKAY or DECERR
    else
      bresp_lo <= '0;
  end

  always_comb begin : bus_response
    if ((wr_state_r == E_WR_RESP) && bready_li) begin
      bvalid_lo = '1;
    end
    else begin
      bvalid_lo = '0;
    end
  end

  assign write_bresp_lo = bready_li & bvalid_lo;

  // wdata channel
  logic [num_slots_p-1:0]                    tx_v_li, tx_r_lo;
  logic [num_slots_p-1:0][fifo_width_lp-1:0] tx_li  ;

  assign tx_v_li    = {num_slots_p{wvalid_li & wready_lo}} & write_to_fifo;
  assign tx_li      = {num_slots_p{wdata_li}};

  // outside read from fifo
  logic [num_slots_p-1:0]                    tx_v_lo, tx_r_li;
  logic [num_slots_p-1:0][fifo_width_lp-1:0] tx_lo  ;

  assign fifo_v_o    = tx_v_lo;
  assign fifo_data_o = tx_lo;
  assign tx_r_li     = fifo_rdy_i;

  wire [num_slots_p-1:0] tx_enqueue = tx_v_li & tx_r_lo;
  wire [num_slots_p-1:0] tx_dequeue = tx_r_li & tx_v_lo;

  logic [num_slots_p-1:0][fifo_ptr_width_lp-1:0] tx_vacancy_lo;

  logic [num_slots_p-1:0][31:0] isr_r;

  for (genvar i=0; i<num_slots_p; i++) begin : transmit_fifo
    bsg_counter_up_down #(
      .max_val_p (fifo_els_p)
      ,.init_val_p(fifo_els_p)
      ,.max_step_p(1         )
    ) tx_vacancy_counter (      
      .clk_i
      ,.reset_i
      ,.down_i (tx_enqueue[i]   )
      ,.up_i   (tx_dequeue[i]   )
      ,.count_o(tx_vacancy_lo[i])
    );

    bsg_fifo_1r1w_small #(
      .width_p           (fifo_width_lp)
      ,.els_p             (fifo_els_p   )
      ,.ready_THEN_valid_p(0            )
    ) tx_fifo (
      .clk_i
      ,.reset_i
      ,.v_i    (tx_v_li[i]   )
      ,.ready_o(tx_r_lo[i]   )
      ,.data_i (tx_li[i]     )
      ,.v_o    (tx_v_lo[i]   )
      ,.data_o (tx_lo[i]     )
      ,.yumi_i (tx_dequeue[i])
    );

    assign write_to_base[i] = (wr_addr_r[base_addr_width_lp+:index_addr_width_lp] == index_addr_width_lp'(i+ (axil_base_addr_p>>base_addr_width_lp)));
    assign write_to_fifo[i] = write_to_base[i] && (wr_addr_r[0+:base_addr_width_lp] == base_addr_width_lp'(ofs_tdr_lp));
    assign write_to_isr[i]  = write_to_base[i] && (wr_addr_r[0+:base_addr_width_lp] == base_addr_width_lp'(ofs_isr_lp));

    always_ff @(posedge clk_i) begin : cfg_register
      if (write_to_isr[i] & (wdata_li[FIFO_ISR_TC_BIT_p]))
        isr_r[i][FIFO_ISR_TC_BIT_p] <= 1'b0;
        else if (tx_vacancy_lo==fifo_ptr_width_lp'(fifo_els_p))
        isr_r[i][FIFO_ISR_TC_BIT_p] <= 1'b1;
      else
        isr_r[i] <= isr_r[i];
    end
  end

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


  wire rx_done_lo = 1'b1; // from rx fifo

  // raddr channel
  assign arready_lo = (rd_state_r == E_RD_ADDR);
  assign rvalid_lo  = ((rd_state_r == E_RD_DATA) && rx_done_lo);

  logic [31:0] rd_addr_r;
  always_ff @(posedge clk_i) begin
    if (arvalid_li & arready_lo)
      rd_addr_r <= araddr_li;
  end

  // read response channel
  always_ff @(posedge clk_i) begin
    if (rd_state_r == E_RD_DATA)
      rresp_lo <= (|read_from_base || read_from_rom) ? '0 : 2'b11;  // OKAY or DECERR
    else
      rresp_lo <= '0;
  end

  // rdata channel
  logic [num_slots_p-1:0] read_from_fifo;
  logic [num_slots_p-1:0] read_from_base;
  logic                   read_from_rom ;

  logic [num_slots_p-1:0]                    rx_r_li;
  logic [num_slots_p-1:0]                    rx_v_lo;
  logic [num_slots_p-1:0][fifo_width_lp-1:0] rx_lo  ;

  logic [num_slots_p-1:0][31:0] reg_lo         ;
  logic [           31:0]       monitor_data_lo;

  logic [`BSG_SAFE_CLOG2(num_slots_p)-1:0] rd_base_idx;

  wire fifo_rdy_idx_v = |read_from_fifo;  // get the fifo data when either fifo is rdy

  if (num_slots_p == 1) begin : one_fifo
    assign rd_base_idx[0] = read_from_fifo[0];
    assign rdata_lo = read_from_rom ? monitor_data_lo : fifo_rdy_idx_v ? rx_lo[0] : reg_lo[0];
  end
  else begin : many_fifos
    bsg_encode_one_hot #(.width_p(num_slots_p)) fifo_idx_encode (
      .i(read_from_base)
      ,.addr_o(rd_base_idx)
      ,.v_o()
    );
    assign rdata_lo = read_from_rom ? monitor_data_lo : fifo_rdy_idx_v ? rx_lo[rd_base_idx] : reg_lo[rd_base_idx];
  end


  assign rx_r_li    = {num_slots_p{rvalid_lo & rready_li}} & read_from_fifo;

  // outside write to fifo
  logic [num_slots_p-1:0]                    rx_v_li, rx_r_lo;
  logic [num_slots_p-1:0][fifo_width_lp-1:0] rx_li  ;

  assign rx_v_li    = fifo_v_i;
  assign rx_li      = fifo_data_i;
  assign fifo_rdy_o = rx_r_lo;

  wire [num_slots_p-1:0] rx_enqueue = rx_v_li & rx_r_lo;
  wire [num_slots_p-1:0] rx_dequeue = rx_r_li & rx_v_lo;

  logic [num_slots_p-1:0][fifo_ptr_width_lp-1:0] rx_occupancy_lo;

  for (genvar i=0; i<num_slots_p; i++) begin : receive_fifo
    bsg_counter_up_down #(
      .max_val_p (fifo_els_p)
      ,.init_val_p(0         )
      ,.max_step_p(1         )
    ) rx_occupancy_counter (
      .clk_i
      ,.reset_i
      ,.down_i (rx_dequeue[i]     )
      ,.up_i   (rx_enqueue[i]     )
      ,.count_o(rx_occupancy_lo[i])
    );

    bsg_fifo_1r1w_small #(
      .width_p           (fifo_width_lp),
      .els_p             (fifo_els_p   ),
      .ready_THEN_valid_p(0            )
    ) rx_fifo (
      .clk_i                 
      ,.reset_i
      ,.v_i    (rx_v_li[i]   )
      ,.ready_o(rx_r_lo[i]   )
      ,.data_i (rx_li[i]     )
      ,.v_o    (rx_v_lo[i]   )
      ,.data_o (rx_lo[i]     )
      ,.yumi_i (rx_dequeue[i])
    );

    assign read_from_base[i] = (rd_addr_r[base_addr_width_lp+:index_addr_width_lp] == index_addr_width_lp'(i + (axil_base_addr_p>>base_addr_width_lp)));
    assign read_from_fifo[i] = read_from_base[i] && (rd_addr_r[0+:base_addr_width_lp] == base_addr_width_lp'(ofs_rdr_lp));

    always_comb begin : registers
      case (rd_addr_r[0+:base_addr_width_lp])
        ofs_isr_lp  : reg_lo[i] = isr_r[i];
        ofs_tdfv_lp : reg_lo[i] = fifo_width_lp'(tx_vacancy_lo[i]);
        ofs_rdfo_lp : reg_lo[i] = fifo_width_lp'({rx_occupancy_lo[i][fifo_ptr_width_lp-1:2],2'b00});
        ofs_rlr_lp  : reg_lo[i] = (rx_occupancy_lo[i] >= fifo_rlr_words_lp) ? (fifo_width_lp/8*fifo_rlr_words_lp): '0;
        default     : reg_lo[i] = fifo_width_lp'(32'hBEEF_DEAD);
      endcase
    end
  end

  // read from monitors
  assign read_from_rom = (rd_addr_r[base_addr_width_lp+:index_addr_width_lp] == index_addr_width_lp'(num_slots_p + (axil_base_addr_p>>base_addr_width_lp)));
  assign rom_addr_o    = rd_addr_r;
  always_comb begin : monitor_signals
    monitor_data_lo = '0;
    if (read_from_rom & rvalid_lo)
      case (rd_addr_r[0+:base_addr_width_lp])
        ofs_rsp_rcv_vacancy_lp : monitor_data_lo = rcv_vacancy_i[0];
        ofs_req_rcv_vacancy_lp : monitor_data_lo = rcv_vacancy_i[1];
        ofs_mc_out_credits_lp  : monitor_data_lo = mc_out_credits_i[0];
        default                : monitor_data_lo = rom_data_i;
      endcase
  end

  // synopsys translate_off
  initial begin
    assert (num_slots_p >= 2)
      else
        $fatal("## only support axis fifo slot == 2!");
  end
  // synopsys translate_on

endmodule // bsg_axil_to_fifos
