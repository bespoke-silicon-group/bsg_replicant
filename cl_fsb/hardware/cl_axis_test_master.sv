/**
*  cl_axis_test_master.sv
*
*  axi-stream master node
*/

module cl_axis_test_master 
  #(
    parameter DATA_WIDTH = 512
    ,parameter SINGLE_NUM = 256
    ,parameter PACKET_SIZE = 16
    ,parameter MULTPKT_NUM = 240
    ) 
  (input clk_i
   ,input reset_i
   ,input en_i
   ,axis_bus_t.slave axis_data_bus
   ,output loop_done
  );

  localparam [15:0] single_num_lp = SINGLE_NUM;
  localparam [15:0] packet_size_lp = PACKET_SIZE;
  localparam [15:0] multpkt_num_lp = MULTPKT_NUM;

  logic [DATA_WIDTH-1:0] txdata;
  logic txlast;
  logic txvalid;
  logic [DATA_WIDTH/8-1:0] txkeep;
  logic txready;

  assign axis_data_bus.txd_tdata = txdata;
  assign axis_data_bus.txd_tlast = txlast;
  assign axis_data_bus.txd_tvalid = txvalid;
  assign axis_data_bus.txd_tkeep = txkeep;
  assign txready = axis_data_bus.txd_tready;


  // Register Reset
  logic [1:0] reset_reg = 2'b0;
  always_ff @(posedge clk_i) begin
    reset_reg <= {reset_reg[0], reset_i};
  end

  logic reset_dly;
  assign reset_dly = (reset_reg == 2'b10);


  // control signals
  logic transfer;
  logic [15:0] t_cnt;  // frame cnt
  logic [15:0] p_cnt;  // packet cnt
  logic t_done;
  logic p_done;

  logic cnt_done;
  logic cnt_dn_reg;
  logic loop_done;

  assign t_done = (t_cnt == packet_size_lp - 1'b1);  // aligns with txlast
  assign p_done = (p_cnt == MULTPKT_NUM - 1'b1);

  assign transfer = txready && txvalid;
  assign cnt_done = (transfer && t_done && p_done);

  assign loop_done = cnt_dn_reg;

  always_ff @(posedge clk_i) begin
    if(reset_i) begin
      p_cnt <= 16'h0000;
      t_cnt <= 16'h0000;
      cnt_dn_reg <= 1'b0;
      txkeep <= {DATA_WIDTH/8{1'b1}};
    end
    else begin
      // Increment counters
      t_cnt <= (transfer) ? (txlast ? 16'h0000 : (t_cnt + 1'b1)) : t_cnt;
      p_cnt <= (transfer && txlast) ? (p_done ? 16'h0000 : (p_cnt + 1'b1)) : p_cnt;

      cnt_dn_reg <= (cnt_done) ? 1'b1 : 1'b0;
    end
  end


  // TXDATA
  genvar i;
  generate
    for(i=0; i<DATA_WIDTH/8; i=i+1) begin: txdata_incr_gen
      always_ff @(posedge clk_i) begin
        if(reset_i) begin
          txdata[8*i+:8] <= 8'h00;
        end
        else begin
          txdata[8*i+:8] <= (transfer) ? txdata[8*i+:8] + 1'b1 : txdata[8*i+:8];
        end
      end
    end
  endgenerate


  // TXVALID
  always_ff @(posedge clk_i) begin
    if(reset_i) begin
      txvalid <= 1'b0;
    end
    else
    begin
      if(~en_i) begin
        txvalid <= 1'b0;
      end
      else if(reset_dly) begin
        txvalid <= 1'b1;
      end
      else begin
        txvalid <= txvalid;
      end
    end
  end


  // TXLAST: indicate the boundary of a packet, must be preservd between master and slave
  always_ff @(posedge clk_i) begin
    if(reset_i) begin
      txlast <= 1'b0;
    end
    else
    begin
      if(reset_dly) begin
        txlast <= 1'b1;
      end
      else if((p_cnt >= (single_num_lp - 1'b1)) && transfer && txlast) begin
        txlast <= 1'b0;
      end
      else if(t_cnt == (packet_size_lp - 16'd2) && transfer) begin
        txlast <= 1'b1;
      end
      else begin
        txlast <= txlast;
      end
    end
  end

endmodule // cl_axis_test_master