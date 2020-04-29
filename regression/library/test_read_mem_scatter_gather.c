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

#include "test_read_mem_scatter_gather.h"
#include <bsg_manycore.h>
#include <bsg_manycore_npa.h>
#include <bsg_manycore_printing.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define TEST_NAME "test_read_mem_scatter_gather"

#define test_pr_err(msg, ...)                           \
        bsg_pr_err(TEST_NAME ": " msg , ##__VA_ARGS__)

hb_mc_manycore_t manycore, *mc = &manycore;


#define WORDS 3

hb_mc_npa_t target_npas [WORDS];
uint32_t    out         [WORDS];
uint32_t    in          [WORDS];
/*
 * Initialize target_npas to an access pattern that strides columns.
 */
static void initialize_target_npas(void)
{
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        hb_mc_idx_t y = hb_mc_config_get_dram_y(cfg);
        hb_mc_idx_t columns = hb_mc_dimension_get_x(hb_mc_config_get_dimension_vcore(cfg));
        hb_mc_idx_t base_x = hb_mc_config_get_vcore_base_x(cfg);

        target_npas[0] = hb_mc_npa_from_x_y(base_x,     y, 0);
        target_npas[1] = hb_mc_npa_from_x_y(base_x+1,   y, 4);
        target_npas[2] = hb_mc_npa_from_x_y(base_x,     y, 8);
}

/*
 * Initialize out word vector
 */
static void initialize_out_data(void)
{
        int i;
        for (i = 0; i < WORDS; i++)
                out[i] = (uint32_t)rand();
}

/*
 * Write out word vector to manycore NPAs
 */
static int write_out_data(void)
{
        int i, err;
        for (i = 0; i < WORDS; i++) {
                err = hb_mc_manycore_write32(mc, &target_npas[i], out[i]);
                if (err != HB_MC_SUCCESS) {
                        char npa_str[256];
                        hb_mc_npa_to_string(&target_npas[i], npa_str, sizeof(npa_str));
                        test_pr_err("failed to write word %d to %s: %s\n",
                                    i, npa_str, hb_mc_strerror(err));
                        return err;
                }
        }

        return HB_MC_SUCCESS;
}

/*
 * Read in word vector manycore NPAs
 */
static int read_in_data(void)
{
        int i, err;

        err = hb_mc_manycore_read_mem_scatter_gather(mc, target_npas, in, WORDS);
        if (err != HB_MC_SUCCESS) {
                char npa_str[256];
                hb_mc_npa_to_string(&target_npas[i], npa_str, sizeof(npa_str));
                test_pr_err("failed to scatter-gather read: %s\n",
                            hb_mc_strerror(err));
                return err;
        }
        return HB_MC_SUCCESS;
}

static int compare(void)
{
        int i;

        for (i = 0; i < WORDS; i++)
                bsg_pr_info("out[%2d] = %08" PRIx32 ", in[%2d] = %08" PRIx32 "\n",
                            i, out[i], i, in[i]);

        if (memcmp(out, in, sizeof(in)) == 0) {
                return HB_MC_SUCCESS;
        } else {
                return HB_MC_FAIL;
        }
}

static int run_tests(void)
{
        int err, rc = HB_MC_FAIL;

        err = hb_mc_manycore_init(mc, TEST_NAME, 0);
        if (err != HB_MC_SUCCESS) {
                test_pr_err("failed to initialize manycore: %s\n",
                            hb_mc_strerror(err));
                goto done;
        }

        initialize_target_npas();
        initialize_out_data();

        err = write_out_data();
        if (err != HB_MC_SUCCESS)
                goto cleanup;

        err = read_in_data();
        if (err != HB_MC_SUCCESS)
                goto cleanup;

        rc = compare();

cleanup:
        hb_mc_manycore_exit(mc);
done:
        return rc;

}

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif

        bsg_pr_test_info(TEST_NAME " Regression Test \n");
        int rc = run_tests();
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}

