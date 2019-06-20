#include "test_vcache_sequence.h"

#define ARRAY_LEN 4096
#define BASE_ADDR 0x0000

int test_vcache_sequence() {
	srand(time(0));
	hb_mc_manycore_t manycore = {0}, *mc = &manycore;
	int err, r = HB_MC_FAIL;

	err = hb_mc_manycore_init(mc, "test_vcache_sequence", 0);
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to init manycore: %s\n",
			   __func__, hb_mc_strerror(err));
		return err;
	}
	

	const hb_mc_config_t *config = hb_mc_manycore_get_config(mc);
	uint32_t manycore_dim_x = hb_mc_coordinate_get_x(hb_mc_config_get_dimension_vcore(config));
	uint32_t manycore_dim_y = hb_mc_coordinate_get_y(hb_mc_config_get_dimension_vcore(config));


 	uint32_t dram_coord_x = 0;
	uint32_t dram_coord_y = hb_mc_config_get_dram_y(config);
	int mismatch = 0; 


	/* To check all dram banks change 1 to manycore_dim_x */
	for (dram_coord_x = 0; dram_coord_x < 1; dram_coord_x ++) {

		bsg_pr_test_info("Testing DRAM bank (%d,%d).\n", dram_coord_x, dram_coord_y);
		uint32_t A_host;
		uint32_t A_device;

		hb_mc_response_packet_t buf[1]; 
		int mismatch = 0;

		hb_mc_coordinate_t dram_coord = hb_mc_coordinate (dram_coord_x, dram_coord_y); 

		for (int i = 0; i < ARRAY_LEN; i ++) { 
			A_host = rand();

			hb_mc_npa_t dram_npa = hb_mc_npa (dram_coord, BASE_ADDR + i); 

			err = hb_mc_manycore_write32(mc, &dram_npa, A_host); 
			if (err != HB_MC_SUCCESS) {
				bsg_pr_err("%s: failed to write A[%d] = %d to DRAM coord(%d,%d) addr %d.\n", __func__, i, A_host, dram_coord_x, dram_coord_y, BASE_ADDR + i);
				return err;
			}


			err = hb_mc_manycore_read32(mc, &dram_npa, &A_device); 
			if ( err != HB_MC_SUCCESS) {
				bsg_pr_err("%s: failed to read A[%d] from DRAM coord (%d,%d) addr %d.\n", __func__, i, dram_coord_x, dram_coord_y, BASE_ADDR + i);
				return err;
			}

			if (A_host != A_device) {
				bsg_pr_err(BSG_RED("Mismatch: ") "A_host[%d] = %d   !=   A_device[%d] = %d -- EPA: %d.\n", i, A_host, i, A_device, BASE_ADDR + i);
				mismatch = 1;
			}
		} 
	}


	if (mismatch) 
		return HB_MC_FAIL;
	return HB_MC_SUCCESS;		
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {	
#ifdef VCS
	svScope scope;
	scope = svGetScopeFromName("tb");
	svSetScope(scope);
#endif
	bsg_pr_test_info("test_vcache_sequence Regression Test (COSIMULATION)\n");
	int rc = test_vcache_sequence();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_vcache_sequence Regression Test (F1)\n");
	int rc = test_vcache_sequence();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif
