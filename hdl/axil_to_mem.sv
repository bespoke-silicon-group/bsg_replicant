/**
 * axil_config.sv
 *
 */

// Note: awaddr[mem_addr_width_p+:4] is the address select. from 0 to 15

`include "bsg_axi_bus_pkg.vh"

module axil_to_mem #(
  mem_addr_width_p = "inv"
  ,axil_base_addr_p = "inv" // 32'hxxxx_xxxx
  ,axil_base_addr_width_p = "inv"
  ,axil_mosi_bus_width_lp = `bsg_axil_mosi_bus_width(1)
  ,axil_miso_bus_width_lp = `bsg_axil_miso_bus_width(1)) (
  input clk_i
  ,input reset_i
  ,input [axil_mosi_bus_width_lp-1:0] s_axil_bus_i
  ,output [axil_miso_bus_width_lp-1:0] s_axil_bus_o

  ,output [mem_addr_width_p-1:0] addr_o
  ,output wen_o
  ,output [31:0] data_o
  ,output ren_o
  ,input [31:0] data_i
  ,output done
);  

// synopsys translate_off
always_ff @(posedge clk_i)
begin
  assert (!(mem_addr_width_p > axil_base_addr_width_p) || (axil_base_addr_width_p > 32-4))
    else $error("config address width should not exceed axil base address width");
end
// synopsys translate_on

`declare_bsg_axil_bus_s(1, bsg_axil_mosi_bus_s, bsg_axil_miso_bus_s);
bsg_axil_mosi_bus_s s_axil_bus_i_cast;
bsg_axil_miso_bus_s s_axil_bus_o_cast;

assign s_axil_bus_i_cast = s_axil_bus_i;
assign s_axil_bus_o = s_axil_bus_o_cast;

// ----------------------------------------------------------------------------
// AXIL INTERFACE
// ----------------------------------------------------------------------------
// control from reg config stage
logic cfg_addr_v; // decide to accept rdata and enable write
logic cfg_done  ; // data transfer finish signal
logic cfg_wr_sel; // write transaction select

// data from reg config stage
logic [31:0] cfg_rdata;
logic [31:0] cfg_raddr;
logic [31:0] cfg_waddr;

logic [ mem_addr_width_p-1:0] cfg_addr_q ;
logic [31:0] cfg_wdata_q;



logic cfg_wen, cfg_ren;
logic cfg_2cp_wen, cfg_2cp_ren;

assign done = cfg_data_done;
assign data_o = cfg_wdata_q;
assign addr_o = cfg_addr_q;
assign cfg_rdata = data_i;
assign wen_o = cfg_2cp_wen;
assign ren_o = cfg_2cp_ren;

// ----------------
// axil signals
// ----------------
// axil write
logic [31:0] axil_awaddr_li;
logic [31:0] axil_wdata_li;
logic axil_awvalid_li;
logic axil_wvalid_li;
assign axil_awaddr_li = s_axil_bus_i_cast.awaddr;
assign axil_wdata_li = s_axil_bus_i_cast.wdata;
assign axil_awvalid_li = s_axil_bus_i_cast.awvalid;
assign axil_wvalid_li = s_axil_bus_i_cast.wvalid;

logic axil_awready_lo, axil_wready_lo;
assign s_axil_bus_o_cast.awready = axil_awready_lo;
assign s_axil_bus_o_cast.wready = axil_wready_lo;

// axil read
logic [31:0] axil_araddr_li;
logic axil_arvalid_li;
logic axil_rready_li;
assign axil_araddr_li = s_axil_bus_i_cast.araddr;
assign axil_arvalid_li = s_axil_bus_i_cast.arvalid;
assign axil_rready_li = s_axil_bus_i_cast.rready;

logic [31:0] axil_rdata_lo;
logic axil_arready_lo;
logic axil_rvalid_lo;
assign s_axil_bus_o_cast.rdata = axil_rdata_lo;
assign s_axil_bus_o_cast.arready = axil_arready_lo;
assign s_axil_bus_o_cast.rvalid = axil_rvalid_lo;
assign s_axil_bus_o_cast.rresp = 2'b00;

// axil bus
logic axil_bready_li;
assign axil_bready_li = s_axil_bus_i_cast.bready;

logic axil_bvalid_lo;
assign s_axil_bus_o_cast.bvalid = axil_bvalid_lo;
assign s_axil_bus_o_cast.bresp = 2'b00;


// axil config interface
typedef enum logic[2:0] {
   AXIL_IDLE = 0,
   AXIL_ADDR = 1,
   AXIL_DATA = 2,
   AXIL_RESP = 3
   } axil_state_e;

axil_state_e axil_state_r, axil_state_n;

// -------------
// DATA PATH
// -------------
// write AND read address
always_ff @(posedge clk_i)
  if (reset_i) begin
    {cfg_raddr, cfg_waddr} <= 64'd0;
  end
  else if ((axil_state_r == AXIL_IDLE) && axil_awvalid_li) begin
    cfg_waddr <= axil_awaddr_li;
  end
  else if ((axil_state_r == AXIL_IDLE) && axil_arvalid_li) begin
    cfg_raddr <= axil_araddr_li;
  end

// write data
always_ff @(posedge clk_i)
   if (reset_i)
     cfg_wdata <= '0;
   else
     cfg_wdata <= axil_wdata_li;

// read data
always_ff @(posedge clk_i)
  if (reset_i)
    axil_rdata_lo <= 0;
  else if (cfg_done)
    axil_rdata_lo <= cfg_addr_v ? cfg_rdata : 32'hdead_beef;


// -------------
// control logic
// -------------
assign axil_rvalid_lo = (axil_state_r==AXIL_RESP) && !cfg_wr_sel;
assign axil_bvalid_lo = (axil_state_r==AXIL_RESP) && cfg_wr_sel;

wire axil_b_r_ready = (cfg_wr_sel) ? axil_bready_li : axil_rready_li; // bus OR read ready signal

// write ready
always_ff @(posedge clk_i)
  if (reset_i) begin
    axil_awready_lo <= 0;
    axil_wready_lo  <= 0;
  end
  else begin
    // CHECK: we assume that awvalid signal will never be deasserted during the AXIL_ADDR cycle
    axil_awready_lo <= (axil_state_r==AXIL_IDLE) && (axil_awvalid_li);
    axil_wready_lo  <= ((axil_state_r==AXIL_DATA) && (cfg_done)) && cfg_wr_sel;
  end

// read ready
always_ff @(posedge clk_i)
  if (reset_i) begin  // read data is always ready
    axil_arready_lo <= 0;
  end
  else begin
    axil_arready_lo <= ((axil_state_r==AXIL_DATA) && (cfg_done)) && ~cfg_wr_sel;
  end


// state machine
always_ff @(posedge clk_i)
   if (reset_i)
      axil_state_r <= AXIL_IDLE;
   else
      axil_state_r <= axil_state_n;

always_comb
  begin : axil_state_control
    axil_state_n = axil_state_r;
    if (reset_i)
      axil_state_n = AXIL_IDLE;
    else begin
      case (axil_state_r)

        AXIL_IDLE :
          begin
            if (axil_awvalid_li)
              axil_state_n = AXIL_ADDR;
            else if (axil_arvalid_li)
              axil_state_n = AXIL_DATA;
            else
              axil_state_n = AXIL_IDLE;
          end

        AXIL_ADDR :
          begin
            axil_state_n = AXIL_DATA;
          end

        AXIL_DATA :
          begin
            if (cfg_done)
              axil_state_n = AXIL_RESP;
            else
              axil_state_n = AXIL_DATA;
          end

        AXIL_RESP :
          begin
            if (axil_b_r_ready)
              axil_state_n = AXIL_IDLE;
            else
              axil_state_n = AXIL_RESP;
          end

      endcase
    end
  end
// END OF AXIL INTERFACE


// -------------
// data path
// -------------
logic [ mem_addr_width_p-1:0] cfg_addr;
logic [31:0] cfg_wdata  ;
wire [31:0] cfg_sel_addr = cfg_wr_sel ? cfg_waddr : cfg_raddr;

always_ff @(posedge clk_i)
  if (reset_i)
    cfg_addr <= '0;
  else
    cfg_addr <= cfg_sel_addr[mem_addr_width_p-1:0];

always @(posedge clk_i)
  if (cfg_wen||cfg_ren) begin
    cfg_addr_q  <= cfg_addr;
    cfg_wdata_q <= cfg_wdata;
  end


// -------------
// control logic
// -------------
assign cfg_addr_v = (cfg_sel_addr[axil_base_addr_width_p+:4]==axil_base_addr_p[axil_base_addr_width_p+:4]);

// cfg write select
always_ff @(posedge clk_i)
   if (reset_i)
      cfg_wr_sel <= 1'b0;
   else if (axil_state_r==AXIL_IDLE)
      cfg_wr_sel <= axil_awvalid_li;

// cfg data valid (read is always valid in REGISTER MODULE)
wire cfg_data_v_li = (cfg_wr_sel)? axil_wvalid_li : 1'b1; 

logic cfg_in_process;
always_ff @(posedge clk_i)
  if (reset_i) begin
    cfg_wen <= 0;
    cfg_ren <= 0;
  end
  else begin
    cfg_wen <= cfg_addr_v ? ((axil_state_r==AXIL_DATA) & cfg_data_v_li & cfg_wr_sel & !cfg_in_process) : 1'b0;
    cfg_ren <= cfg_addr_v ? ((axil_state_r==AXIL_DATA) & cfg_data_v_li & !cfg_wr_sel & !cfg_in_process) : 1'b0;
  end

always_ff @(posedge clk_i)
  if (reset_i) begin
    cfg_in_process <= 0;
  end
  else if (axil_state_r==AXIL_IDLE) begin
    cfg_in_process <= 0;
  end
  else if (cfg_wen || cfg_ren) begin
    cfg_in_process <= 1;
  end

logic cfg_data_done;
always @(posedge clk_i)
   if (reset_i) begin
      cfg_2cp_wen <= 0;
      cfg_2cp_ren <= 0;
   end
   else begin
      cfg_2cp_wen <= cfg_wen || (cfg_2cp_wen && !cfg_data_done);
      cfg_2cp_ren <= cfg_ren || (cfg_2cp_ren && !cfg_data_done);
   end

always_ff @(posedge clk_i)
   if (reset_i)
      cfg_data_done <= 0;
   else
      cfg_data_done <= ((cfg_2cp_wen||cfg_2cp_ren) && !cfg_data_done); 

assign cfg_done = cfg_addr_v ? cfg_data_done : 1'b1;


endmodule
