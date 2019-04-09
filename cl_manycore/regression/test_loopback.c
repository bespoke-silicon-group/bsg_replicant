#include "manycore_tests.h"
#include "test_loopback.h"

int test_loopback () {

	printf("Running the Manycore-Cache-Loopback test on a 4x4.\n\n");

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

	char test_file[100]; 
	char* bsg_maycore_dir;
	bsg_maycore_dir = getenv("BSG_MANYCORE_DIR");
	strcpy(test_file, bsg_maycore_dir);
	strcat(test_file, "/software/spmd/bsg_dram_loopback_cache/main.riscv");
	printf("%s\n", test_file);
	
	hb_mc_load_binary(fd, test_file, &x, &y, 1);

  	hb_mc_unfreeze(fd, 0, 1);

	printf("Checking receive packet...\n");
	usleep(100); /* 100 us */
	hb_mc_request_packet_t manycore_finish;
	hb_mc_read_fifo(fd, 1, &manycore_finish);	

	uint32_t addr = hb_mc_request_packet_get_addr(&manycore_finish);
	uint32_t data = hb_mc_request_packet_get_data(&manycore_finish);
	uint32_t op_ex = hb_mc_request_packet_get_op_ex(&manycore_finish);
	uint32_t x_src = hb_mc_request_packet_get_x_src(&manycore_finish);
	uint32_t y_src = hb_mc_request_packet_get_y_src(&manycore_finish);
	uint32_t x_dst = hb_mc_request_packet_get_x_dst(&manycore_finish);
	uint32_t y_dst = hb_mc_request_packet_get_y_dst(&manycore_finish);
	uint32_t op = hb_mc_request_packet_get_op(&manycore_finish);
	printf("Manycore finish packet received at Address 0x%x at coordinates (0x%x, 0x%x) from (0x%x, 0x%x). Operation: 0x%x, Data: 0x%x\n", addr, x_dst, y_dst, x_src, y_src, op, data & op_ex);
	if (addr == 0x3ab4)
		return HB_MC_SUCCESS;
	else
		return HB_MC_FAIL;
 }


#ifdef COSIM
	void test_main(int *exit_code) {	
		printf("Regression Test on COSIMULATION:\n\n");
		int rc = test_loopback();
		*exit_code = rc;
		if (rc == HB_MC_SUCCESS)
			printf("TEST PASSED~~~\n");
		else
			printf("TEST FAILED!!!\n");
		return;
	}
#else
	int main() {
		printf("Regression Test on F1:\n\n");
		int rc = test_loopback();
		if (rc == HB_MC_SUCCESS)
			printf("TEST PASSED~~~\n");
		else
			printf("TEST FAILED!!!\n");
		return rc;
	}
#endif

