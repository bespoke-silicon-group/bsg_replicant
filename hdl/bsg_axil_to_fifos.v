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
* 3. read the isr and check [27]=1 for successful write (the last data has been dequeued from the tx FIFO). check again or reset if fail.
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
`include "bsg_axil_to_mcl_pkg.vh"

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
  ,output [           num_slots_p-1:0]                    fifo_v_o
  ,output [           num_slots_p-1:0][fifo_width_lp-1:0] fifo_data_o
  ,input  [           num_slots_p-1:0]                    fifo_ready_i
  ,input  [           num_slots_p-1:0]                    fifo_v_i
  ,input  [           num_slots_p-1:0][fifo_width_lp-1:0] fifo_data_i
  ,output [           num_slots_p-1:0]                    fifo_ready_o
  ,output [                      31:0]                    rom_addr_o
  ,input  [                      31:0]                    rom_data_i
  ,input  [           num_slots_p-1:0][fifo_width_lp-1:0] rcv_vacancy_i
  ,input  [         num_slots_p/2-1:0][fifo_width_lp-1:0] mc_out_credits_i
);



  localparam fifo_ptr_width_lp = `BSG_WIDTH(fifo_els_p);

  localparam index_addr_width_lp  = (axil_addr_width_lp-base_addr_width_p);

  localparam ofs_rsp_rcv_vacancy_lp = base_addr_width_p'(HOST_RCV_VACANCY_MC_RES_p);
  localparam ofs_req_rcv_vacancy_lp = base_addr_width_p'(HOST_RCV_VACANCY_MC_REQ_p);
  localparam ofs_mc_out_credits_lp  = base_addr_width_p'(HOST_REQ_CREDITS_p)       ;

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


  //----------------------------------------------
  // tx fifo
  //----------------------------------------------

  // data output
  logic [num_slots_p-1:0][fifo_width_lp-1:0] tx_data_lo;
  logic [num_slots_p-1:0] tx_v_lo;
  logic [num_slots_p-1:0] tx_ready_li;

  wire [num_slots_p-1:0] tx_enqueue = tx_v_lo & tx_ready_li;
  wire [num_slots_p-1:0] tx_dequeue = fifo_v_o & fifo_ready_i;

  logic [num_slots_p-1:0] clear_isr_tc_lo;
  logic [num_slots_p-1:0][fifo_ptr_width_lp-1:0] tx_vacancy_lo;
  logic [num_slots_p-1:0][31:0] isr_r;

  bsg_axil_to_fifos_tx #(
    .num_fifos_p (num_slots_p  ),
    .fifo_width_p(fifo_width_lp)
  ) axil_to_fifos_tx (
    .clk_i(clk_i)
    ,.reset_i(reset_i)
    ,.awaddr_i      (awaddr_li      )
    ,.awvalid_i     (awvalid_li     )
    ,.awready_o     (awready_lo     )
    ,.wdata_i       (wdata_li       )
    ,.wstrb_i       (wstrb_li       )
    ,.wvalid_i      (wvalid_li      )
    ,.wready_o      (wready_lo      )
    ,.bresp_o       (bresp_lo       )
    ,.bvalid_o      (bvalid_lo      )
    ,.bready_i      (bready_li      )
    ,.tx_data_o     (tx_data_lo     )
    ,.tx_v_o        (tx_v_lo        )
    ,.tx_ready_i    (tx_ready_li    )
    ,.clear_isr_tc_o(clear_isr_tc_lo)
  );

  for (genvar i=0; i<num_slots_p; i++) begin : transmit_fifo
    bsg_counter_up_down #(
      .max_val_p (fifo_els_p)
      ,.init_val_p(fifo_els_p)
      ,.max_step_p(1         )
    ) tx_vacancy_counter (      
      .clk_i(clk_i)
      ,.reset_i(reset_i)
      ,.down_i (tx_enqueue[i]   )
      ,.up_i   (tx_dequeue[i]   )
      ,.count_o(tx_vacancy_lo[i])
    );

    bsg_fifo_1r1w_small #(
      .width_p           (fifo_width_lp)
      ,.els_p             (fifo_els_p   )
      ,.ready_THEN_valid_p(0            )
    ) tx_fifo (
      .clk_i(clk_i)
      ,.reset_i(reset_i)
      ,.v_i    (tx_v_lo[i]    )
      ,.ready_o(tx_ready_li[i])
      ,.data_i (tx_data_lo[i] )
      ,.v_o    (fifo_v_o[i]   )
      ,.data_o (fifo_data_o[i])
      ,.yumi_i (tx_dequeue[i] )
    );

    always_ff @(posedge clk_i) begin
			if (reset_i)
				isr_r <= '0;
			else if (clear_isr_tc_lo[i])
        isr_r[i][FIFO_ISR_TC_BIT_p] <= 1'b0; // clear the tx complete bit
      else if ((tx_vacancy_lo[i]==fifo_ptr_width_lp'(fifo_els_p-1)) & tx_dequeue[i])
        isr_r[i][FIFO_ISR_TC_BIT_p] <= 1'b1;  // set the tx complete bit
      else
        isr_r[i] <= isr_r[i];
    end
  end


  //----------------------------------------------
  // rx fifo
  //----------------------------------------------

  wire [num_slots_p-1:0] rx_v_li;
  wire [num_slots_p-1:0][31:0] rx_data_li;
  wire [num_slots_p-1:0] rx_ready_lo;

  wire [num_slots_p-1:0] rx_enqueue = fifo_v_i & fifo_ready_o;
  wire [num_slots_p-1:0] rx_dequeue = rx_ready_lo & rx_v_li;

  logic [num_slots_p-1:0][fifo_ptr_width_lp-1:0] rx_occupancy_lo;

  wire [31:0] rd_addr_lo;
  logic [num_slots_p-1:0][31:0] mon_data_li;
  logic [31:0] rom_data_li;

  bsg_axil_to_fifos_rx #(.num_fifos_p(num_slots_p)) axil_fifo_rx (
    .clk_i     (clk_i),
    .reset_i   (reset_i),
    .araddr_i  (araddr_li  ),
    .arvalid_i (arvalid_li ),
    .arready_o (arready_lo ),
    .rdata_o   (rdata_lo   ),
    .rresp_o   (rresp_lo   ),
    .rvalid_o  (rvalid_lo  ),
    .rready_i  (rready_li  ),
    .rx_v_i    (rx_v_li    ),
    .rx_data_i (rx_data_li ),
    .rx_ready_o(rx_ready_lo),
    .rd_addr_o (rd_addr_lo ),
    .mon_data_i(mon_data_li),
    .rom_data_i(rom_data_li)
  );

  for (genvar i=0; i<num_slots_p; i++) begin : receive_fifo
    bsg_counter_up_down #(
      .max_val_p (fifo_els_p)
      ,.init_val_p(0         )
      ,.max_step_p(1         )
    ) rx_occupancy_counter (
      .clk_i(clk_i)
      ,.reset_i(reset_i)
      ,.down_i (rx_dequeue[i]     )
      ,.up_i   (rx_enqueue[i]     )
      ,.count_o(rx_occupancy_lo[i])
    );

    bsg_fifo_1r1w_small #(
      .width_p           (fifo_width_lp)
      ,.els_p             (fifo_els_p   )
      ,.ready_THEN_valid_p(0            )
    ) rx_fifo (
      .clk_i  (clk_i          )
      ,.reset_i(reset_i        )
      ,.v_i    (fifo_v_i[i]    )
      ,.ready_o(fifo_ready_o[i])
      ,.data_i (fifo_data_i[i] )
      ,.v_o    (rx_v_li[i]     )
      ,.data_o (rx_data_li[i]  )
      ,.yumi_i (rx_dequeue[i]  )
    );

    // read from registers
    always_comb begin
      case (rd_addr_lo[0+:base_addr_width_p])
        ofs_isr_lp  : mon_data_li[i] = isr_r[i];
        ofs_tdfv_lp : mon_data_li[i] = fifo_width_lp'(tx_vacancy_lo[i]);
        ofs_rdfo_lp : mon_data_li[i] = fifo_width_lp'({rx_occupancy_lo[i][fifo_ptr_width_lp-1:2],2'b00});
        ofs_rlr_lp  : mon_data_li[i] = (rx_occupancy_lo[i] >= fifo_rlr_words_lp) ? (fifo_width_lp/8*fifo_rlr_words_lp): '0;
        default     : mon_data_li[i] = fifo_width_lp'(32'hBEEF_DEAD);
      endcase
    end
  end

  // read from rom and monitors
  assign rom_addr_o = rd_addr_lo;
  always_comb begin
    case (rd_addr_lo[0+:base_addr_width_p])
      ofs_rsp_rcv_vacancy_lp : rom_data_li = rcv_vacancy_i[0];
      ofs_req_rcv_vacancy_lp : rom_data_li = rcv_vacancy_i[1];
      ofs_mc_out_credits_lp  : rom_data_li = mc_out_credits_i[0];
      default                : rom_data_li = rom_data_i;
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
