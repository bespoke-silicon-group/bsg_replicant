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

#include "test_manycore_eva.h"
#include "test_manycore_parameters.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>

typedef enum __hb_mc_region_t{
        LOCAL = 0,
        GROUP = 1,
        GLOBAL = 2,
        DRAM = 3,
        NUM_REGIONS = 4
} hb_mc_region_t;

#define NETWORK_ADDRESS_BITS 32
#define NUM_REGIONS 4 // Local, Group, Global, DRAM
#define NUM_EVAS 100
#define NUM_NPAS 100

int test_manycore_eva () {
        int rc = 0, fail = 0, i = 0;
        uint8_t dram_x_offset;
        hb_mc_region_t region;
        hb_mc_npa_t npa;
        size_t npa_sz;
        hb_mc_epa_t epa;
        hb_mc_eva_t eva;
        hb_mc_dimension_t dim;
        hb_mc_idx_t dim_x, dim_y;

        hb_mc_coordinate_t src;
        hb_mc_idx_t src_x, src_y;

        hb_mc_coordinate_t origin;
        hb_mc_idx_t origin_x, origin_y;

        hb_mc_coordinate_t tgt;
        hb_mc_idx_t tgt_x, tgt_y;
        hb_mc_epa_t tgt_epa;
        size_t tgt_sz;

        hb_mc_npa_t result_npa;
        hb_mc_epa_t result_epa;
        hb_mc_coordinate_t result_c;
        hb_mc_idx_t result_x, result_y;
        size_t result_sz;

        const hb_mc_config_t *config;
        hb_mc_manycore_t mc = {0};

        srand(42);

        rc = hb_mc_manycore_init(&mc, "manycore@test_manycore_eva", 0);
        if(rc != HB_MC_SUCCESS){
                bsg_pr_test_err("Failed to initialize manycore device: %s\n",
                                hb_mc_strerror(rc));
                return HB_MC_FAIL;
        }

        config = hb_mc_manycore_get_config(&mc);
        dim = hb_mc_config_get_dimension_vcore(config);
        dim_x = hb_mc_dimension_get_x(dim);
        dram_x_offset = DRAM_STRIPE_WIDTH;

        if(dim_x > GROUP_X_MAX){
                bsg_pr_test_err("Manycore X dimension is larger than GROUP_X_MAX");
                return HB_MC_FAIL;
        }
        if(dim_x > GLOBAL_X_MAX){
                bsg_pr_test_err("Manycore X dimension is larger than GLOBAL_X_MAX");
                return HB_MC_FAIL;
        }
        dim_y = hb_mc_dimension_get_y(dim);
        if(dim_y > GROUP_Y_MAX){
                bsg_pr_test_err("Manycore Y dimension is larger than GROUP_Y_MAX");
                return HB_MC_FAIL;
        }
        if(dim_y > GLOBAL_Y_MAX){
                bsg_pr_test_err("Manycore Y dimension is larger than GLOBAL_Y_MAX");
                return HB_MC_FAIL;
        }

        bsg_pr_test_info("Testing Random Manycore EVAs\n");
        // This is the origin of the Default EVA ID
        origin_x = hb_mc_config_get_vcore_base_x(config);
        origin_y = hb_mc_config_get_vcore_base_y(config);
        origin = hb_mc_coordinate(origin_x, origin_y);

        src_x = 1;
        src_y = 1;
        src = hb_mc_coordinate(src_x, src_y);
        for( i = 0; i < NUM_EVAS; ++i){
                region = rand() % NUM_REGIONS;
                if(region == LOCAL){
                        eva = rand() % DMEM_EPA_SIZE;
                        tgt_sz = DMEM_EPA_SIZE - eva;
                        tgt_epa = eva + DMEM_EPA_OFFSET;
                        eva = eva + DMEM_EVA_OFFSET;
                        bsg_pr_test_info("Creating Local EVA: 0x%x\n", eva);

                        tgt_x = src_x;
                        tgt_y = src_y;
                } else if (region == GROUP){
                        eva = rand() % DMEM_EPA_SIZE;
                        tgt_sz = DMEM_EPA_SIZE - eva;
                        tgt_epa = eva + DMEM_EPA_OFFSET;
                        eva = eva + DMEM_EVA_OFFSET;

                        tgt_x = (rand() % (dim_x - origin_x)) + origin_x;
                        tgt_y = (rand() % ((dim_y / 2) - origin_y)) + origin_y;

                        eva = (GROUP_INDICATOR) | ((tgt_x - origin_x) << GROUP_X_OFFSET) | 
                                ((tgt_y - origin_y) << GROUP_Y_OFFSET) | (eva);

                        bsg_pr_test_info("Creating GROUP EVA: 0x%x\n", eva);
                } else if (region == GLOBAL){
                        eva = rand() % DMEM_EPA_SIZE;
                        tgt_sz = DMEM_EPA_SIZE - eva;
                        tgt_epa = eva + DMEM_EPA_OFFSET;
                        eva = eva + DMEM_EVA_OFFSET;

                        tgt_x = (rand() % (dim_x - 1)) + origin_x;
                        tgt_y = rand() % dim_y;

                        eva = (GLOBAL_INDICATOR) | (tgt_x << GLOBAL_X_OFFSET) | 
                                (tgt_y << GLOBAL_Y_OFFSET) | (eva);

                        bsg_pr_test_info("Creating GLOBAL EVA: 0x%x\n", eva);
                } else if (region == DRAM){
                        tgt_epa = rand() % DRAM_EPA_SIZE; // Small, but we'll deal.

                        tgt_x = (rand() % (dim_x - 1)) + origin_x;
                        tgt_y = hb_mc_config_get_dram_y(config);

                        eva =   (  (tgt_epa & DRAM_STRIPE_MASK)
                                 | (tgt_x << dram_x_offset)
                                 | ((tgt_epa >> dram_x_offset) << ((int)(dram_x_offset + ceil(log2(dim_x)))))
                                 | DRAM_INDICATOR );

                        bsg_pr_test_info("Creating DRAM EVA: 0x%x\n", eva);
                        tgt_sz = (1 << dram_x_offset) - (tgt_epa & DRAM_STRIPE_MASK);
                } else {
                        bsg_pr_test_err("Invalid Region number %d\n", region);
                        return HB_MC_FAIL;
                }
                
                result_x = 0; result_y = 0; result_epa = 0; result_sz = 0;
                rc = hb_mc_eva_to_npa(&mc, &default_map, &src,
                                &eva, &result_npa, &result_sz);
                if(rc != HB_MC_SUCCESS){
                        bsg_pr_test_err("Call to hb_mc_manycore_eva_to_npa failed\n", eva);
                        fail = 1;
                }

                result_x = hb_mc_npa_get_x(&result_npa);
                result_y = hb_mc_npa_get_y(&result_npa);
                result_epa = hb_mc_npa_get_epa(&result_npa);

                if(result_x != tgt_x){
                        bsg_pr_test_err("EVA To NPA: Unexpected NPA X Coordinate. Expected %d, got %d\n", 
                                        tgt_x, result_x);
                        fail = 1;
                }

                if(result_y != tgt_y){
                        bsg_pr_test_err("EVA To NPA: Unexpected NPA Y Coordinate. Expected %d, got %d\n", 
                                        tgt_y, result_y);
                        fail = 1;
                }

                if(result_epa != tgt_epa){
                        bsg_pr_test_err("EVA To NPA: Unexpected NPA EPA. Expected 0x%x, got 0x%x\n", 
                                        tgt_epa, result_epa);
                        fail = 1;
                }

                if(result_sz != tgt_sz){
                        bsg_pr_test_err("EVA To NPA: Unexpected NPA SZ. Expected 0x%x, got 0x%x\n", 
                                        tgt_sz, result_sz);
                        fail = 1;
                }
        }

        bsg_pr_test_info("Creating Random Manycore NPAs\n");
        for( i = 0; i < NUM_NPAS; ++i){
        }

        hb_mc_manycore_exit(&mc);

        return fail ? HB_MC_FAIL : HB_MC_SUCCESS;
}

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif

        bsg_pr_test_info("test_manycore_eva Regression Test \n");
        int rc = test_manycore_eva();
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}

