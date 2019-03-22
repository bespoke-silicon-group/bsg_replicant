/**
  *  bsg_bladerunner_rom.v
  *
  */

`include "bsg_defines.v"
`include "bsg_manycore_packet.vh"

module bsg_bladerunner_rom #(
  // rom parameters
  rom_data_width_p="inv"
  ,rom_els_p="inv"
  ,rom_filename_p="inv"
  // manycore parameters
  ,x_cord_width_p="inv"
  ,y_cord_width_p="inv"
  ,fifo_els_p=4
  ,addr_width_p="inv"
  ,data_width_p="inv"
  ,max_out_credits_p=16
  ,load_id_width_p="inv"
  ,data_mask_width_lp=(data_width_p>>3)
  ,link_sif_width_lp=`bsg_manycore_link_sif_width(addr_width_p,data_width_p,x_cord_width_p,y_cord_width_p,load_id_width_p)
) (input clk_i
  , input reset_i

  , input [x_cord_width_p-1:0] my_x_i
  , input [y_cord_width_p-1:0] my_y_i
  , input [link_sif_width_lp-1:0] link_sif_i
  , output [link_sif_width_lp-1:0] link_sif_o
);

  logic in_v_lo;
  logic in_yumi_li;
  logic [data_width_p-1:0] in_data_lo;
  logic [data_mask_width_lp-1:0] in_mask_lo;
  logic [addr_width_p-1:0] in_addr_lo;
  logic in_we_lo;
  logic [x_cord_width_p-1:0] in_src_x_cord_lo;
  logic [y_cord_width_p-1:0] in_src_y_cord_lo;

  logic [data_width_p-1:0] returning_data_li;
  logic returning_v_li;

  bsg_manycore_endpoint_standard #(
    .x_cord_width_p   (x_cord_width_p)
    ,.y_cord_width_p   (y_cord_width_p)
    ,.fifo_els_p       (fifo_els_p)
    ,.addr_width_p     (addr_width_p)
    ,.data_width_p     (data_width_p)
    ,.max_out_credits_p(max_out_credits_p)
    ,.load_id_width_p  (load_id_width_p)
  ) mcl_endpoint_standard (
    .clk_i
    ,.reset_i

    ,.link_sif_i
    ,.link_sif_o

    ,.in_v_o              (in_v_lo)
    ,.in_yumi_i           (in_yumi_li)
    ,.in_data_o           (in_data_lo)
    ,.in_mask_o           (in_mask_lo)
    ,.in_addr_o           (in_addr_lo)
    ,.in_we_o             (in_we_lo)
    ,.in_src_x_cord_o     (in_src_x_cord_lo)
    ,.in_src_y_cord_o     (in_src_y_cord_lo)

    ,.out_v_i             (1'b0)
    ,.out_packet_i        ('0)
    ,.out_ready_o         ()

    ,.returned_data_r_o   ()
    ,.returned_load_id_r_o()
    ,.returned_v_r_o      ()
    ,.returned_fifo_full_o()
    ,.returned_yumi_i     (1'b0)

    ,.returning_data_i    (returning_data_li)
    ,.returning_v_i       (returning_v_li)

    ,.out_credits_o       ()
    ,.my_x_i
    ,.my_y_i
  );


  // request from manycore network
  wire in_ready = 1'b1;
  assign in_yumi_li = in_ready & in_v_lo;

  // response to manycore network
  logic returning_v_r;
  logic [data_width_p-1:0] returning_data_r;
  logic [data_mask_width_lp-1:0] in_mask_r;

	for (genvar i=0; i< data_mask_width_lp; i++) begin
		assign returning_data_li[8*i+:8] = {8{in_mask_r[i]}} & returning_data_r[8*i+:8];
	end

  always_ff @(posedge clk_i)
  begin: return_valid_reg
    if(reset_i) begin
      returning_v_r <= 1'b0;
      in_mask_r <= data_mask_width_lp'(0);
      returning_data_r <= '0;
    end
    else begin
      returning_v_r <= in_v_lo & ~in_we_lo;
      in_mask_r <= in_mask_lo;
      returning_data_r <= data_width_p'(rom_data_lo);
    end
  end

  // rom data path
  localparam rom_addr_width_lp = `BSG_SAFE_CLOG2(rom_els_p);

  logic [rom_addr_width_lp-1:0] rom_addr_li;
  logic [rom_data_width_p-1:0] rom_data_lo;

  assign rom_addr_li = rom_addr_width_lp'(in_addr_lo);
  assign returning_v_li = returning_v_r;

  data_rom #(
    .word_width_p(rom_data_width_p)
    ,.word_count_p(rom_els_p)
    ,.filename_p(rom_filename_p)
  ) bladerunner_rom (
    .addr_i(rom_addr_li)
    ,.data_o(rom_data_lo)
  );

// synopsys translate_off
  initial
  begin
    assert (rom_data_width_p <= data_width_p)
    else $error("ROM data width %x should not exceed manycore link data width %x (%m)\n", rom_data_width_p, data_width_p);

    assert (rom_addr_width_lp <= addr_width_p)
    else $error("ROM addr width %x should not exceed manycore link addr width %x (%m)\n", rom_addr_width_lp, addr_width_p);
  end

  always_ff @(negedge clk_i)
  begin
    if (in_v_lo & in_we_lo) begin
      $display("## Write request is not supporting: from YX=%d,%d landed at YX=%d,%d (%m)"
        ,in_src_y_cord_lo ,in_src_x_cord_lo
        ,my_y_i, my_x_i);
      $finish();
    end
  end
// synopsys translate_on

endmodule

