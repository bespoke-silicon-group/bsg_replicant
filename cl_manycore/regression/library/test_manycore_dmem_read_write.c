#include <bsg_manycore.h>
#include <bsg_manycore_npa.h>
#include <bsg_manycore_printing.h>
#include <inttypes.h>
#include "test_manycore_dmem_read_write.h"

#define TEST_NAME "test_manycore_dmem_read_write"

int test_manycore_dmem_read_write () {
	hb_mc_manycore_t manycore = {0}, *mc = &manycore;
	int err, r = HB_MC_FAIL;


	/********/
	/* INIT */
	/********/
	err = hb_mc_manycore_init(mc, TEST_NAME, 0);
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to initialize manycore: %s\n",
			   __func__, hb_mc_strerror(err));
		return HB_MC_FAIL;
	}


	/**************************/
	/* Writing to Data Memory */
	/**************************/
	uint32_t write_data = 0xABCD;
	hb_mc_npa_t npa = { .x = 0, .y = 1, .epa = DMEM_BASE };

	bsg_pr_test_info("Writing to DMEM\n");
	err = hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data));
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to write to manycore DMEM: %s\n",
			   __func__,
			   hb_mc_strerror(err));
		goto cleanup;
	}
	bsg_pr_test_info("Write successful\n");

	/******************************/
	/* Read back Data from Memory */
	/******************************/
	uint32_t read_data;
	err = hb_mc_manycore_read_mem(mc, &npa, &read_data, sizeof(read_data));
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to read from manycore DMEM: %s\n",
			   __func__,
			   hb_mc_strerror(err));
		goto cleanup;
	}

	bsg_pr_test_info("Completed read\n");
	if (read_data == write_data) {
		bsg_pr_test_info("Read back data written: 0x%08" PRIx32 "\n",
				 read_data);
	} else {
		bsg_pr_test_info("Data mismatch: read 0x%08" PRIx32 ", wrote 0x%08" PRIx32 "\n",
				 read_data, write_data);
	}
	r = (read_data == write_data ? HB_MC_SUCCESS : HB_MC_FAIL);

	/*******/
	/* END */
	/*******/
cleanup:
	hb_mc_manycore_exit(mc);
	return r;
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {
	bsg_pr_test_info(TEST_NAME " Regression Test (COSIMULATION)\n");
	int rc = test_manycore_dmem_read_write();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info(TEST_NAME " Regression Test (F1)\n");
	int rc = test_manycore_dmem_read_write();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif
