// Copyright (c) 2019, University of Washington All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list
// of conditions and the following disclaimer.
// 
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
// 
// Neither the name of the copyright holder nor the names of its contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "test_rom.h"
#include <inttypes.h>

int test_rom (int argc, char **argv) {
        int rc = 0, fail = 0;
        uint32_t unexpected, expected, minexpected, maxexpected, result;
        uint32_t vcache_assoc, vcache_sets, vcache_block_words;
        uint32_t vcache_assoc_expect_max = 32;
        uint32_t vcache_sets_expect = BSG_MANYCORE_MACHINE_VCACHE_BANK_SETS;
        uint32_t major_version_max = 4; uint32_t minor_version_max = 10, minor_version_min = 0;
        uint32_t dram_size = 0, dram_bank_size = 0, n_dram_co = 0;
        hb_mc_dimension_t dim;
        hb_mc_coordinate_t host;
        const hb_mc_config_t *config;
        hb_mc_manycore_t mc = {0};
        hb_mc_memsys_id_t memsysid;
        char *path;
        struct arguments_none args = {NULL};

        rc = argp_parse (&argp_none, argc, argv, 0, 0, &args);

        if(rc != HB_MC_SUCCESS){
                return rc;
        }

        rc = hb_mc_manycore_init(&mc, "manycore@test_rom", 0);
        if(rc != HB_MC_SUCCESS){
                bsg_pr_test_err("Failed to initialize manycore device: %s\n",
                                hb_mc_strerror(rc));
                return HB_MC_FAIL;
        }

        config = hb_mc_manycore_get_config(&mc);


        /* Read configuration and test values */
        minexpected = 0; maxexpected = 4;
        bsg_pr_test_info("Checking that the F1 Major Version Number is "
                        "between %d and %d\n", minexpected, maxexpected);
        result = hb_mc_config_get_version_major(config);
        if((result < minor_version_min) || (result > minor_version_max)){
                bsg_pr_test_err("Unexpected value for Major Version Number. "
                                "Got: %d. Expected >= %d, <= %d\n",
                                result, minor_version_min, minor_version_max);
                fail = 1;
        }

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

        dim = hb_mc_config_get_dimension_vcore(config);
        minexpected = 1; maxexpected = 64;

        result = hb_mc_dimension_get_y(dim);
        bsg_pr_test_info("Checking that the Vanilla Core Y Dimension is "
                        "between %d and %d\n", minexpected, maxexpected);
        if((result <= minexpected) || (result >= maxexpected)){
                bsg_pr_test_err("Unexpected value for Network Y Dimension. "
                                "Got: %d. Expected at least %d, at most %d\n",
                                result, minexpected, maxexpected);
                fail = 1;
        }


        result = hb_mc_dimension_get_x(dim);
        bsg_pr_test_info("Checking that the Vanilla Core X Dimension is "
                        "between %d and %d\n", minexpected, maxexpected);
        if((result <= minexpected) || (result >= maxexpected)){
                bsg_pr_test_err("Unexpected value for Network X Dimension. "
                                "Got: %d. Expected at least %d, at most %d\n",
                                result, minexpected, maxexpected);
                fail = 1;
        }

        dim = hb_mc_config_get_dimension_vcore(config);
        expected = hb_mc_dimension_get_y(dim) + 3;

        dim = hb_mc_config_get_dimension_network(config);

        result = hb_mc_dimension_get_y(dim);
        bsg_pr_test_info("Checking that the Network Y Dimension is %d\n",
                        expected);
        if(result != expected){
                bsg_pr_test_err("Incorrect Network dimension. "
                                "Got: %d, expected %d\n", result, expected);
                fail = 1;
        }

        dim = hb_mc_config_get_dimension_vcore(config);
        expected = hb_mc_dimension_get_x(dim);

        dim = hb_mc_config_get_dimension_network(config);

        result = hb_mc_dimension_get_x(dim);
        bsg_pr_test_info("Checking that the Network X Dimension is %d\n",
                        expected);
        if(result != expected){
                bsg_pr_test_err("Incorrect Network dimension. "
                                "Got: %d, expected %d\n", result, expected);
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

        vcache_assoc       = hb_mc_config_get_vcache_ways(config);
        bsg_pr_test_info("Checking that V-Cache associativity is less than %" PRIu32 "\n", vcache_assoc_expect_max);
        if (vcache_assoc > vcache_assoc_expect_max) {
                bsg_pr_test_err("Unexpected associativity value: Got %" PRIu32 ". Expected <= %" PRIu32 "\n",
                                vcache_assoc, vcache_assoc_expect_max);
                fail = 1;
        }

        vcache_sets        = hb_mc_config_get_vcache_sets(config);
        bsg_pr_test_info("Checking that V-Cache number of sets is %" PRIu32 "\n", vcache_sets_expect);
        if (vcache_sets != vcache_sets_expect) {
                bsg_pr_test_err("Unexpected V-Cache set number: Got %" PRIu32 ". Expected %" PRIu32 "\n",
                                vcache_sets, vcache_sets_expect);
                fail = 1;
        }

        vcache_block_words = hb_mc_config_get_vcache_block_words(config);
        bsg_pr_test_info("Checking that V-Cache block size in words is >= 0 \n");
        if (vcache_block_words <= 0) {
                bsg_pr_test_err("Unexpected V-Cache block size: "
                                "Got %" PRIu32 " words, "
                                "Expected >=0 words\n",
                                vcache_block_words);
                fail = 1;
        }

        dram_size = hb_mc_config_get_dram_size(config);
        n_dram_co = hb_mc_config_get_num_dram_coordinates(config);
        dram_bank_size = hb_mc_config_get_dram_bank_size(config);
        memsysid = hb_mc_config_memsys_id(config);

        bsg_pr_test_info("Checking that DRAM bank size = DRAM size / DRAM coordinates for %s\n",
                         hb_mc_memsys_id_to_string(memsysid));

        if (!(dram_bank_size == (dram_size/n_dram_co))) {
                bsg_pr_test_err("Failed: DRAM size = %" PRIu32 " bytes, "
                                "DRAM bank size = %" PRIu32 " bytes, "
                                "DRAM coordinates = %" PRIu32 "\n",
                                dram_size, dram_bank_size, n_dram_co);
                fail = 1;
        }

cleanup:
        rc = hb_mc_manycore_exit(&mc);

        return fail ? HB_MC_FAIL : HB_MC_SUCCESS;
}

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif

        bsg_pr_test_info("test_rom Regression Test \n");
        int rc = test_rom(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
