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
#include "test_vcache_simplified.h"

#define WRITE_BLOCK_NUM 3
#define VCACHE_ADDR_WIDTH 9
#define EPA_ADDR_WIDTH 28 
#define BASE_ADDR 0x0000

static hb_mc_npa_t make_npa(hb_mc_idx_t x, hb_mc_idx_t y, hb_mc_epa_t addr)
{
    hb_mc_npa_t npa;
    hb_mc_npa_set_x(&npa, x);
    hb_mc_npa_set_y(&npa, y);
    hb_mc_npa_set_epa(&npa, addr);
    return npa;
}

int test_vcache_simplified() {
        hb_mc_manycore_t manycore = {0}, *mc = &manycore;
        int err, r = HB_MC_FAIL;

        srand(0);

        err = hb_mc_manycore_init(mc, "test_vcache_simplified", 0);
        if (err != HB_MC_SUCCESS) {
                bsg_pr_err("%s: failed to init manycore: %s\n",
                           __func__, hb_mc_strerror(err));
                return err;
        }

        const hb_mc_config_t *config = hb_mc_manycore_get_config(mc);
        uint32_t manycore_dim_x = hb_mc_coordinate_get_x(hb_mc_config_get_dimension_vcore(config));
        uint32_t manycore_dim_y = hb_mc_coordinate_get_y(hb_mc_config_get_dimension_vcore(config));

        // get the coordinate of the 1st logical victim cache
        hb_mc_coordinate_t dram_coord = hb_mc_config_get_dram_coordinate(config, 0);
        uint32_t dram_coord_x = hb_mc_coordinate_get_x(dram_coord);
        uint32_t dram_coord_y = hb_mc_coordinate_get_y(dram_coord);
        int mismatch = 0; 

        uint32_t tag_reset_addr;
        uint32_t zeros = 0;

         // 1st reset the tag of the vcache under test
        for (int i = 0; i < 2; i ++) {
                tag_reset_addr = (1 << (EPA_ADDR_WIDTH - 2 + 1)) + ((i+1) << VCACHE_ADDR_WIDTH); // tagst command to invalidate the tag
                hb_mc_npa_t npa = make_npa(dram_coord_x, dram_coord_y, tag_reset_addr);
                err = hb_mc_manycore_write_mem(mc, &npa, &zeros, sizeof(zeros));
                if (err != HB_MC_SUCCESS) {
                        bsg_pr_err("%s: failed to reset the vcache tag A[%d] at DRAM coord(%d,%d) addr 0x%08" PRIx32 ": %s\n",
                                   __func__, i, dram_coord_x, dram_coord_y, tag_reset_addr, hb_mc_strerror(err));
                        goto cleanup;
                } else {
                        bsg_pr_test_info("reset tag %d succesfully\n", i);
                }
        }

        // 2nd write to the same bank 3 times, so that the 3rd write will evict a cache line.   
        uint32_t A_host[WRITE_BLOCK_NUM];
        uint32_t A_device[WRITE_BLOCK_NUM];
        uint32_t epa_addr[WRITE_BLOCK_NUM];

        for (int i = 0; i < WRITE_BLOCK_NUM; i ++) {
                A_host[i] = 1 + i; // write none zero data
                epa_addr[i] = ((1 + i) << VCACHE_ADDR_WIDTH);  // write none zero address
        }
        
        for (int i = 0; i < WRITE_BLOCK_NUM; i ++) {
                hb_mc_npa_t npa = make_npa(dram_coord_x, dram_coord_y, BASE_ADDR + epa_addr[i]);
                err = hb_mc_manycore_write_mem(mc, &npa, &A_host[i], sizeof(A_host[i]));
                if (err != HB_MC_SUCCESS) {
                        bsg_pr_err("%s: failed to write A[%d] = %d to DRAM coord(%d,%d) addr 0x%08" PRIx32 ": %s\n",
                                   __func__, i, A_host[i], dram_coord_x, dram_coord_y, BASE_ADDR + epa_addr[i], hb_mc_strerror(err));
                        goto cleanup;
                }
        }
        
        for (int i = 0; i < WRITE_BLOCK_NUM; i ++) {
                hb_mc_npa_t npa = make_npa(dram_coord_x, dram_coord_y, BASE_ADDR + epa_addr[i]);
                err = hb_mc_manycore_read_mem(mc, &npa, &A_device[i], sizeof(A_device[i]));
                if (err != HB_MC_SUCCESS) {
                        bsg_pr_err("%s: failed to read A[%d] from DRAM coord (%d,%d) addr 0x%08" PRIx32 ": %s\n",
                                   __func__, i, dram_coord_x, dram_coord_y, BASE_ADDR + epa_addr[i], hb_mc_strerror(err));
                        goto cleanup;
                }
        }

        bsg_pr_test_info("Checking vcache at (%d,%d):\n", dram_coord_x, dram_coord_y); 
        for (int i = 0; i < WRITE_BLOCK_NUM; i ++) { 
                if (A_host[i] == A_device[i]) {
                        bsg_pr_test_info("%s: " BSG_GREEN("Success") " -- A_host[%d] = %d   ==   A_device[%d] = %d -- EPA: 0x%08" PRIx32 "\n",
                                   __func__, i, A_host[i], i, A_device[i], BASE_ADDR + epa_addr[i], hb_mc_strerror(err)); 
                }
                else { 
                        bsg_pr_test_info("%s: " BSG_RED("Failed") " -- A_host[%d] = %d   !=   A_device[%d] = %d -- EPA: 0x%08" PRIx32 "\n",
                                   __func__, i, A_host[i], i, A_device[i], BASE_ADDR + epa_addr[i]);
                        mismatch = 1;
                }
        }


        
        if (mismatch) 
                r = HB_MC_FAIL;
        else
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

        bsg_pr_test_info("test vcache simplified Regression Test \n");
        int rc = test_vcache_simplified();
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}

