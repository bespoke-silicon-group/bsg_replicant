/**
 * test_vcache_stride
 *
 *
 */
#include "test_vcache_stride.h"

#define STRIDE_SIZE 1024
#define DRAM_BASE 0x0000
#define NUM_STRIDES 16

int test_vcache_stride() {
	int rc;
	hb_mc_manycore_t mc = HB_MC_MANYCORE_INIT;
	const hb_mc_config_t *config;
	hb_mc_dimension_t dim;
	hb_mc_coordinate_t host, dest;
	hb_mc_idx_t host_x, host_y, dim_x, dim_y;
	srand(time(0));

	rc = hb_mc_manycore_init(&mc, "manycore@test_vcache_stride", 0);
	if(rc != HB_MC_SUCCESS){
		bsg_pr_test_err("Failed to initialize manycore device!\n");
		return HB_MC_FAIL;
	}

	config = hb_mc_manycore_get_config(&mc);

	host = hb_mc_config_get_host_interface(config);
	host_x = hb_mc_coordinate_get_x(host);
	host_y = hb_mc_coordinate_get_y(host);

	dim = hb_mc_config_get_dimension_network(config);
	dim_x = hb_mc_dimension_get_x(dim);
	dim_y = hb_mc_dimension_get_y(dim);

	/* To increase the number of DRAM banks tested, increase ndrams (must be
	 * less than dim_x) and add the X coordinates to dim_x */
	int dram = 0, ndrams = 1;
	hb_mc_idx_t dram_xs[dim_x];
	dram_xs[0] = 0;
	hb_mc_idx_t dram_coord_y = hb_mc_config_get_dram_y(config);
	hb_mc_idx_t dram_coord_x = -1;
	hb_mc_epa_t epa;
	hb_mc_npa_t npa;
	int xidx, stride;
	uint32_t gold  [NUM_STRIDES] = {0};
	uint32_t result[NUM_STRIDES] = {0};
	uint32_t val;

	hb_mc_request_packet_t req;
	hb_mc_response_packet_t res;

	for (dram = 0; dram < ndrams; ++dram){
		dram_coord_x = dram_xs[dram];
		bsg_pr_test_info("Testing DRAM/Cache Interface at (%d, %d).\n", dram_coord_x, dram_coord_y);

		for (stride = 0; stride < NUM_STRIDES; ++stride) {
			gold[stride] = rand();
		}

		for (stride = 0; stride < NUM_STRIDES; ++stride) {
			epa = DRAM_BASE + stride * STRIDE_SIZE;
			dest = hb_mc_coordinate(dram_coord_x, dram_coord_y);
			npa = hb_mc_epa_to_npa(dest, epa);
			val = gold[stride];

			bsg_pr_test_info("%s -- Writing value %lu to 0x%x @ (%d, %d)\n", 
					__func__, val, epa, dram_coord_x, dram_coord_y);
			rc = hb_mc_manycore_write32(&mc, &npa, val);
			if(rc != HB_MC_SUCCESS) {
				bsg_pr_test_err("%s -- hb_mc_write32 failed on interation %d!\n", __func__, stride);
				return HB_MC_FAIL;
			}
			val = ~val;

			rc = hb_mc_manycore_read32(&mc, &npa, &val);
			if(rc != HB_MC_SUCCESS) {
				bsg_pr_test_err("%s -- hb_mc_read32 failed on iteration %d!\n", __func__, stride);
				return HB_MC_FAIL;
			}
			bsg_pr_test_info("%s -- Read value %lu from 0x%x @ (%d, %d)\n", 
					__func__, val, epa, dram_coord_x, dram_coord_y);
			result[stride] = val;
		}
	}
	for (stride = 0; stride < NUM_STRIDES; ++stride) {
		if(result[stride] != gold[stride]){
			bsg_pr_test_err("%s -- Index %d: Result, %lu, did not match expected, %lu!\n", 
					__func__, stride, result[stride], gold[stride]);
			return HB_MC_FAIL;
		}
	}
	bsg_pr_test_info("%s -- %d Strides Passed\n", __func__, stride);

	return HB_MC_SUCCESS;
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {
#ifdef VCS
	svScope scope;
	scope = svGetScopeFromName("tb");
	svSetScope(scope);
#endif
	bsg_pr_test_info("test_vcache_stride Regression Test (COSIMULATION)\n");
	int rc = test_vcache_stride();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info("test_vcache_stride Regression Test (F1)\n");
	int rc = test_vcache_stride();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif
