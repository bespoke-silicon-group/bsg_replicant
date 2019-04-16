#include "spmd_tests.h"
#include "test_bsg_dram_loopback_cache.h"

int test_loopback () {
	uint8_t fd;
	if (hb_mc_init_host(&fd) != HB_MC_SUCCESS) {
		printf("failed to initialize host.\n");
		return HB_MC_FAIL;
	}

	uint8_t x = 0, y = 1;
	hb_mc_freeze(fd, 0, 1);	
	hb_mc_set_tile_group_origin(fd, 0, 1, 0, 1);
	
	hb_mc_init_cache_tag(fd, 0, 5);
	hb_mc_init_cache_tag(fd, 1, 5);
	hb_mc_init_cache_tag(fd, 2, 5);
	hb_mc_init_cache_tag(fd, 3, 5);

	hb_mc_freeze(fd, 0, 1);
	
	hb_mc_set_tile_group_origin(fd, 0, 1, 0, 1);

	char* bsg_manycore_dir = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/"
		"bsg_dram_loopback_cache/main.riscv";

	hb_mc_load_binary(fd, bsg_manycore_dir, &x, &y, 1);

  	hb_mc_unfreeze(fd, 0, 1);

	printf("BSG INFO: Checking receive packet...\n");
	usleep(100); /* 100 us */
	hb_mc_packet_t manycore_finish;
	hb_mc_read_fifo(fd, 1, &manycore_finish);	

	uint32_t addr = hb_mc_request_packet_get_addr(&manycore_finish.request);
	uint32_t data = hb_mc_request_packet_get_data(&manycore_finish.request);
	uint32_t op_ex = hb_mc_request_packet_get_op_ex(&manycore_finish.request);
	uint32_t x_src = hb_mc_request_packet_get_x_src(&manycore_finish.request);
	uint32_t y_src = hb_mc_request_packet_get_y_src(&manycore_finish.request);
	uint32_t x_dst = hb_mc_request_packet_get_x_dst(&manycore_finish.request);
	uint32_t y_dst = hb_mc_request_packet_get_y_dst(&manycore_finish.request);
	uint32_t op = hb_mc_request_packet_get_op(&manycore_finish.request);
	printf("BSG INFO: Manycore finish packet received at Address 0x%x at coordinates (0x%x, 0x%x) from (0x%x, 0x%x). Operation: 0x%x, Data: 0x%x\n", addr, x_dst, y_dst, x_src, y_src, op, data & op_ex);
	if (addr == 0x3ab4)
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;
}


#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	printf("BSG INFO: test_bsg_dram_loopback_cache Regression Test (COSIMULATION)\n");
	int rc = test_loopback();
	*exit_code = rc;
	if (rc == HB_MC_SUCCESS)
		printf("BSG REGRESSION TEST PASSED\n");
	else
		printf("BSG REGRESSION TEST FAILED\n");
	return;
}
#else
int main() {
	printf("BSG INFO: test_bsg_dram_loopback_cache Regression Test (F1)\n");
	int rc = test_loopback();
	if (rc == HB_MC_SUCCESS)
		printf("BSG REGRESSION TEST PASSED\n");
	else
		printf("BSG REGRESSION TEST FAILED\n");
	return rc;
}
#endif

