/*
* bsg_axil_to_fifos.v
*
*     axil interface <-> fifo interface
*       ____________      __________
*      | tx streams | -> | tx fifos | -->
*      |____________|    |__________|
*      |            \______________________clr isr
*  ==> |                                   |
*      |____________      __________       |
*      | rx streams | <- | rx fifos | -->  |
*      |____________|    |__________|      |
*          |              ____________     |
*  --> --->/<------------/monitor regs\<---/
*  rom_data &            \____________/
*  mcl_credits           (see definitions in Note2)
*
* Note:
* 1) Axil address is divided as |--slot index[31:12]--|--base address[11:0]--|
*    The slot indexes are defined as:
*    20'b0: axil to fifos, host as master
*    20'b1: axil to fifos, host as slave
*    20'b2: monitor and rom data
*
* 2) The axil to stream adapter is similar to the Xilinx axi_fifo_mm_s IP in cut-though mode
*    config registers are defined in bsg_manycore_link_to_axil_pkg.v
*
* 3) Axil Write and Read examples:
*    a. write to a FIFO,
*       1. set the isr[27] to 0
*       2. write words to base_addr + axil_mm2s_ofs_tdr_gp
*       3. read the isr and check [27]=1 for successful write, i.e. the last data has been
*          dequeued from the tx FIFO. if fail. check again or reset if fail
*       4. read the transmit vacancy register (TDFV) to get the current tx FIFO status.
           TDFV will not be updated if write to a full fifo
*
*    b. read from a FIFO,
*       1. read the Receive Length Reigster
           For simple implementation, RLR is fixed to 
           (rx_FIFO >= axil_mm2s_rlr_els_gp) ? fifo_width_p/8 * axil_mm2s_rlr_els_gp : 0
*       2. read RLR bytes of data at Receive Destination Register (RDR)
*          Host will get stale data if read from a empty fifo
*
* TODO: Monitor addresses should be remapped, to fully parameterize the parameter num_slots_p,
*
*/

`include "bsg_defines.v"
`include "bsg_axi_bus_pkg.vh"
`include "bsg_manycore_link_to_axil_pkg.v"

module bsg_axil_to_fifos
  import bsg_manycore_link_to_axil_pkg::*;
#(
  parameter num_slots_p = 2
  , parameter fifo_width_p = "inv"
  , parameter axil_mosi_bus_width_lp = `bsg_axil_mosi_bus_width(1)
  , parameter axil_miso_bus_width_lp = `bsg_axil_miso_bus_width(1)
) (
  input                                                 clk_i
  ,input                                                 reset_i
  ,input  [axil_mosi_bus_width_lp-1:0]                   s_axil_bus_i
  ,output [axil_miso_bus_width_lp-1:0]                   s_axil_bus_o
  ,output [           num_slots_p-1:0][fifo_width_p-1:0] fifos_o
  ,output [           num_slots_p-1:0]                   fifos_v_o
  ,input  [           num_slots_p-1:0]                   fifos_ready_i
  ,input  [           num_slots_p-1:0][fifo_width_p-1:0] fifos_i
  ,input  [           num_slots_p-1:0]                   fifos_v_i
  ,output [           num_slots_p-1:0]                   fifos_ready_o
  ,output [                      31:0]                   rom_addr_o
  ,input  [                      31:0]                   rom_data_i
  ,input  [           num_slots_p-1:0][fifo_width_p-1:0] rcv_vacancy_i
  ,input  [         num_slots_p/2-1:0][fifo_width_p-1:0] out_credits_i
);

  // synopsys translate_off
  initial begin
    assert (num_slots_p == 2)
      else $fatal("Only support num_slots_p=2, [%m]");
  end
  // synopsys translate_on


  `declare_bsg_axil_bus_s(1, bsg_axil_mosi_bus_s, bsg_axil_miso_bus_s);
  bsg_axil_mosi_bus_s s_axil_bus_li_cast;
  bsg_axil_miso_bus_s s_axil_bus_lo_cast;

  assign s_axil_bus_li_cast = s_axil_bus_i;
  assign s_axil_bus_o       = s_axil_bus_lo_cast;


  logic [num_slots_p-1:0][fifo_width_p-1:0] txs_lo      ;
  logic [num_slots_p-1:0]                   txs_v_lo    ;
  logic [num_slots_p-1:0]                   txs_ready_li;

  logic [num_slots_p-1:0][fifo_width_p-1:0] rxs_li      ;
  logic [num_slots_p-1:0]                   rxs_v_li    ;
  logic [num_slots_p-1:0]                   rxs_ready_lo;

  localparam fifo_ptr_width_lp = `BSG_WIDTH(axil_fifo_els_gp);

  logic [num_slots_p-1:0][fifo_ptr_width_lp-1:0] tx_vacancy_lo  ;
  logic [num_slots_p-1:0][fifo_ptr_width_lp-1:0] rx_occupancy_lo;

  // txs to fifos
  //-------------

  logic [num_slots_p-1:0] clr_isr_txc_lo;

  bsg_axil_txs #(.num_fifos_p(num_slots_p)) mm2s_tx (
    .clk_i         (clk_i                     ),
    .reset_i       (reset_i                   ),
    .awaddr_i      (s_axil_bus_li_cast.awaddr ),
    .awvalid_i     (s_axil_bus_li_cast.awvalid),
    .awready_o     (s_axil_bus_lo_cast.awready),
    .wdata_i       (s_axil_bus_li_cast.wdata  ),
    .wstrb_i       (s_axil_bus_li_cast.wstrb  ),
    .wvalid_i      (s_axil_bus_li_cast.wvalid ),
    .wready_o      (s_axil_bus_lo_cast.wready ),
    .bresp_o       (s_axil_bus_lo_cast.bresp  ),
    .bvalid_o      (s_axil_bus_lo_cast.bvalid ),
    .bready_i      (s_axil_bus_li_cast.bready ),
    .txs_o         (txs_lo                    ),
    .txs_v_o       (txs_v_lo                  ),
    .txs_ready_i   (txs_ready_li              ),
    .clr_isrs_txc_o(clr_isr_txc_lo            )
  );

  wire [num_slots_p-1:0] tx_enqueue = txs_v_lo & txs_ready_li  ;
  wire [num_slots_p-1:0] tx_dequeue = fifos_v_o & fifos_ready_i;

  for (genvar i=0; i<num_slots_p; i++) begin : tx_s
    bsg_counter_up_down #(
      .max_val_p (axil_fifo_els_gp)
      ,.init_val_p(axil_fifo_els_gp)
      ,.max_step_p(1         )
    ) tx_vacancy_counter (
      .clk_i(clk_i)
      ,.reset_i(reset_i)
      ,.down_i (tx_enqueue[i]   )
      ,.up_i   (tx_dequeue[i]   )
      ,.count_o(tx_vacancy_lo[i])
    );

    bsg_fifo_1r1w_small #(
      .width_p           (fifo_width_p    ),
      .els_p             (axil_fifo_els_gp),
      .ready_THEN_valid_p(0               )
    ) tx_fifo (
      .clk_i  (clk_i          ),
      .reset_i(reset_i        ),
      .v_i    (txs_v_lo[i]    ),
      .ready_o(txs_ready_li[i]),
      .data_i (txs_lo[i]      ),
      .v_o    (fifos_v_o[i]   ),
      .data_o (fifos_o[i]     ),
      .yumi_i (tx_dequeue[i]  )
    );
  end : tx_s


  // fifos to rxs
  //-------------

  wire [num_slots_p-1:0] rx_enqueue = fifos_v_i & fifos_ready_o;
  wire [num_slots_p-1:0] rx_dequeue = rxs_ready_lo & rxs_v_li  ;

  for (genvar i=0; i<num_slots_p; i++) begin : rx_s
    bsg_counter_up_down #(
      .max_val_p (axil_fifo_els_gp)
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
      .width_p           (fifo_width_p    ),
      .els_p             (axil_fifo_els_gp),
      .ready_THEN_valid_p(0               )
    ) rx_fifo (
      .clk_i  (clk_i           ),
      .reset_i(reset_i         ),
      .v_i    (fifos_v_i[i]    ),
      .ready_o(fifos_ready_o[i]),
      .data_i (fifos_i[i]      ),
      .v_o    (rxs_v_li[i]     ),
      .data_o (rxs_li[i]       ),
      .yumi_i (rx_dequeue[i]   )
    );
  end : rx_s

  logic [           31:0]       axil_rd_addr_lo;
  logic [num_slots_p-1:0][31:0] mm2s_regs_li   ;
  logic [           31:0]       mcl_data_li    ;

  bsg_axil_rxs #(.num_fifos_p(num_slots_p)) mm2s_rx (
    .clk_i      (clk_i                     ),
    .reset_i    (reset_i                   ),
    .araddr_i   (s_axil_bus_li_cast.araddr ),
    .arvalid_i  (s_axil_bus_li_cast.arvalid),
    .arready_o  (s_axil_bus_lo_cast.arready),
    .rdata_o    (s_axil_bus_lo_cast.rdata  ),
    .rresp_o    (s_axil_bus_lo_cast.rresp  ),
    .rvalid_o   (s_axil_bus_lo_cast.rvalid ),
    .rready_i   (s_axil_bus_li_cast.rready ),
    .rxs_i      (rxs_li                    ),
    .rxs_v_i    (rxs_v_li                  ),
    .rxs_ready_o(rxs_ready_lo              ),
    .rd_addr_o  (axil_rd_addr_lo           ),
    .mm2s_regs_i(mm2s_regs_li              ),
    .mcl_data_i (mcl_data_li               )
  );


  //----------------------------------------------
  // mux for reading data:
  // mm2s registers, mcl rom data, mcl credits
  //----------------------------------------------
  logic [num_slots_p-1:0][31:0] axil_mm2s_isr_r;

  for (genvar i=0; i<num_slots_p; i++) begin : mm2s_regs
    always_ff @(posedge clk_i) begin
      if (reset_i)
        axil_mm2s_isr_r[i] <= '0;
      else if (clr_isr_txc_lo[i])
        axil_mm2s_isr_r[i][axil_mm2s_isr_txc_bit_gp] <= 1'b0; // clear the tx complete bit
      else if ((tx_vacancy_lo[i]==fifo_ptr_width_lp'(axil_fifo_els_gp-1)) & tx_dequeue[i])
        axil_mm2s_isr_r[i][axil_mm2s_isr_txc_bit_gp] <= 1'b1; // set the tx complete bit
      else
        axil_mm2s_isr_r[i] <= axil_mm2s_isr_r[i];
    end

    // read from registers
    always_comb begin
      case (axil_rd_addr_lo[0+:axil_base_addr_width_gp])
        axil_mm2s_ofs_isr_gp  : mm2s_regs_li[i] = axil_mm2s_isr_r[i];
        axil_mm2s_ofs_tdfv_gp : mm2s_regs_li[i] = fifo_width_p'(tx_vacancy_lo[i]);
        axil_mm2s_ofs_rdfo_gp : mm2s_regs_li[i] = fifo_width_p'({rx_occupancy_lo[i][fifo_ptr_width_lp-1:2],2'b00});
        axil_mm2s_ofs_rlr_gp  : mm2s_regs_li[i] = (rx_occupancy_lo[i] >= axil_mm2s_rlr_els_gp) ?
                                                    (fifo_width_p/8*axil_mm2s_rlr_els_gp) : '0;
        default               : mm2s_regs_li[i] = fifo_width_p'(32'hBEEF_DEAD);
      endcase
    end
  end : mm2s_regs

  // read from rom and mcl credits
  assign rom_addr_o = axil_rd_addr_lo;
  always_comb begin
    case (axil_rd_addr_lo[0+:axil_base_addr_width_gp])
      host_rcv_vacancy_mc_res_gp : mcl_data_li = rcv_vacancy_i[0];
      host_rcv_vacancy_mc_req_gp : mcl_data_li = rcv_vacancy_i[1];
      host_req_credits_out_gp    : mcl_data_li = out_credits_i[0];
      default                    : mcl_data_li = rom_data_i;
    endcase
  end

endmodule // bsg_axil_to_fifos
