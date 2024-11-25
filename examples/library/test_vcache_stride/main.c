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

/**
 * test_vcache_stride
 *
 *
 */
#include <bsg_manycore.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <bsg_manycore_regression.h>
#include <bsg_manycore_config_pod.h>

#define STRIDE_SIZE 1024
#define DRAM_BASE 0x0000
#define NUM_STRIDES 16

int test_vcache_stride(int argc, char *argv[]) {
        int rc;
        hb_mc_manycore_t mc = HB_MC_MANYCORE_INIT;
        const hb_mc_config_t *config;
        srand(time(0));

        rc = hb_mc_manycore_init(&mc, "manycore@test_vcache_stride", HB_MC_DEVICE_ID);
        if(rc != HB_MC_SUCCESS){
                bsg_pr_test_err("Failed to initialize manycore device!\n");
                return HB_MC_FAIL;
        }

        config = hb_mc_manycore_get_config(&mc);

        /* To increase the number of DRAM banks tested, increase ndrams (must be
         * less than dim_x) and add the X coordinates to dim_x */
        hb_mc_epa_t epa;
        hb_mc_npa_t npa;
        int xidx, stride;
        uint32_t gold  [NUM_STRIDES] = {0};
        uint32_t result[NUM_STRIDES] = {0};
        uint32_t val;

        hb_mc_request_packet_t req;
        hb_mc_response_packet_t res;

        hb_mc_coordinate_t pod;
        hb_mc_config_foreach_pod(pod, config)
        {
                hb_mc_coordinate_t dest;
                hb_mc_config_pod_foreach_dram(dest, pod, config)
                {
                        hb_mc_idx_t dram_coord_x, dram_coord_y;
                        dram_coord_x = hb_mc_coordinate_get_x(dest);
                        dram_coord_y = hb_mc_coordinate_get_y(dest);
                        bsg_pr_test_info("Testing DRAM/Cache Interface at (%d, %d).\n", dram_coord_x, dram_coord_y);

                        for (stride = 0; stride < NUM_STRIDES; ++stride) {
                                gold[stride] = rand();
                        }

                        for (stride = 0; stride < NUM_STRIDES; ++stride) {
                                epa = DRAM_BASE + stride * STRIDE_SIZE;
                                dest = hb_mc_coordinate(dram_coord_x, dram_coord_y);
                                npa = hb_mc_epa_to_npa(dest, epa);
                                val = gold[stride];

                                bsg_pr_test_info("%s -- Writing value %08x to 0x%08x @ (%d, %d)\n",
                                                 __func__, val, epa, dram_coord_x, dram_coord_y);
                                rc = hb_mc_manycore_write32(&mc, &npa, val);
                                if(rc != HB_MC_SUCCESS) {
                                        bsg_pr_test_err("%s -- hb_mc_write32 failed on interation %d!\n", __func__, stride);
                                        return HB_MC_FAIL;
                                }
                                val = ~val;

                                rc = hb_mc_manycore_read32(&mc, &npa, &val);
                                if(rc != HB_MC_SUCCESS) {
                                        bsg_pr_test_err("%s -- hb_mc_read32 failed on iteration %d!\n", __func__, stride);
                                        return HB_MC_FAIL;
                                }
                                bsg_pr_test_info("%s -- Read value %08x from 0x%08x @ (%d, %d)\n",
                                                 __func__, val, epa, dram_coord_x, dram_coord_y);
                                result[stride] = val;
                        }
                }
        }
        for (stride = 0; stride < NUM_STRIDES; ++stride) {
                if(result[stride] != gold[stride]){
                        bsg_pr_test_err("%s -- Index %d: Result, %08x, did not match expected, %08x!\n",
                                        __func__, stride, result[stride], gold[stride]);
                        return HB_MC_FAIL;
                }
        }
        bsg_pr_test_info("%s -- %d Strides Passed\n", __func__, stride);

        return HB_MC_SUCCESS;
}

declare_program_main("test_vcache_stride", test_vcache_stride);
