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

#define DEBUG 
#include <bsg_manycore.h>
#include <bsg_manycore_eva.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_errno.h>
#include <bsg_manycore_tile.h>
#include <inttypes.h>
#include "../cl_manycore_regression.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define TEST_NAME "test_manycore_credits"

int test_manycore_credits() {
        /********/
        /* INIT */
        /********/
        int rc, i;
        hb_mc_manycore_t manycore = {0}, *mc = &manycore;

        srand(time(0));
        
        rc = hb_mc_manycore_init(mc, TEST_NAME, 0);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("%s: failed to intialize manycore: %s\n",
                           __func__,
                           hb_mc_strerror(rc));
                return HB_MC_FAIL;
        }

        const hb_mc_config_t *config = hb_mc_manycore_get_config(mc);
        hb_mc_idx_t dim_x = hb_mc_coordinate_get_x(hb_mc_config_get_dimension_vcore(config));
        hb_mc_idx_t dim_y = hb_mc_coordinate_get_y(hb_mc_config_get_dimension_vcore(config));
        hb_mc_coordinate_t target = hb_mc_config_get_origin_vcore(config);

        hb_mc_eva_t eva;
        size_t dmem_size = hb_mc_tile_get_size_dmem(mc, &target); 
        size_t dmem_words = dmem_size/sizeof(uint32_t);
        uint32_t write_data[dmem_words];
        uint32_t read_data[dmem_words];

        /**************************************************************/
        /* Perform an EVA write that fills DMEM of a single tile.     */
        /**************************************************************/
        eva = 0x1000;

        for (i = 0; i < dmem_words; ++i){
                write_data[i] = rand() % ((1 << 16)-1);
        }
        rc = hb_mc_manycore_eva_write(mc, &default_map, &target, &eva, write_data, dmem_size);
                                
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("%s: failed to write buffer to EVA 0x%x of coord (%d,%d)\n",
                        __func__, eva,
                        hb_mc_coordinate_get_x(target), 
                        hb_mc_coordinate_get_y(target));
                goto cleanup;
        }
        bsg_pr_info("Successfully fill DMEM of tile (%d, %d)\n",
                hb_mc_coordinate_get_x(target), 
                hb_mc_coordinate_get_y(target));

        rc = hb_mc_manycore_eva_read(mc, &default_map, &target, &eva, read_data, dmem_size);
                                
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("%s: failed to read buffer to EVA 0x%x of coord (%d,%d)\n",
                        __func__, eva,
                        hb_mc_coordinate_get_x(target),
                        hb_mc_coordinate_get_y(target));
                goto cleanup;
        }

        rc = HB_MC_SUCCESS;
        /********/
        /* EXIT */
        /********/
cleanup:
        hb_mc_manycore_exit(mc);
        return rc;
}

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif

        bsg_pr_test_info(TEST_NAME " Regression Test \n");
        int rc = test_manycore_credits();
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}

