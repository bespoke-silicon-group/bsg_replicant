#include "test_vcache_simplified.h"

#define WRITE_BLOCK_NUM 3
#define VCACHE_ADDR_WIDTH 9
#define EPA_ADDR_WIDTH 28 
#define BASE_ADDR 0x0000

int test_vcache_simplified() {
	srand(0);
	int error;
	uint8_t fd; 

	if (hb_mc_fifo_init(&fd) != HB_MC_SUCCESS) {
		fprintf(stderr, "test_dram_read_write(): failed to initialize host.\n");
		return HB_MC_FAIL;
	}

	uint32_t manycore_dim_x, manycore_dim_y; 
	error = hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_DIM_X, &manycore_dim_x); 
	if (error != HB_MC_SUCCESS) { 
		fprintf(stderr, "hb_mc_get_config(): failed to get manycore X dim.\n");
		return HB_MC_FAIL;
	}

	error = hb_mc_get_config(fd, HB_MC_CONFIG_DEVICE_DIM_Y, &manycore_dim_y); 
	if (error != HB_MC_SUCCESS) { 
		fprintf(stderr, "hb_mc_get_config(): failed to get manycore Y dim.\n");
		return HB_MC_FAIL;
	}

 	uint32_t dram_coord_x = 0;
	uint32_t dram_coord_y = manycore_dim_y + 1;
	int mismatch = 0; 


	// 1st reset the tag of the vcache under test
	uint32_t tag_reset_addr;
	uint32_t zeros;
	zeros = 0;
	for (int i = 0; i < 2; i ++) {
		tag_reset_addr = (1 << (EPA_ADDR_WIDTH - 2 + 1)) + ((i+1) << VCACHE_ADDR_WIDTH); // tagst command to invalidate the tag
		error = hb_mc_copy_to_epa(fd, dram_coord_x, dram_coord_y, BASE_ADDR + tag_reset_addr, &zeros, 1);
		if (error != HB_MC_SUCCESS) {
			fprintf(stderr, "hb_mc_copy_to_epa: failed to reset the vcache tag A[%d] at DRAM coord(%d,%d) addr %d.\n", i, dram_coord_x, dram_coord_y, BASE_ADDR + i);
		return HB_MC_FAIL;
		}
		else 
			bsg_pr_test_info("reset tag %d successed.\n", i);
	}


	// 2nd write to the same bank 3 times, so that the 3rd write will evict a cache line.	
	uint32_t A_host[WRITE_BLOCK_NUM];
	uint32_t A_device[WRITE_BLOCK_NUM];
	uint32_t epa_addr[WRITE_BLOCK_NUM];

	for (int i = 0; i < WRITE_BLOCK_NUM; i ++) {
		A_host[i] = 1 + i; // write none zero data
		epa_addr[i] = ((1 + i) << VCACHE_ADDR_WIDTH);  // write none zero address
	}

	for (int i = 0; i < WRITE_BLOCK_NUM; i ++) {	
		error = hb_mc_copy_to_epa(fd, dram_coord_x, dram_coord_y, BASE_ADDR + epa_addr[i], &A_host[i], 1);
		if (error != HB_MC_SUCCESS) {
			fprintf(stderr, "hb_mc_copy_to_epa: failed to write A[%d] = %d to DRAM coord(%d,%d) addr %d.\n", i, A_host[i], dram_coord_x, dram_coord_y, BASE_ADDR + i);
		return HB_MC_FAIL;
		}
	} 
	
	hb_mc_response_packet_t buf[1]; 
	for (int i = 0; i < WRITE_BLOCK_NUM; i ++) {
		error = hb_mc_copy_from_epa(fd, &buf[0], dram_coord_x, dram_coord_y, BASE_ADDR + epa_addr[i], 1); 
		if ( error != HB_MC_SUCCESS) {
			fprintf(stderr, "hb_mc_copy_from_epa: failed to read A[%d] from DRAM coord (%d,%d) addr %d.\n", i, dram_coord_x, dram_coord_y, BASE_ADDR + i);
			return HB_MC_FAIL;
		}
		A_device[i] = hb_mc_response_packet_get_data(&buf[0]);
	}

	bsg_pr_test_info("Checking vcache at (%d,%d):\n", dram_coord_x, dram_coord_y); 
	for (int i = 0; i < WRITE_BLOCK_NUM; i ++) { 
		if (A_host[i] == A_device[i]) {
			fprintf(stderr, "Success -- A_host[%d] = %d   ==   A_device[%d] = %d -- EPA: %d.\n", i, A_host[i], i, A_device[i], BASE_ADDR + i); 
		}
		else { 
			fprintf(stderr, "Failed -- A_host[%d] = %d   !=   A_device[%d] = %d -- EPA: %d.\n", i, A_host[i], i, A_device[i], BASE_ADDR + i);
			mismatch = 1;
		}
	}

	if (mismatch) 
		return HB_MC_FAIL;
	return HB_MC_SUCCESS;		
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info("test vcache simplified Regression Test (COSIMULATION)\n");
	int rc = test_vcache_simplified();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test vcache simplified Regression Test (F1)\n");
	int rc = test_vcache_simplified();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif
