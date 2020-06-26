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

// This test repeatedly reads the manycore cycle counter and makes
// sure that time increases.

#include "test_get_cycle.hpp"
#include <cstdint>

int test_get_cycle (int argc, char **argv) {
        int rc = 0, fail = 0;
        uint64_t cycle, last;
        uint32_t data = 0;
        hb_mc_manycore_t mc = {0};
        const hb_mc_config_t *config;
        struct arguments_none args = {};

        rc = argp_parse (&argp_none, argc, argv, 0, 0, &args);

        if(rc != HB_MC_SUCCESS){
                return rc;
        }

        rc = hb_mc_manycore_init(&mc, "manycore@test_get_cycle", 0);
        if(rc != HB_MC_SUCCESS){
                bsg_pr_test_err("Failed to initialize manycore device: %s\n",
                                hb_mc_strerror(rc));
                return HB_MC_FAIL;
        }

        config = hb_mc_manycore_get_config(&mc);

        // To cause time to proceed, we write to DRAM.
        uint32_t manycore_dim_x = hb_mc_coordinate_get_x(hb_mc_config_get_dimension_vcore(config));
        uint32_t manycore_dim_y = hb_mc_coordinate_get_y(hb_mc_config_get_dimension_vcore(config));

        uint32_t dram_coord_x = 0;
        uint32_t dram_coord_y = hb_mc_config_get_dram_y(config);
        hb_mc_npa_t npa = { .x = dram_coord_x, .y = dram_coord_y, .epa = 0};

        bsg_pr_test_info("Testing invalid input (This will print an ERROR message)\n");
        rc = hb_mc_manycore_get_cycle(&mc, nullptr);
        if(rc != HB_MC_INVALID){
                bsg_pr_test_err("hb_mc_manycore_get_cycle() failed to fail on invalid input");
                fail = HB_MC_FAIL;
                goto cleanup;
        }
                

        last = 0;
        for(int i = 0 ; i < 10; ++i){
                rc = hb_mc_manycore_get_cycle(&mc, &cycle);
                if(rc != HB_MC_SUCCESS){
                        bsg_pr_test_err("Failed to read cycle counter: %s\n",
                                        hb_mc_strerror(rc));
                        fail = rc;
                        goto cleanup;
                }

                if(last > cycle){
                        bsg_pr_test_err("Cycle counter not increasing!\n");
                        fail = HB_MC_FAIL;
                        goto cleanup;
                }

                last = cycle;

                bsg_pr_test_info("Current cycle is: %llu\n", cycle);
                rc = hb_mc_manycore_write_mem(&mc, &npa, &data, sizeof(data));
                if(rc != HB_MC_SUCCESS){
                        bsg_pr_test_err("Failed to write DRAM: %s\n",
                                        hb_mc_strerror(rc));
                        fail = rc;
                        goto cleanup;
                }

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

        bsg_pr_test_info("test_get_cycle Regression Test \n");
        int rc = test_get_cycle(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
