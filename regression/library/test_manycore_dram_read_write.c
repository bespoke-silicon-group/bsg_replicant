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
#include "test_manycore_dram_read_write.h"

#define TEST_NAME "test_manycore_dram_read_write"

#define ARRAY_LEN  4096
#define BASE_ADDR 0x0000

int test_manycore_dram_read_write() {
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
        uint32_t manycore_dim_y = hb_mc_coordinate_get_y(hb_mc_config_get_dimension_vcore(config));

        uint32_t dram_coord_x = 0;
        uint32_t dram_coord_y = hb_mc_config_get_dram_y(config);
        int mismatch = 0;

        /**************************************************************/
        /* Loop over all DRAM banks and write ARRAY_LEN words to each */
        /**************************************************************/
        for (dram_coord_x = 0; dram_coord_x < 1; dram_coord_x++) {
                uint32_t write_data[ARRAY_LEN];
                uint32_t read_data[ARRAY_LEN];
                int err;
                
                for (size_t i = 0; i < ARRAY_LEN; i++)
                        write_data[i] = rand() % ((1 << 16)-1);

                for (size_t i = 0; i < ARRAY_LEN; i++) {
                        if (i % 64 == 1)
                                bsg_pr_test_info("%s: Have written %zu words to DRAM\n",
                                    __func__, i);
                        
                        hb_mc_npa_t npa = { .x = dram_coord_x, .y = dram_coord_y, .epa = BASE_ADDR + (i*4) };
                        err = hb_mc_manycore_write_mem(mc, &npa,
                                                       &write_data[i], sizeof(write_data[i]));
                        if (err != HB_MC_SUCCESS) {
                                bsg_pr_err("%s: failed to write A[%d] = 0x%08" PRIx32 " "
                                           "to DRAM coord(%d,%d) @ 0x%08" PRIx32 "\n",
                                           __func__, i, write_data[i],
                                           dram_coord_x, dram_coord_y,
                                           BASE_ADDR + i);
                                goto cleanup;
                        }
                }

                for (size_t i = 0; i < ARRAY_LEN; i++) {
                        if (i % 64 == 1)
                                bsg_pr_test_info("%s: Have read %zu words from DRAM\n",
                                                 __func__, i);
                        hb_mc_npa_t npa = { .x = dram_coord_x, .y = dram_coord_y , .epa = BASE_ADDR + (i*4) };
                        err = hb_mc_manycore_read_mem(mc, &npa,
                                                      &read_data[i], sizeof(read_data[i]));
                        if (err != HB_MC_SUCCESS) {
                                bsg_pr_err("%s: failed to read A[%d] "
                                           "from DRAM coord(%d,%d) @ 0x%08" PRIx32 "\n",
                                           i, dram_coord_x, dram_coord_y, BASE_ADDR + i);
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

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif

        bsg_pr_test_info(TEST_NAME " Regression Test \n");
        int rc = test_manycore_dram_read_write();
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}

