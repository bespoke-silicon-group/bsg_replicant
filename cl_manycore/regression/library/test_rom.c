#include "test_rom.h"
int test_rom () {
        int rc = 0, fail = 0;
        uint32_t unexpected, expected, minexpected, maxexpected, result;
        hb_mc_dimension_t dim;
        hb_mc_coordinate_t host;
        const hb_mc_config_t *config;
        hb_mc_manycore_t mc = {0};

        rc = hb_mc_manycore_init(&mc, "manycore@test_rom", 0);
        if(rc != HB_MC_SUCCESS){
		bsg_pr_test_err("Failed to initialize manycore device: %s\n",
				hb_mc_strerror(rc));
                return HB_MC_FAIL;
        }

        config = hb_mc_manycore_get_config(&mc);
        /* Test host credit return value. Expect 16 and fail otherwise */
        expected = 16;
        bsg_pr_test_info("Checking that the number of host credits is %d\n", expected);
        bsg_pr_test_info("(I know it's a magic number...)\n");
        result = hb_mc_manycore_get_host_credits(&mc);
        if(result != expected){
                bsg_pr_test_err("Incorrect number of host credits. "
                                "Got: %d, expected %d\n", result, expected);
		bsg_pr_test_err("Have you programed your FPGA"
				" (fpga-load-local-image)\n");
		fail = 1;
		goto cleanup; // Considered harmful
        }

        /* Read configuration and test values */
#ifdef COSIM
        expected = 0xFF;
        bsg_pr_test_info("Checking that the COSIM Major Version Number is %d\n", expected);
        result = hb_mc_config_get_version_major(config);
        if(result != expected){
                bsg_pr_test_err("(COSIM) Incorrect Major Version Number. "
                                "Got: %d, expected %d\n", result, expected);
		fail = 1;
        }
#else
        minexpected = 0; maxexpected = 1;
        bsg_pr_test_info("Checking that the F1 Major Version Number is "
                        "between %d and %d\n", minexpected, maxexpected);
        result = hb_mc_config_get_version_major(config);
        if((result < minexpected) || (result > maxexpected)){
                bsg_pr_test_err("Unexpected value for Major Version Number. "
                                "Got: %d. Expected at least %d, at most %d\n",
                                result, minexpected, maxexpected);
		fail = 1;
        }
#endif
	bsg_pr_test_info("Read Major Version = %" PRIu32 ", "
			 "Minor Version = %" PRIu32 "\n",
			 hb_mc_config_get_version_major(config),
			 hb_mc_config_get_version_minor(config));

        unexpected = 0;
        bsg_pr_test_info("Checking that the BaseJump STL Hash is not "
                        "%d\n", unexpected);
        result = hb_mc_config_get_githash_basejump(config);
        if(result == unexpected){
                bsg_pr_test_err("Unexpected value for BaseJump STL Hash. "
                                "Got: 0x%x. (Should not be this value)\n",
                                unexpected);
		fail = 1;
        }

        unexpected = 0;
        bsg_pr_test_info("Checking that the BSG Manycore Hash is not "
                        "%d\n", unexpected);
        result = hb_mc_config_get_githash_manycore(config);
        if(result == unexpected){
                bsg_pr_test_err("Unexpected value for BSG Manycore Hash. "
                                "Got: 0x%x. (Should not be this value)\n",
                                unexpected);
		fail = 1;
        }

        unexpected = 0;
        bsg_pr_test_info("Checking that the BSG F1 Hash is not "
                        "%d\n", unexpected);
        result = hb_mc_config_get_githash_f1(config);
        if(result == unexpected){
                bsg_pr_test_err("Unexpected value for BSG F1 Hash. "
                                "Got: 0x%x. (Should not be this value)\n",
                                unexpected);
		fail = 1;
        }

        dim = hb_mc_config_get_dimension(config);
        minexpected = 1; maxexpected = 64;

        result = hb_mc_dimension_get_y(dim);
        bsg_pr_test_info("Checking that the Manycore Y Dimension is "
                        "between %d and %d\n", minexpected, maxexpected);
        if((result <= minexpected) || (result >= maxexpected)){
                bsg_pr_test_err("Unexpected value for Network Y Dimension. "
                                "Got: %d. Expected at least %d, at most %d\n",
                                result, minexpected, maxexpected);
		fail = 1;
        }


        result = hb_mc_dimension_get_x(dim);
        bsg_pr_test_info("Checking that the Manycore X Dimension is "
                        "between %d and %d\n", minexpected, maxexpected);
        if((result <= minexpected) || (result >= maxexpected)){
                bsg_pr_test_err("Unexpected value for Network X Dimension. "
                                "Got: %d. Expected at least %d, at most %d\n",
                                result, minexpected, maxexpected);
		fail = 1;
        }

        host = hb_mc_config_get_host_interface(config);

        minexpected = 0; maxexpected = hb_mc_dimension_get_y(dim) - 1;
        bsg_pr_test_info("Checking that the Host Interface Y Coordinate is "
                        "between %d and %d\n", minexpected, maxexpected);
        result = hb_mc_coordinate_get_y(host);
        if((result < minexpected) || (result > maxexpected)){
                bsg_pr_test_err("Unexpected value for Host Interface Y Coordinate. "
                                "Got: %d. Expected at least %d, at most %d\n",
                                result, minexpected, maxexpected);
		fail = 1;
        }

        minexpected = 0; maxexpected = hb_mc_dimension_get_x(dim) - 1;
        bsg_pr_test_info("Checking that the Host Interface X Coordinate is "
                        "between %d and %d\n", minexpected, maxexpected);
        result = hb_mc_coordinate_get_x(host);
        if((result < minexpected) || (result > maxexpected)){
                bsg_pr_test_err("Unexpected value for Host Interface X Coordinate. "
                                "Got: %d. Expected at least %d, at most %d\n",
                                result, minexpected, maxexpected);
		fail = 1;
        }

cleanup:
        hb_mc_manycore_exit(&mc);

        return fail ? HB_MC_FAIL : HB_MC_SUCCESS;
}

#ifdef COSIM
void test_main(uint32_t *exit_code) {
#ifdef VCS
	svScope scope;
	scope = svGetScopeFromName("tb");
	svSetScope(scope);
#endif
        bsg_pr_test_info("test_rom Regression Test (COSIMULATION)\n");
        int rc = test_rom();
        *exit_code = rc;
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return;
}
#else
int main() {
        bsg_pr_test_info("test_rom Regression Test (F1)\n");
        int rc = test_rom();
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
#endif

