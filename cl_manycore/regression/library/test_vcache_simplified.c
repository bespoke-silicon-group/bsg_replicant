#include <bsg_manycore.h>
#include "test_vcache_simplified.h"

#define WRITE_BLOCK_NUM 3
#define VCACHE_ADDR_WIDTH 9
#define EPA_ADDR_WIDTH 28 
#define BASE_ADDR 0x0000

int test_vcache_simplified() {
	hb_mc_manycore_t manycore = {0}, *mc = &manycore;
	int err, r = HB_MC_FAIL;

	srand(0);

	err = hb_mc_manycore_init(mc, "test_vcache_simplified", 0);
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to init manycore: %s\n",
			   __func__, hb_mc_strerror(err));
		return err;
	}

	const hb_mc_config_t *config = hb_mc_manycore_get_config(mc);
	uint32_t manycore_dim_x = hb_mc_coordinate_get_x(hb_mc_config_get_dimension(config));
	uint32_t manycore_dim_y = hb_mc_coordinate_get_y(hb_mc_config_get_dimension(config));

	uint32_t dram_coord_x = 0;
	uint32_t dram_coord_y = manycore_dim_y + 1;
	int mismatch = 0; 

	uint32_t tag_reset_addr;
	uint32_t zeros = 0;

         // 1st reset the tag of the vcache under test
	for (int i = 0; i < 2; i ++) {
		tag_reset_addr = (1 << (EPA_ADDR_WIDTH - 2 + 1)) + ((i+1) << VCACHE_ADDR_WIDTH); // tagst command to invalidate the tag
		npa_t npa = { .x = dram_coord_x, .y = dram_coord_y, .epa = BASE_ADDR + tag_reset_addr };
		err = hb_mc_manycore_write_mem(mc, &npa, &zeros, sizeof(zeros));
		if (err != HB_MC_SUCCESS) {
			bsg_pr_err("hb_mc_copy_to_epa: failed to reset the vcache tag A[%d] at DRAM coord(%d,%d) addr %d.\n", i, dram_coord_x, dram_coord_y, BASE_ADDR + i);
			goto cleanup;
		} else {
			bsg_pr_test_info("reset tag %d succesfully\n", i);
		}
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
		npa_t npa = { .x = dram_coord_x, .y = dram_coord_y, .epa = BASE_ADDR + epa_addr[i] };
		err = hb_mc_manycore_write_mem(mc, &npa, &A_host[i], sizeof(A_host[i]));
		if (err != HB_MC_SUCCESS) {
			bsg_pr_err("hb_mc_copy_to_epa: failed to write A[%d] = %d to DRAM coord(%d,%d) addr %d.\n", i, A_host[i], dram_coord_x, dram_coord_y, BASE_ADDR + i);
			goto cleanup;
		}
	}
	
	for (int i = 0; i < WRITE_BLOCK_NUM; i ++) {
		npa_t npa = { .x = dram_coord_x, .y = dram_coord_y , .epa = BASE_ADDR + epa_addr[i] };
		err = hb_mc_manycore_read_mem(mc, &npa, &A_device[i], sizeof(A_device[i]));
		if (err != HB_MC_SUCCESS) {
			bsg_pr_err("hb_mc_copy_from_epa: failed to read A[%d] from DRAM coord (%d,%d) addr %d.\n", i, dram_coord_x, dram_coord_y, BASE_ADDR + i);
			goto cleanup;
		}
	}

	bsg_pr_test_info("Checking vcache at (%d,%d):\n", dram_coord_x, dram_coord_y); 
	for (int i = 0; i < WRITE_BLOCK_NUM; i ++) { 
		if (A_host[i] == A_device[i]) {
			bsg_pr_err("Success -- A_host[%d] = %d   ==   A_device[%d] = %d -- EPA: %d.\n", i, A_host[i], i, A_device[i], BASE_ADDR + i); 
		}
		else { 
			bsg_pr_err("Failed -- A_host[%d] = %d   !=   A_device[%d] = %d -- EPA: %d.\n", i, A_host[i], i, A_device[i], BASE_ADDR + i);
			mismatch = 1;
		}
	}


	
	if (mismatch) 
		r = HB_MC_FAIL;
	else
		r = HB_MC_SUCCESS;

cleanup:
	hb_mc_manycore_exit(mc);
	return r;		
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
