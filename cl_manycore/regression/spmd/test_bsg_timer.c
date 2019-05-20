#include "test_bsg_timer.h"

int test_bsg_timer() {
	uint8_t fd;
	if (hb_mc_fifo_init(&fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "test_timer(): failed to initialize host.\n");
		return HB_MC_FAIL;
	}

	char* program = BSG_STRINGIFY(BSG_MANYCORE_DIR) "/software/spmd/" "bsg_timer/main.riscv";
	uint8_t x = 0, y = 1;
	hb_mc_tile_freeze(fd, 0, 1);
	hb_mc_tile_set_group_origin(fd, 0, 1, 0, 1);
	bsg_pr_test_info("file to be loaded is %s\n", program);	
		
	if (hb_mc_load_binary(fd, program, &x, &y, 1) != HB_MC_SUCCESS) 
		return HB_MC_FAIL; /* could not load binary */

	hb_mc_tile_unfreeze(fd, 0, 1);

	bsg_pr_test_info("Collecting packets...\n");
	
	hb_mc_request_packet_t mc_req_pkt;
	while(1) {
		hb_mc_fifo_receive(fd, HB_MC_FIFO_RX_REQ, &mc_req_pkt);
  	uint32_t addr = hb_mc_request_packet_get_addr(&mc_req_pkt);
  	uint32_t data = hb_mc_request_packet_get_data(&mc_req_pkt);
  	uint32_t mask = hb_mc_request_packet_get_mask(&mc_req_pkt);
  	uint32_t x_src = hb_mc_request_packet_get_x_src(&mc_req_pkt);
  	uint32_t y_src = hb_mc_request_packet_get_y_src(&mc_req_pkt);
  	uint32_t x_dst = hb_mc_request_packet_get_x_dst(&mc_req_pkt);
  	uint32_t y_dst = hb_mc_request_packet_get_y_dst(&mc_req_pkt);
  	uint32_t op = hb_mc_request_packet_get_op(&mc_req_pkt);

		bsg_pr_test_info("Manycore finish packet received at Address 0x%x at coordinates (0x%x, 0x%x) from (0x%x, 0x%x). Operation: 0x%x, Data: 0x%x\n", addr, x_dst, y_dst, x_src, y_src, op, data);

		if (addr == 0x3ab4) {
			return HB_MC_SUCCESS;
		}
	}
}


#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info("test_bsg_timer Regression Test (COSIMULATION)\n");
	int rc = test_bsg_timer();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_bsg_timer Regression Test (F1)\n");
	int rc = test_bsg_timer();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif

