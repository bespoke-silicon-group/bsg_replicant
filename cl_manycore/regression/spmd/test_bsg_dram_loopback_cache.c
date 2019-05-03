#include "test_bsg_dram_loopback_cache.h"

int test_loopback () {
	uint8_t fd;
	if (hb_mc_init_host(&fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "test_bsg_dram_loopback_cache(): failed to initialize host.\n");
		return HB_MC_FAIL;
	}

	uint8_t x = 0, y = 1;
	hb_mc_freeze(fd, 0, 1);	
	hb_mc_set_tile_group_origin(fd, 0, 1, 0, 1);
	bsg_pr_test_info("file to be loaded is %s\n", getenv("MAIN_LOOPBACK"));	
	
	
	char* bsg_manycore_dir = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/"
		"bsg_dram_loopback_cache/main.riscv";

	if (hb_mc_load_binary(fd, bsg_manycore_dir, &x, &y, 1) != HB_MC_SUCCESS) 
		return HB_MC_FAIL; /* could not load binary */

  	hb_mc_unfreeze(fd, 0, 1);

	bsg_pr_test_info("Checking receive packet...\n");
	usleep(100); /* 100 us */
	hb_mc_request_packet_t manycore_finish;
	hb_mc_fifo_receive(fd, HB_MC_FIFO_RX_REQ, &manycore_finish);	

	uint32_t addr = hb_mc_request_packet_get_addr(&manycore_finish);
	uint32_t data = hb_mc_request_packet_get_data(&manycore_finish);
	uint32_t mask = hb_mc_request_packet_get_mask(&manycore_finish);
	uint32_t x_src = hb_mc_request_packet_get_x_src(&manycore_finish);
	uint32_t y_src = hb_mc_request_packet_get_y_src(&manycore_finish);
	uint32_t x_dst = hb_mc_request_packet_get_x_dst(&manycore_finish);
	uint32_t y_dst = hb_mc_request_packet_get_y_dst(&manycore_finish);
	uint32_t op = hb_mc_request_packet_get_op(&manycore_finish);
	bsg_pr_test_info("Manycore finish packet received at Address 0x%x at coordinates (0x%x, 0x%x) from (0x%x, 0x%x). Operation: 0x%x, Data: 0x%x\n", addr, x_dst, y_dst, x_src, y_src, op, data);

	if (hb_mc_host_finish(fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "test_bsg_dram_loopback_cache(): failed to terminate host.\n");
		return HB_MC_FAIL;
	}


	if (addr == 0x3ab4)
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;
}


#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info("test_bsg_dram_loopback_cache Regression Test (COSIMULATION)\n");
	int rc = test_loopback();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_bsg_dram_loopback_cache Regression Test (F1)\n");
	int rc = test_loopback();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

