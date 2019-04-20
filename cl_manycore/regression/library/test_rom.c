#include "test_rom.h"

int compare_word(int size, char *desc[], uint32_t *expected, uint32_t *actual) {
  int rc = 0;
  for(int i = 0; i < size; i++) {
    bsg_pr_test_info("%s: Expected = %d, Actual = %d\n", desc[i], expected[i], actual[i]);
    if(expected[i] != actual[i]) {
      printf("\033[31m Failed \033[0m\n");
      rc = 1;
    }
    printf("\033[033m Succeeded \033[0m\n");
  }
  if(rc)
    return HB_MC_FAIL;
  else
    return HB_MC_SUCCESS;
}

int read_npa_rom(uint8_t fd, int idx, int num, /* out */ uint32_t *result) {
  hb_mc_response_packet_t buf[num];
  int read = hb_mc_copy_from_epa(fd, &buf[0], 0, 0, idx, num);
  if (read == HB_MC_SUCCESS) {
    for (int i=0; i<num; i++) {
      result[i] = hb_mc_response_packet_get_data(&buf[i]);
      bsg_pr_test_info("Read rom data from address 0x%x: 0x%x\n", idx + i, result[i]); 
    }
    return HB_MC_SUCCESS;
  }
  bsg_pr_test_info("Read from ROM failed.\n");
  return HB_MC_FAIL;
}


int read_axi_rom(uint8_t fd, int idx, int num, /* out */ uint32_t *result) {
  for (int i=0; i<num; i++) {
    result[i] = hb_mc_get_axi_rom(fd, (idx + i)<<2);
  }
  return HB_MC_SUCCESS;
}


int test_rom () {
  uint8_t fd = 0;

  // In order, at offset 4, the elements of the array are:
  // NETWORK_DIM_X, NETWORK_DIM_Y, HOST_INTERFACE_COORD_X, HOST_INTERFACE_COORD_Y
  const char *desc[4] = { "Network X Dimension", "Network Y Dimension", "Host X Coordinate", "Host Y Coordinate" };
  uint32_t expected[4] = { CL_MANYCORE_DIM_X, CL_MANYCORE_DIM_Y, 0, CL_MANYCORE_DIM_Y - 1 };
  uint32_t actual[4] = {};
  read_axi_rom(fd, 4, 4, actual); 
  printf("get host credits: 0x%x\n", hb_mc_get_host_credits(fd));
  bsg_pr_test_info("Comparing AXI space results:\n");
  compare_word(4, desc, expected, actual);
  printf("Read RCV_FIFO_MC_RES: %x\n", hb_mc_get_recv_vacancy(fd));
  
  hb_mc_init_host(&fd);

  bsg_pr_test_info("Readback manycore link monitor register\n");
  uint32_t recv_vacancy = hb_mc_get_recv_vacancy(fd);
  bsg_pr_test_info("Recv vacancy is: %x\n", recv_vacancy);
  bsg_pr_test_info("Readback ROM from tile (%d, %d)\n", 0, 0);

  if(read_npa_rom(fd, 4, 4, actual) == HB_MC_FAIL) {
    return HB_MC_FAIL;
  }
  bsg_pr_test_info("Comparing NPA space results:\n");
  compare_word(4, desc, expected, actual);

  return HB_MC_SUCCESS;
}

#ifdef COSIM
void test_main(uint32_t *exit_code) { 
  bsg_pr_test_info("test_rom Regression Test (COSIMULATION)\n");
  int rc = test_rom();
  *exit_code = rc;
  bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
  return;
}
#else
int main() {
  bsg_pr_test_info("test_rom Regression Test (F1)\n");
  int rc = test_rom();
  bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
  return rc;
}
#endif

