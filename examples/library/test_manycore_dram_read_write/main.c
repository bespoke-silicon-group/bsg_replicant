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

#include <inttypes.h>
#include <bsg_manycore.h>
#include <bsg_manycore_config.h>
#include <bsg_manycore_coordinate.h>
#include <bsg_manycore_vcache.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <bsg_manycore_loader.h>
#include <bsg_manycore_errno.h>
#include <bsg_manycore_printing.h>

#include <bsg_manycore_regression.h>


#define TEST_NAME "test_manycore_dram_read_write"

#define ARRAY_LEN  4096
#define BASE_ADDR HB_MC_VCACHE_EPA_BASE

int test_manycore_dram_read_write(int argc, char *argv[]) {
        /********/
        /* INIT */
        /********/
        int err, r = HB_MC_FAIL;
        hb_mc_manycore_t manycore = {0}, *mc = &manycore;
        struct arguments_none args = {};
        err = argp_parse (&argp_none, argc, argv, 0, 0, &args);
        if(err != HB_MC_SUCCESS){
                return err;
        }

        srand(time(0));
        
        err = hb_mc_manycore_init(mc, TEST_NAME, args.device_id);
        if (err != HB_MC_SUCCESS) {
                bsg_pr_err("%s: failed to intialize manycore: %s\n",
                           __func__,
                           hb_mc_strerror(err));
                return HB_MC_FAIL;
        }

        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);

        int mismatch = 0;

        /**************************************************************/
        /* Loop over all DRAM banks and write ARRAY_LEN words to each */
        /**************************************************************/
        char str[256];
        bsg_pr_info("V-core dimesion = %s\n",
                    hb_mc_coordinate_to_string(hb_mc_config_get_dimension_vcore(cfg), str, sizeof(str)));

        bsg_pr_info("Network dimesion = %s\n",
                    hb_mc_coordinate_to_string(hb_mc_config_get_dimension_network(cfg), str, sizeof(str)));

        hb_mc_coordinate_t pod;
        hb_mc_config_foreach_pod(pod, cfg)
        {
                hb_mc_coordinate_t dram_coord;
                hb_mc_config_pod_foreach_dram(dram_coord, pod, cfg)
                {
                        uint32_t write_data[ARRAY_LEN];
                        uint32_t read_data[ARRAY_LEN];
                        int err;
                
                        for (size_t i = 0; i < ARRAY_LEN; i++)
                                write_data[i] = rand() % ((1 << 16)-1);

                        for (size_t i = 0; i < ARRAY_LEN; i++) {
                                if (i % 64 == 1)
                                        bsg_pr_test_info("%s: Have written %zu words to DRAM\n",
                                                         __func__, i);

                                hb_mc_npa_t npa = hb_mc_npa(dram_coord, BASE_ADDR + (i*4));
                                err = hb_mc_manycore_write_mem(mc, &npa,
                                                               &write_data[i], sizeof(write_data[i]));
                                if (err != HB_MC_SUCCESS) {
                                        bsg_pr_err("%s: failed to write A[%d] = 0x%08" PRIx32 " "
                                                   "to DRAM coord(%d,%d) @ 0x%08" PRIx32 "\n",
                                                   __func__, i, write_data[i],
                                                   hb_mc_coordinate_get_x(dram_coord),
                                                   hb_mc_coordinate_get_y(dram_coord),
                                                   BASE_ADDR + i);
                                        goto cleanup;
                                }
                        }

                        for (size_t i = 0; i < ARRAY_LEN; i++) {
                                if (i % 64 == 1)
                                        bsg_pr_test_info("%s: Have read %zu words from DRAM\n",
                                                         __func__, i);
                                hb_mc_npa_t npa = hb_mc_npa(dram_coord, BASE_ADDR + (i*4));
                                err = hb_mc_manycore_read_mem(mc, &npa,
                                                              &read_data[i], sizeof(read_data[i]));
                                if (err != HB_MC_SUCCESS) {
                                        bsg_pr_err("%s: failed to read A[%d] "
                                                   "from DRAM coord(%d,%d) @ 0x%08" PRIx32 "\n",
                                                   __func__, i,
                                                   hb_mc_coordinate_get_x(dram_coord),
                                                   hb_mc_coordinate_get_y(dram_coord),
                                                   BASE_ADDR + i);
                                        goto cleanup;
                                }
                        }

                        for (size_t i = 0; i < ARRAY_LEN; i++) {
                                int data_match = write_data[i] == read_data[i];
                                if (!data_match) {
                                        bsg_pr_err("\n%s: mismatch @ index %d: "
                                                   "wrote 0x%08" PRIx32 " -- "
                                                   "read 0x%08" PRIx32 ": @ 0x%08" PRIx32 "\n",
                                                   __func__, i, i, read_data, write_data);
                                }
                                mismatch = mismatch || !data_match;
                        }
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

declare_program_main(TEST_NAME, test_manycore_dram_read_write);
