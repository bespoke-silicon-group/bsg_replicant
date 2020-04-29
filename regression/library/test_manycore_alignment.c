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
#include <bsg_manycore_printing.h>
#include <bsg_manycore_npa.h>
#include "test_manycore_alignment.h"

#define TEST_NAME "test_manycore_alignment"

#define BASE_ADDR 0x0000

int test_manycore_alignment() {
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

        hb_mc_dimension_t dim = hb_mc_config_get_dimension_network(config);

        uint32_t dram_coord_x = 0;

        uint32_t dram_coord_y = hb_mc_config_get_dram_y(config);
        int mismatch = 0;

        /*
         * Loop over all DRAM banks and try to read/write to an unaligned address
         */
        for (dram_coord_x = 0; dram_coord_x < hb_mc_dimension_get_x(dim); dram_coord_x++) {
                uint32_t write_data, read_data;
                bsg_pr_test_info("%s: Testing DRAM bank (%" PRIu32 ",%" PRIu32 ")\n",
                                 __func__, dram_coord_x, dram_coord_y);

                // Write a 4-byte word into mis-aligned addresses to
                // verify that API returns an invalid address error
                hb_mc_npa_t npa = hb_mc_npa_from_x_y(dram_coord_x,
                                                     dram_coord_y,
                                                     BASE_ADDR + 1);
                write_data = rand();
                err = hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data));
                if (err == HB_MC_UNALIGNED) { 
                        bsg_pr_test_info("%s: Expected an invalid address error. We're good!\n", __func__);
                } else {
                        bsg_pr_err("%s: Did not catch write into invalid misaligned address: 0x%08" PRIx32 ".\n",
                                   __func__, hb_mc_npa_get_epa(&npa));
                        return HB_MC_FAIL;
                }

                npa = hb_mc_npa_from_x_y(dram_coord_x,
                                         dram_coord_y,
                                         BASE_ADDR + 2);
                write_data = rand();
                err = hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data));
                if (err == HB_MC_UNALIGNED) {
                        bsg_pr_test_info("%s: Expected an invalid address error. We're good!\n", __func__);
                } else {
                        bsg_pr_err("%s: Did not catch write into invalid misaligned address: 0x%08" PRIx32 ".\n",
                                   __func__, hb_mc_npa_get_epa(&npa));
                        return HB_MC_FAIL;
                }

                npa = hb_mc_npa_from_x_y(dram_coord_x,
                                         dram_coord_y,
                                         BASE_ADDR + 3);
                write_data = rand();
                err = hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data));
                if (err == HB_MC_UNALIGNED) {
                        bsg_pr_test_info("%s: Expected an invalid address error. We're good!\n", __func__);
                } else {
                        bsg_pr_err("%s: Did not catch write into invalid misaligned address: 0x%08" PRIx32 ".\n",
                                   __func__, hb_mc_npa_get_epa(&npa));
                        return HB_MC_FAIL;
                }


                // Read a 4-byte word from mis-aligned addresses to 
                // verify that API returns an invalid address error
                npa = hb_mc_npa_from_x_y(dram_coord_x,
                                         dram_coord_y,
                                         BASE_ADDR + 1);
                err = hb_mc_manycore_read_mem(mc, &npa, &read_data, sizeof(read_data));
                if (err == HB_MC_UNALIGNED) { 
                        bsg_pr_test_info("%s: Expected an invalid address error. We're good!\n", __func__);
                } else {
                        bsg_pr_err("%s: Did not catch write into invalid misaligned address: 0x%08" PRIx32 ".\n",
                                   __func__, hb_mc_npa_get_epa(&npa));
                        return HB_MC_FAIL;
                }

                npa = hb_mc_npa_from_x_y(dram_coord_x,
                                         dram_coord_y,
                                         BASE_ADDR + 2);
                err = hb_mc_manycore_read_mem(mc, &npa, &read_data, sizeof(read_data));
                if (err == HB_MC_UNALIGNED) {
                        bsg_pr_test_info("%s: Expected an invalid address error. We're good!\n", __func__);
                } else {
                        bsg_pr_err("%s: Did not catch write into invalid misaligned address: 0x%08" PRIx32 ".\n",
                                   __func__, hb_mc_npa_get_epa(&npa));
                        return HB_MC_FAIL;
                }

                npa = hb_mc_npa_from_x_y(dram_coord_x,
                                         dram_coord_y,
                                         BASE_ADDR + 3);
                err = hb_mc_manycore_read_mem(mc, &npa, &read_data, sizeof(read_data));
                if (err == HB_MC_UNALIGNED) {
                        bsg_pr_test_info("%s: Expected an invalid address error. We're good!\n", __func__);
                } else {
                        bsg_pr_err("%s: Did not catch write into invalid misaligned address: 0x%08" PRIx32 ".\n",
                                   __func__, hb_mc_npa_get_epa(&npa));
                        return HB_MC_FAIL;
                }

                // Write a 4-byte word into mis-aligned addresses to
                // verify that API returns an invalid address error
                npa = hb_mc_npa_from_x_y(dram_coord_x,
                                         dram_coord_y,
                                         BASE_ADDR + 1);
                write_data = rand();
                err = hb_mc_manycore_write32(mc, &npa, &write_data);
                if (err == HB_MC_UNALIGNED) { 
                        bsg_pr_test_info("%s: Expected an invalid address error. We're good!\n", __func__);
                } else {
                        bsg_pr_err("%s: Did not catch write into invalid misaligned address: 0x%08" PRIx32 ".\n",
                                   __func__, hb_mc_npa_get_epa(&npa));
                        return HB_MC_FAIL;
                }

                npa = hb_mc_npa_from_x_y(dram_coord_x,
                                         dram_coord_y,
                                         BASE_ADDR + 2);
                write_data = rand();
                err = hb_mc_manycore_write32(mc, &npa, &write_data);
                if (err == HB_MC_UNALIGNED) {
                        bsg_pr_test_info("%s: Expected an invalid address error. We're good!\n", __func__);
                } else {
                        bsg_pr_err("%s: Did not catch write into invalid misaligned address: 0x%08" PRIx32 ".\n",
                                   __func__, hb_mc_npa_get_epa(&npa));
                        return HB_MC_FAIL;
                }

                npa = hb_mc_npa_from_x_y(dram_coord_x,
                                         dram_coord_y,
                                         BASE_ADDR + 3);
                write_data = rand();
                err = hb_mc_manycore_write32(mc, &npa, &write_data);
                if (err == HB_MC_UNALIGNED) {
                        bsg_pr_test_info("%s: Expected an invalid address error. We're good!\n", __func__);
                } else {
                        bsg_pr_err("%s: Did not catch write into invalid misaligned address: 0x%08" PRIx32 ".\n",
                                   __func__, hb_mc_npa_get_epa(&npa));
                        return HB_MC_FAIL;
                }


                // Read a 4-byte word from mis-aligned addresses to 
                // verify that API returns an invalid address error
                npa = hb_mc_npa_from_x_y(dram_coord_x,
                                         dram_coord_y,
                                         BASE_ADDR + 1);
                err = hb_mc_manycore_read32(mc, &npa, &read_data);
                if (err == HB_MC_UNALIGNED) { 
                        bsg_pr_test_info("%s: Expected an invalid address error. We're good!\n", __func__);
                } else {
                        bsg_pr_err("%s: Did not catch write into invalid misaligned address: 0x%08" PRIx32 ".\n",
                                   __func__, hb_mc_npa_get_epa(&npa));
                        return HB_MC_FAIL;
                }

                npa = hb_mc_npa_from_x_y(dram_coord_x,
                                         dram_coord_y,
                                         BASE_ADDR + 2);
                err = hb_mc_manycore_read32(mc, &npa, &read_data);
                if (err == HB_MC_UNALIGNED) {
                        bsg_pr_test_info("%s: Expected an invalid address error. We're good!\n", __func__);
                } else {
                        bsg_pr_err("%s: Did not catch write into invalid misaligned address: 0x%08" PRIx32 ".\n",
                                   __func__, hb_mc_npa_get_epa(&npa));
                        return HB_MC_FAIL;
                }

                npa = hb_mc_npa_from_x_y(dram_coord_x,
                                         dram_coord_y,
                                         BASE_ADDR + 3);
                err = hb_mc_manycore_read32(mc, &npa, &read_data);
                if (err == HB_MC_UNALIGNED) {
                        bsg_pr_test_info("%s: Expected an invalid address error. We're good!\n", __func__);
                } else {
                        bsg_pr_err("%s: Did not catch write into invalid misaligned address: 0x%08" PRIx32 ".\n",
                                   __func__, hb_mc_npa_get_epa(&npa));
                        return HB_MC_FAIL;
                }

                // Write a 2-byte value into mis-aligned addresses to
                // verify that API returns an invalid address error
                npa = hb_mc_npa_from_x_y(dram_coord_x,
                                         dram_coord_y,
                                         BASE_ADDR + 1);
                write_data = rand();
                err = hb_mc_manycore_write16(mc, &npa, &write_data);
                if (err == HB_MC_UNALIGNED) { 
                        bsg_pr_test_info("%s: Expected an invalid address error. We're good!\n", __func__);
                } else {
                        bsg_pr_err("%s: Did not catch write into invalid misaligned address: 0x%08" PRIx32 ".\n",
                                   __func__, hb_mc_npa_get_epa(&npa));
                        return HB_MC_FAIL;
                }

                npa = hb_mc_npa_from_x_y(dram_coord_x,
                                         dram_coord_y,
                                         BASE_ADDR + 3);
                write_data = rand();
                err = hb_mc_manycore_write16(mc, &npa, &write_data);
                if (err == HB_MC_UNALIGNED) {
                        bsg_pr_test_info("%s: Expected an invalid address error. We're good!\n", __func__);
                } else {
                        bsg_pr_err("%s: Did not catch write into invalid misaligned address: 0x%08" PRIx32 ".\n",
                                   __func__, hb_mc_npa_get_epa(&npa));
                        return HB_MC_FAIL;
                }


                // Read a 2-byte value from mis-aligned addresses to 
                // verify that API returns an invalid address error
                npa = hb_mc_npa_from_x_y(dram_coord_x,
                                         dram_coord_y,
                                         BASE_ADDR + 1);
                err = hb_mc_manycore_read16(mc, &npa, &read_data);
                if (err == HB_MC_UNALIGNED) { 
                        bsg_pr_test_info("%s: Expected an invalid address error. We're good!\n", __func__);
                } else {
                        bsg_pr_err("%s: Did not catch write into invalid misaligned address: 0x%08" PRIx32 ".\n",
                                   __func__, hb_mc_npa_get_epa(&npa));
                        return HB_MC_FAIL;
                }

                npa = hb_mc_npa_from_x_y(dram_coord_x,
                                         dram_coord_y,
                                         BASE_ADDR + 3);
                err = hb_mc_manycore_read16(mc, &npa, &read_data);
                if (err == HB_MC_UNALIGNED) {
                        bsg_pr_test_info("%s: Expected an invalid address error. We're good!\n", __func__);
                } else {
                        bsg_pr_err("%s: Did not catch write into invalid misaligned address: 0x%08" PRIx32 ".\n",
                                   __func__, hb_mc_npa_get_epa(&npa));
                        return HB_MC_FAIL;
                }

        }

        r = HB_MC_SUCCESS;
        
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
        int rc = test_manycore_alignment();
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}

