#define DEBUG
#include <bsg_manycore.h>
#include <bsg_manycore_eva.h>
#include <bsg_manycore_printing.h>
#include <inttypes.h>
#include "../cl_manycore_regression.h"
#define TEST_NAME "test_manycore_eva_read_write"
#define DATA_WORDS 32
int test_manycore_eva_read_write () {
	hb_mc_manycore_t manycore = {0}, *mc = &manycore;
	int err, r = HB_MC_FAIL, i, errors;
	uint32_t write_data[DATA_WORDS], read_data[DATA_WORDS] = {0};
	hb_mc_eva_t eva_dest, eva_src;
	hb_mc_coordinate_t target = { .x = 1, .y = 1 };
	uint8_t memset_char;
	uint32_t memset_word;
	/********/
	/* INIT */
	/********/
	err = hb_mc_manycore_init(mc, TEST_NAME, 0);
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to initialize manycore: %s\n",
			   __func__, hb_mc_strerror(err));
		return HB_MC_FAIL;
	}

	for (i = 0; i < DATA_WORDS; ++i) {
		write_data[i] = i + 1;
	}

	eva_dest = 1024;
	memset_char = 0xAA;
	memset_word = (memset_char << 24) | (memset_char << 16) | (memset_char << 8) | memset_char;
	/***************************/
	/* Running Memset (0xAA)   */
	/***************************/
	bsg_pr_test_info("Memset DMEM at EVA 0x%08x\n", eva_dest);
	err = hb_mc_manycore_eva_memset(mc, &default_map, &target, &eva_dest, 
					memset_char, sizeof(write_data));
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to memset manycore DMEM: %s\n",
			   __func__,
			   hb_mc_strerror(err));
		goto cleanup;
	}
	bsg_pr_test_info("Memset successful\n");

	/******************************/
	/* Read back Data from Memory */
	/******************************/

	err = hb_mc_manycore_eva_read(mc, &default_map, &target, &eva_dest,
				read_data, sizeof(read_data));
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to read from manycore DMEM: %s\n",
			   __func__,
			   hb_mc_strerror(err));
		goto cleanup;
	}

	bsg_pr_test_info("Completed read\n");
	errors = 0;
	for (i = 0; i < DATA_WORDS; ++i) {
		if (read_data[i] == memset_word) {
			bsg_pr_test_info("Read back data written: 0x%08" PRIx32 "\n",
					read_data[i]);
		} else {
			bsg_pr_test_info("Data mismatch: read 0x%08" PRIx32 ", wrote 0x%08" PRIx32 "\n",
					read_data[i], memset_word);
			errors +=1;
		}
	}

	/***************************/
	/* Writing to DMEM via EVA */
	/***************************/
	
	bsg_pr_test_info("Writing to DMEM at EVA 0x%08x\n", eva_dest);
	err = hb_mc_manycore_eva_write(mc, &default_map, &target, &eva_dest, write_data, sizeof(write_data));
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

	err = hb_mc_manycore_eva_read(mc, &default_map, &target, &eva_dest, read_data, sizeof(read_data));
	if (err != HB_MC_SUCCESS) {
		bsg_pr_err("%s: failed to read from manycore DMEM: %s\n",
			   __func__,
			   hb_mc_strerror(err));
		goto cleanup;
	}

	bsg_pr_test_info("Completed read\n");
	errors = 0;
	for (i = 0; i < DATA_WORDS; ++i) {
		if (read_data[i] == write_data[i]) {
			bsg_pr_test_info("Read back data written: 0x%08" PRIx32 "\n",
					read_data[i]);
		} else {
			bsg_pr_test_info("Data mismatch: read 0x%08" PRIx32 ", wrote 0x%08" PRIx32 "\n",
					read_data[i], write_data[i]);
			errors +=1;
		}
	}
	if(!errors)
		r = HB_MC_SUCCESS;


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
	int rc = test_manycore_eva_read_write();
	*exit_code = rc;
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return;
}
#else
int main() {
	bsg_pr_test_info(TEST_NAME " Regression Test (F1)\n");
	int rc = test_manycore_eva_read_write();
	bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
	return rc;
}
#endif
