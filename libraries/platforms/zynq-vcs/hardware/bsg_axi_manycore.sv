module bsg_axi_manycore
  import bsg_manycore_pkg::*;
  import bsg_bladerunner_pkg::*;      
  #(
   parameter integer C_S00_AXI_DATA_WIDTH = 32,
   parameter integer C_S00_AXI_ADDR_WIDTH = 6

   , parameter link_sif_width_p = 
   `bsg_manycore_link_sif_width 
   (bsg_machine_noc_epa_width_gp
    ,bsg_machine_noc_data_width_gp
    ,bsg_machine_noc_coord_x_width_gp
    ,bsg_machine_noc_coord_y_width_gp
    )
   )
  (
   //  AXI Slave bus interface
    input wire                                  s00_axi_aclk_i,
    input wire                                  s00_axi_aresetn_i,
    input wire [C_S00_AXI_ADDR_WIDTH-1 : 0]     s00_axi_awaddr_i,
    input wire [2 : 0]                          s00_axi_awprot_i,
    input wire                                  s00_axi_awvalid_i,
    output wire                                 s00_axi_awready_o,
    input wire [C_S00_AXI_DATA_WIDTH-1 : 0]     s00_axi_wdata_i,
    input wire [(C_S00_AXI_DATA_WIDTH/8)-1 : 0] s00_axi_wstrb_i,
    input wire                                  s00_axi_wvalid_i,
    output wire                                 s00_axi_wready_o,
    output wire [1 : 0]                         s00_axi_bresp_o,
    output wire                                 s00_axi_bvalid_o,
    input wire                                  s00_axi_bready_i,
    input wire [C_S00_AXI_ADDR_WIDTH-1 : 0]     s00_axi_araddr_i,
    input wire [2 : 0]                          s00_axi_arprot_i,
    input wire                                  s00_axi_arvalid_i,
    output wire                                 s00_axi_arready_o,
    output wire [C_S00_AXI_DATA_WIDTH-1 : 0]    s00_axi_rdata_o,
    output wire [1 : 0]                         s00_axi_rresp_o,
    output wire                                 s00_axi_rvalid_o,
    input wire                                  s00_axi_rready_i

   // manycore control logic
   , input  clk_i
   , input  reset_i
   , input  reset_done_i

   // manycore I/O
   , input  [link_sif_width_p-1:0]              io_link_sif_i
   , output [link_sif_width_p-1:0]              io_link_sif_o

   // global x/y
   , input  [bsg_machine_noc_coord_x_width_gp-1:0] global_x_i
   , input  [bsg_machine_noc_coord_y_width_gp-1:0] global_y_i
   );

  ////////////////
  // Zynq shell //
  ////////////////
  
   localparam num_regs_ps_to_pl_lp = 4;  
   localparam num_fifo_ps_to_pl_lp = 2;
   localparam num_fifo_pl_to_ps_lp = 2;
   localparam num_regs_pl_to_ps_lp = 1+bsg_machine_rom_els_gp;

   wire [num_fifo_pl_to_ps_lp-1:0][C_S00_AXI_DATA_WIDTH-1:0] pl_to_ps_fifo_data_li;
   wire [num_fifo_pl_to_ps_lp-1:0]                           pl_to_ps_fifo_v_li;
   wire [num_fifo_pl_to_ps_lp-1:0]                           pl_to_ps_fifo_ready_lo;

   wire [num_fifo_ps_to_pl_lp-1:0][C_S00_AXI_DATA_WIDTH-1:0] ps_to_pl_fifo_data_lo;
   wire [num_fifo_ps_to_pl_lp-1:0]                           ps_to_pl_fifo_v_lo;
   wire [num_fifo_ps_to_pl_lp-1:0]                           ps_to_pl_fifo_yumi_li;

   wire [num_regs_ps_to_pl_lp-1:0][C_S00_AXI_DATA_WIDTH-1:0] csr_data_lo;
   wire [num_regs_pl_to_ps_lp-1:0][C_S00_AXI_DATA_WIDTH-1:0] csr_data_li;

   bsg_zynq_pl_shell
     #(
       .num_regs_ps_to_pl_p (num_regs_ps_to_pl_lp)
       ,.num_fifo_ps_to_pl_p(num_fifo_ps_to_pl_lp)
       ,.num_fifo_pl_to_ps_p(num_fifo_pl_to_ps_lp)
       ,.num_regs_pl_to_ps_p(num_regs_pl_to_ps_lp)
       ,.C_S_AXI_DATA_WIDTH (C_S00_AXI_DATA_WIDTH)
       ,.C_S_AXI_ADDR_WIDTH (C_S00_AXI_ADDR_WIDTH)
       ) bzps
       (
        .pl_to_ps_fifo_data_i  (pl_to_ps_fifo_data_li)
        ,.pl_to_ps_fifo_v_i    (pl_to_ps_fifo_v_li)
        ,.pl_to_ps_fifo_ready_o(pl_to_ps_fifo_ready_lo)

        ,.ps_to_pl_fifo_data_o (ps_to_pl_fifo_data_lo)
        ,.ps_to_pl_fifo_v_o    (ps_to_pl_fifo_v_lo)
        ,.ps_to_pl_fifo_yumi_i (ps_to_pl_fifo_yumi_li)

        ,.csr_data_o(csr_data_lo)
        ,.csr_data_i(csr_data_li)
        ,.S_AXI_ACLK   (s00_axi_aclk_i   )
        ,.S_AXI_ARESETN(s00_axi_aresetn_i)
        ,.S_AXI_AWADDR (s00_axi_awaddr_i)
        ,.S_AXI_AWPROT (s00_axi_awprot_i )
        ,.S_AXI_AWVALID(s00_axi_awvalid_i)
        ,.S_AXI_AWREADY(s00_axi_awready_o)
        ,.S_AXI_WDATA  (s00_axi_wdata_i  )
        ,.S_AXI_WSTRB  (s00_axi_wstrb_i  )
        ,.S_AXI_WVALID (s00_axi_wvalid_i )
        ,.S_AXI_WREADY (s00_axi_wready_o )
        ,.S_AXI_BRESP  (s00_axi_bresp_o  )
        ,.S_AXI_BVALID (s00_axi_bvalid_o )
        ,.S_AXI_BREADY (s00_axi_bready_i )
        ,.S_AXI_ARADDR (s00_axi_araddr_i )
        ,.S_AXI_ARPROT (s00_axi_arprot_i )
        ,.S_AXI_ARVALID(s00_axi_arvalid_i)
        ,.S_AXI_ARREADY(s00_axi_arready_o)
        ,.S_AXI_RDATA  (s00_axi_rdata_o  )
        ,.S_AXI_RRESP  (s00_axi_rresp_o  )
        ,.S_AXI_RVALID (s00_axi_rvalid_o )
        ,.S_AXI_RREADY (s00_axi_rready_i )
        );  

  ///////////////////////
  // Configuration ROM //
  ///////////////////////
  logic [bsg_machine_rom_width_gp-1:0] rom [bsg_machine_rom_els_gp-1:0];
  assign rom = bsg_machine_rom_arr_gp;
  genvar k;  
  for (k = 0; k < bsg_machine_rom_els_gp; k++) begin
    assign csr_data_li[k+1] = rom[k];
  end
  assign csr_data_li[0] = {0, reset_done_i};

  ///////////////
  // manycore  //
  ///////////////  
  localparam ep_fifo_els_lp = 4;
  localparam fifo_width_lp = 128;
  
  // manycore requests -> host
  logic [fifo_width_lp-1:0] mc_req_lo;  
  logic mc_req_v_lo;
  logic mc_req_ready_li;

  // manycore responses -> host
  logic [fifo_width_lp-1:0] mc_rsp_lo;  
  logic mc_rsp_v_lo;
  logic mc_rsp_ready_li;    

  // host requests -> manycore
  logic [fifo_width_lp-1:0] endpoint_req_li;  
  logic endpoint_req_v_li;
  logic endpoint_req_ready_lo;

  // host responses -> manycore
  logic [fifo_width_lp-1:0] endpoint_rsp_li;  
  logic endpoint_rsp_v_li;
  logic endpoint_rsp_ready_lo;  
  
  bsg_manycore_endpoint_to_fifos
    #(
      .fifo_width_p(fifo_width_lp)
      ,.x_cord_width_p(bsg_machine_noc_coord_x_width_gp)
      ,.y_cord_width_p(bsg_machine_noc_coord_y_width_gp)
      ,.addr_width_p(bsg_machine_noc_epa_width_gp)
      ,.data_width_p(bsg_machine_noc_data_width_gp)
      ,.ep_fifo_els_p(ep_fifo_els_lp)
      ,.credit_counter_width_p(`BSG_WIDTH(bsg_machine_io_credits_max_gp))
      )
  eptofifo
    (.clk_i(clk_i)
     ,.reset_i(reset_i)

     ,.mc_req_o(mc_req_lo)
     ,.mc_req_v_o(mc_req_v_lo)
     ,.mc_req_ready_i(mc_req_ready_li)

     ,.mc_rsp_o(mc_rsp_lo)
     ,.mc_rsp_v_o(mc_rsp_v_lo)
     ,.mc_rsp_ready_i(mc_rsp_ready_li)

     ,.endpoint_req_i(endpoint_req_li)
     ,.endpoint_req_v_i(endpoint_req_v_li)
     ,.endpoint_req_ready_o(endpoint_req_ready_lo)
     
     ,.endpoint_rsp_i(endpoint_rsp_li)
     ,.endpoint_rsp_v_i(endpoint_rsp_v_li)
     ,.endpoint_rsp_ready_o(endpoint_rsp_ready_lo)

     ,.link_sif_i(io_link_sif_i)
     ,.link_sif_o(io_link_sif_o)

     ,.global_x_i(global_x_i)
     ,.global_y_i(global_y_i)
     );

  //////////
  // SIPO //
  //////////
  logic [num_fifo_ps_to_pl_lp-1:0] [fifo_width_lp-1:0] ps_to_pl_fifo_par_data_lo;
  logic [num_fifo_ps_to_pl_lp-1:0] ps_to_pl_fifo_par_v_lo;
  logic [num_fifo_ps_to_pl_lp-1:0] ps_to_pl_fifo_par_yumi_li;
  logic [num_fifo_ps_to_pl_lp-1:0] ps_to_pl_fifo_par_ready_lo;
  
  for (genvar k = 0; k < num_fifo_ps_to_pl_lp; k++) begin    
    bsg_serial_in_parallel_out_full
      #(.width_p(C_S00_AXI_DATA_WIDTH)
        ,.els_p(`BSG_CDIV(fifo_width_lp,C_S00_AXI_DATA_WIDTH)))
    sipo
       (.clk_i   (clk_i)
        ,.reset_i(reset_i)
        
        ,.v_i    (ps_to_pl_fifo_v_lo[k])
        ,.ready_o(ps_to_pl_fifo_par_ready_lo[k])
        ,.data_i (ps_to_pl_fifo_data_lo[k])

        ,.data_o(ps_to_pl_fifo_par_data_lo[k])
        ,.v_o   (ps_to_pl_fifo_par_v_lo[k])
        ,.yumi_i(ps_to_pl_fifo_par_yumi_li[k]));

    assign ps_to_pl_fifo_yumi_li[k] = ps_to_pl_fifo_v_lo[k] & ps_to_pl_fifo_par_ready_lo[k];
  end

  assign endpoint_req_li              = ps_to_pl_fifo_par_data_lo[0];
  assign endpoint_req_v_li            = ps_to_pl_fifo_par_v_lo[0];  
  assign ps_to_pl_fifo_par_yumi_li[0] = endpoint_req_v_li & endpoint_req_ready_lo;

  assign endpoint_rsp_li              = ps_to_pl_fifo_par_data_lo[0];
  assign endpoint_rsp_v_li            = ps_to_pl_fifo_par_v_lo[1];
  assign ps_to_pl_fifo_par_yumi_li[1] = endpoint_rsp_v_li & endpoint_rsp_ready_lo;

  for (genvar j = 0; j < 2; j++) begin
    always @(posedge ps_to_pl_fifo_v_lo[j]) begin
      $display("%M: ps_to_pl_fifo_v_lo[%d]=%b"
               , j
               , ps_to_pl_fifo_v_lo[j]);
      $display("%M: ps_to_pl_fifo_par_ready_lo[%d]=%b"
               , j
               , ps_to_pl_fifo_par_ready_lo[j]);      
    end

    always @(posedge ps_to_pl_fifo_par_v_lo[j]) begin
      $display("%M: ps_to_pl_fifo_par_v_lo[%d]=%b"
               , j
               , ps_to_pl_fifo_par_v_lo[j]);
      $display("%M: endpoint_req_ready_lo=%b"
               , j
               , endpoint_req_ready_lo);      
    end  
  end
  
  
  //////////
  // PISO //
  //////////
  logic [num_fifo_pl_to_ps_lp-1:0] [fifo_width_lp-1:0] pl_to_ps_fifo_par_data_li;
  logic [num_fifo_pl_to_ps_lp-1:0] pl_to_ps_fifo_par_v_li;  
  logic [num_fifo_pl_to_ps_lp-1:0] pl_to_ps_fifo_par_ready_lo;
  logic [num_fifo_pl_to_ps_lp-1:0] pl_to_ps_fifo_par_yumi_li; 
  for (genvar k = 0; k < num_fifo_pl_to_ps_lp; k++) begin
    bsg_parallel_in_serial_out
      #(.width_p(C_S00_AXI_DATA_WIDTH)
        ,.els_p(`BSG_CDIV(fifo_width_lp,C_S00_AXI_DATA_WIDTH)))
    piso
       (.clk_i   (clk_i)
        ,.reset_i(reset_i)
        
        ,.valid_i(pl_to_ps_fifo_par_v_li[k])
        ,.data_i(pl_to_ps_fifo_par_data_li[k])
        ,.ready_and_o(pl_to_ps_fifo_par_ready_lo[k])
        
        ,.valid_o(pl_to_ps_fifo_v_li[k])
        ,.data_o(pl_to_ps_fifo_data_li[k])
        ,.yumi_i(pl_to_ps_fifo_par_yumi_li[k]));

        assign pl_to_ps_fifo_par_yumi_li[k] = pl_to_ps_fifo_v_li[k] & pl_to_ps_fifo_ready_lo[k];        
  end
  
  // assign pl_to_ps_fifo_par_data_li[0]  = mc_req_lo;
  // assign pl_to_ps_fifo_par_v_li[0]     = mc_req_v_lo;
  // assign mc_req_ready_li               = pl_to_ps_fifo_par_ready_lo[0];
  
  // assign pl_to_ps_fifo_par_data_li[1]  = mc_rsp_lo;
  // assign pl_to_ps_fifo_par_v_li[1]     = mc_rsp_v_lo;
  // assign mc_rsp_ready_li               = pl_to_ps_fifo_par_ready_lo[1];

  // mc request fifo
  bsg_fifo_1r1w_small
    #(.width_p(fifo_width_lp)
      ,.els_p(bsg_machine_io_credits_max_gp))
  mc_req_buf
    (.clk_i(clk_i)
     ,.reset_i(reset_i)
     
     ,.v_i(mc_req_v_lo)
     ,.ready_o(mc_req_ready_li)
     ,.data_i(mc_req_lo)
     
     ,.v_o(pl_to_ps_fifo_par_v_li[0])
     ,.data_o(pl_to_ps_fifo_par_data_li[0])
     ,.yumi_i(pl_to_ps_fifo_par_ready_lo[0] & pl_to_ps_fifo_par_v_li[0]));

  // mc response fifo
  bsg_fifo_1r1w_small
    #(.width_p(fifo_width_lp)
      ,.els_p(bsg_machine_io_credits_max_gp))
  mc_rsp_buf
    (.clk_i(clk_i)
     ,.reset_i(reset_i)
     
     ,.v_i(mc_rsp_v_lo)
     ,.ready_o(mc_rsp_ready_li)
     ,.data_i(mc_rsp_lo)
     
     ,.v_o(pl_to_ps_fifo_par_v_li[1])
     ,.data_o(pl_to_ps_fifo_par_data_li[1])
     ,.yumi_i(pl_to_ps_fifo_par_ready_lo[1] & pl_to_ps_fifo_par_v_li[1]));  
  
endmodule

