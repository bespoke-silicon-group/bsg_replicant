#include <inttypes.h>
#include <bsg_manycore.h>
#include <bsg_manycore_config.h>
#include <bsg_manycore_coordinate.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_npa.h>
#include "test_manycore_vcache_sequence.h"

#define TEST_NAME "test_manycore_vcache_sequence"

#define ARRAY_LEN  4096
#define BASE_ADDR 0x0000

int test_manycore_vcache_sequence() {
	/********/
	/* INIT */
	/********/
	int err, r = HB_MC_FAIL;
	hb_mc_manycore_t manycore = {0}, *mc = &manycore;

	srand(time(0));
	
	err = hb_mc_manycore_init(mc, TEST_NAME, 0);
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to intialize manycore: %s\n",
			   __func__,
			   hb_mc_strerror(err));
		return HB_MC_FAIL;
	}

	const hb_mc_config_t *config = hb_mc_manycore_get_config(mc);
	uint32_t manycore_dim_x = hb_mc_coordinate_get_x(hb_mc_config_get_dimension_vcore(config));

	uint32_t dram_coord_x = 0;
	uint32_t dram_coord_y = hb_mc_config_get_dram_y(config);
	int mismatch = 0;

	/**************************************************************/
	/* Loop over all DRAM banks and write ARRAY_LEN words to each */
	/**************************************************************/
	for (dram_coord_x = 0; dram_coord_x < 1; dram_coord_x++) {
		uint32_t write_data, read_data;
		bsg_pr_test_info("%s: Testing DRAM bank (%" PRIu32 ",%" PRIu32 ")\n",
				 __func__, dram_coord_x, dram_coord_y);
		
		for (size_t i = 0; i < ARRAY_LEN; i++) {
			if ((i % 64) == 1)
				bsg_pr_test_info("%s: Have written and read back %4zu words\n",
						 __func__, i);
			
			hb_mc_npa_t npa = hb_mc_npa_from_x_y(dram_coord_x,
							     dram_coord_y,
							     BASE_ADDR+i);
			write_data = rand();
			err = hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data));
			if (err != HB_MC_SUCCESS) {
				bsg_pr_err("\n%s: failed to write A[%d] = 0x%08" PRIx32 ""
					   " to DRAM coord (%d,%d) @ 0x08%" PRIx32 ": %s\n",
					   __func__, i, write_data,
					   dram_coord_x, dram_coord_y,
					   (npa.epa+i) << 2,
					   hb_mc_strerror(err));
				goto cleanup;
			}

			err = hb_mc_manycore_read_mem(mc, &npa, &read_data, sizeof(read_data));
			if (err != HB_MC_SUCCESS) {
				bsg_pr_err("\n%s: failed read A[%d] "
					   "from DRAM coord (%d,%d) @ 0x%08" PRIx32 ": %s\n",
					   __func__, i,
					   dram_coord_x, dram_coord_y,
					   (npa.epa+i) << 2,
					   hb_mc_strerror(err));
				goto cleanup;
			}

			int data_match = read_data == write_data;
			if (!data_match) {
				bsg_pr_test_info("\n%s: mismatch @ index %d: "
						 "wrote 0x%08" PRIx32 " -- "
						 "read 0x%08" PRIx32 ": @ 0x%08" PRIx32 "\n",
						 __func__, i, i, write_data, read_data,
						 hb_mc_npa_get_epa(&npa));
			}
			
			mismatch = mismatch || !data_match;
		}
	}

	/********************************/
	/* Determine if the Test Failed */
	/********************************/
	r = mismatch ? HB_MC_FAIL : HB_MC_SUCCESS;
	
	/********/
	/* EXIT */
	/********/
cleanup:	
	hb_mc_manycore_exit(mc);
	return r;
}

#ifdef COSIM
void cosim_main(uint32_t *exit_code, char * args) {
	// We aren't passed command line arguments directly so we parse them
	// from *args. args is a string from VCS - to pass a string of arguments
	// to args, pass c_args to VCS as follows: +c_args="<space separated
	// list of args>"
	int argc = get_argc(args);
	char *argv[argc];
	get_argv(args, argc, argv);

#ifdef VCS
	svScope scope;
	scope = svGetScopeFromName("tb");
	svSetScope(scope);
#endif
	bsg_pr_test_info(TEST_NAME " Regression Test (COSIMULATION)\n");
	int rc = test_manycore_vcache_sequence();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main(int argc, char ** argv) {
	bsg_pr_test_info(TEST_NAME " Regression Test (F1)\n");
	int rc = test_manycore_vcache_sequence();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif
