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
#include <bsg_manycore_coordinate.h>
#include <bsg_manycore_tile.h>
#include <bsg_manycore_vcache.h>
#include <bsg_manycore_cuda.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_config_pod.h>
#include <bsg_manycore_regression.h>
#include <inttypes.h>
#include <vector>

#define CHECK(stmt)                                             \
    do {                                                        \
        if (!(stmt)) {                                          \
            bsg_pr_err("'%s' failed\n", #stmt);                 \
            return HB_MC_FAIL;                                  \
        }                                                       \
    } while (0)


#define CHECK_M(stmt, m)                                        \
    do {                                                        \
        if (!(stmt)) {                                          \
            bsg_pr_err("'%s' failed: %s\n", #stmt, m);          \
            return HB_MC_FAIL;                                  \
        }                                                       \
    } while (0)

int test_pod_iteration (int argc, char **argv) {
    hb_mc_manycore_t mc = {};
    BSG_CUDA_CALL(hb_mc_manycore_init(&mc, "test_pod_iteration", HB_MC_DEVICE_ID));

    const hb_mc_config_t *cfg = hb_mc_manycore_get_config(&mc);
    // iterate over each pod and report its origin coordinate
    hb_mc_coordinate_t pod;
    hb_mc_config_foreach_pod(pod, cfg)
    {
        hb_mc_coordinate_t og = hb_mc_config_pod_vcore_origin(cfg, pod);
        char pod_str [256];
        char pod_og_str [256];

        bsg_pr_info("Pod %s: Origin Core at %s\n",
                    hb_mc_coordinate_to_string(pod, pod_str, sizeof(pod_str)),
                    hb_mc_coordinate_to_string(og, pod_og_str, sizeof(pod_og_str)));
    }

    // itarate over each pod, and iterate over its dram banks
    int i = 0;
    hb_mc_config_foreach_pod(pod, cfg)
    {
        hb_mc_coordinate_t dram;
        char pod_str[256];
        hb_mc_coordinate_to_string(pod, pod_str, sizeof(pod_str));
        int j = 0;
        hb_mc_config_pod_foreach_dram(dram, pod, cfg)
        {
            char dram_str [256];
            char pod_test_str[256];
            hb_mc_coordinate_t pod_test = hb_mc_config_pod(cfg, dram);
            hb_mc_coordinate_to_string(dram, dram_str, sizeof(dram_str));
            hb_mc_coordinate_to_string(pod_test, pod_test_str, sizeof(pod_test_str));
            bsg_pr_info("Pod %s: DRAM %2d at %s\n", pod_str, i, dram_str);
            CHECK(hb_mc_config_is_dram(cfg, dram) == 1);
            CHECK(hb_mc_config_is_vanilla_core(cfg, dram) == 0);
            CHECK(hb_mc_config_is_host(cfg, dram) == 0);
            CHECK_M(hb_mc_config_dram_id(cfg, dram) == i,
                    "DRAM ID does not match iteration order");
            CHECK_M(pod_test.x == pod.x && pod_test.y == pod.y,
                    "Pod(xy) function produces incorrect pod");
            CHECK_M(hb_mc_config_pod_dram_id(cfg,dram) == j,
                    "Pod DRAM ID does not match itertion order");
            // read a word of DRAM
            uint32_t word;
            hb_mc_npa_t mem = hb_mc_npa(dram, HB_MC_VCACHE_EPA_OFFSET_DRAM);
            BSG_CUDA_CALL(hb_mc_manycore_read32(&mc, &mem, &word));
            j++;
            i++;
        }
    }

    // iterate over each pod, and iterate over its tiles
    i = 0;
    hb_mc_config_foreach_pod(pod, cfg)
    {
        hb_mc_coordinate_t core;
        char pod_str[256];
        hb_mc_coordinate_to_string(pod, pod_str, sizeof(pod_str));
        int j = 0;
        hb_mc_config_pod_foreach_vcore(core, pod, cfg)
        {
            char core_str[256];
            char pod_test_str[256];
            hb_mc_coordinate_t pod_test = hb_mc_config_pod(cfg, core);
            hb_mc_coordinate_to_string(core, core_str, sizeof(core_str));
            hb_mc_coordinate_to_string(pod_test, pod_test_str, sizeof(pod_test_str));
            bsg_pr_info("Pod %s: Tile %2d at %s\n", pod_str, i, core_str);
            CHECK(hb_mc_config_is_dram(cfg,core) == 0);
            CHECK(hb_mc_config_is_vanilla_core(cfg, core) == 1);
            CHECK(hb_mc_config_is_host(cfg, core) == 0);
            CHECK_M(pod_test.x == pod.x && pod_test.y == pod.y,
                    "Pod(xy) function produces incorrect pod");

            // read a word of DMEM
            uint32_t word;
            hb_mc_npa_t dmem = hb_mc_npa(core, HB_MC_TILE_EPA_DMEM_BASE);
            BSG_CUDA_CALL(hb_mc_manycore_read32(&mc, &dmem, &word));
            i++;
            j++;
        }
    }


    BSG_CUDA_CALL(hb_mc_manycore_exit(&mc));
    return HB_MC_SUCCESS;
}

declare_program_main("test_pod_iteration", test_pod_iteration);
