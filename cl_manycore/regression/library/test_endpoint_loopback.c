#include "test_endpoint_loopback.h"

#define X_CORD  0
#define Y_CORD  1
#define ADDR    0x1234
#define DATA    0xABCD

int test_endpoint_loopback() {
  int rc = 0;
  uint8_t fd = 0;

  // test AXI space register
  const char *desc[4] = { "Network X Dimension", "Network Y Dimension", "Host X Coordinate", "Host Y Coordinate" };
  uint32_t result[4];
  uint32_t expected[4] = { CL_MANYCORE_DIM_X, CL_MANYCORE_DIM_Y, CL_MANYCORE_DIM_X - 1, 0 };
  rc = hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_DIM_X, &result[0]);
  if(rc == HB_MC_FAIL) return rc;
  rc = hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_DIM_Y, &result[1]);
  if(rc == HB_MC_FAIL) return rc;
  rc = hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_X, &result[2]);
  if(rc == HB_MC_FAIL) return rc;
  rc = hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_Y, &result[3]);
  if(rc == HB_MC_FAIL) return rc;

  rc = compare_results(4, desc, expected, result);
    if(rc != HB_MC_SUCCESS){
      return HB_MC_FAIL;
  }

  // test loopback through manycore_endpoint
  hb_mc_packet_t tx_packets;
  hb_mc_packet_t rx_packets;

  uint32_t addr = ADDR;  // NPA
  uint32_t data = DATA;
  hb_mc_format_request_packet(fd, &tx_packets.request, addr, data, X_CORD, Y_CORD, HB_MC_PACKET_OP_REMOTE_STORE);

  if (hb_mc_fifo_transmit(fd, HB_MC_FIFO_TX_REQ, &tx_packets) == HB_MC_FAIL) return HB_MC_FAIL;
  bsg_pr_test_info("Write to manycore_endpoint successful.\n");
  bsg_pr_test_info("Requeset packet : data = %x addr = %x coord = (%d,%d).\n", data, addr, X_CORD, Y_CORD);

  if (hb_mc_fifo_receive(fd, HB_MC_FIFO_RX_REQ, (hb_mc_packet_t *) &rx_packets) == HB_MC_FAIL) return HB_MC_FAIL;
  uint32_t rdata = hb_mc_request_packet_get_data(&rx_packets.request);
  uint32_t raddr = hb_mc_request_packet_get_addr(&rx_packets.request);
  uint32_t x_src = hb_mc_request_packet_get_x_src(&rx_packets.request);
  uint32_t y_src = hb_mc_request_packet_get_y_src(&rx_packets.request);
  uint32_t x_dst = hb_mc_request_packet_get_x_dst(&rx_packets.request);
  uint32_t y_dst = hb_mc_request_packet_get_y_dst(&rx_packets.request);
  bsg_pr_test_info("Read from manycore_endpoint successful.\n");
  bsg_pr_test_info("RAW=-- data = %x  addr = %x src_coord = (%d,%d) dst_coord = (%d,%d).\n",  rdata, raddr, x_src, y_src, x_dst, y_dst);

  return HB_MC_SUCCESS;

}

#ifdef COSIM
void test_main(uint32_t *exit_code) {
  bsg_pr_test_info("test_endpoint_loopback Regression Test (COSIMULATION)\n");
  int rc = test_endpoint_loopback();
  *exit_code = rc;
  bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
  return;
}
#else
int main() {
  bsg_pr_test_info("test_endpoint_loopback Regression Test is not supported in (F1)\n");
  return HB_MC_SUCCESS;
}
#endif
