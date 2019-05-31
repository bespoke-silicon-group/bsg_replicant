#define DEBUG 
#include <bsg_manycore.h>
#include <bsg_manycore_eva.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_errno.h>
#include <bsg_manycore_tile.h>
#include <inttypes.h>
#include "../cl_manycore_regression.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define TEST_NAME "test_manycore_credits"

int test_manycore_credits() {
	/********/
	/* INIT */
	/********/
	int rc, i;
	hb_mc_manycore_t manycore = {0}, *mc = &manycore;

	srand(time(0));
	
	rc = hb_mc_manycore_init(mc, TEST_NAME, 0);
	if (rc != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to intialize manycore: %s\n",
			   __func__,
			   hb_mc_strerror(rc));
		return HB_MC_FAIL;
	}

	const hb_mc_config_t *config = hb_mc_manycore_get_config(mc);
	hb_mc_idx_t dim_x = hb_mc_coordinate_get_x(hb_mc_config_get_dimension(config));
	hb_mc_idx_t dim_y = hb_mc_coordinate_get_y(hb_mc_config_get_dimension(config));
	hb_mc_idx_t x = 1;
	hb_mc_idx_t y = 1;
	hb_mc_coordinate_t target;

	hb_mc_eva_t eva;
	size_t dmem_size = hb_mc_tile_get_size_dmem(mc, &target); 
	size_t dmem_words = dmem_size/sizeof(uint32_t);
	uint32_t write_data[dmem_words];
	uint32_t read_data[dmem_words];

	/**************************************************************/
	/* Perform an EVA write that fills DMEM of a single tile.     */
	/**************************************************************/
	target = hb_mc_coordinate(x, y);
	eva = 0x1000;

	for (i = 0; i < dmem_words; ++i){
		write_data[i] = rand() % ((1 << 16)-1);
	}
	rc = hb_mc_manycore_eva_write(mc, &default_map, &target, &eva, write_data, dmem_size);
				
	if (rc != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to write buffer to EVA 0x%x of coord (%d,%d)\n",
			__func__, eva, x, y);
		goto cleanup;
	}
	bsg_pr_info("Successfully fill DMEM of tile (%d, %d)\n", x, y);
	rc = hb_mc_manycore_eva_read(mc, &default_map, &target, &eva, read_data, dmem_size);
				
	if (rc != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to read buffer to EVA 0x%x of coord (%d,%d)\n",
			__func__, eva, x, y);
		goto cleanup;
	}

	rc = HB_MC_SUCCESS;
	/********/
	/* EXIT */
	/********/
cleanup:
	hb_mc_manycore_exit(mc);
	return rc;
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {	
	bsg_pr_test_info(TEST_NAME " Regression Test (COSIMULATION)\n");
	int rc = test_manycore_credits();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info(TEST_NAME " Regression Test (F1)\n");
	int rc = test_manycore_credits();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif
