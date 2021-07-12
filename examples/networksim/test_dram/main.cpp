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

#include <bsg_manycore.h>
#include <bsg_manycore_npa.h>
#include <bsg_manycore_tile.h>
#include <bsg_manycore_vcache.h>
#include <bsg_manycore_regression.h>


#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif
        bsg_pr_test_info(__FILE__ " Regression Test \n");
        int err = HB_MC_SUCCESS;
        hb_mc_manycore_t manycore = {0}, *mc = &manycore;
        hb_mc_coordinate_t origin, dim, target, dram;
        hb_mc_npa_t npa;
        uint32_t write_data = 0, read_data = 0;
        hb_mc_coordinate_t pod = {.x = 0, .y = 0};

        unsigned int data[32] = {
                0, 1, 2, 3, 4, 5, 6, 7,
                8, 9, 10, 11, 12, 13, 14, 15,
                16, 17, 18, 19, 20, 21, 22, 23,
                24, 25, 26, 27, 28, 29, 30, 31};
        int len = sizeof(data)/sizeof(*data);
        uint32_t expected = len * (len - 1)/2;

        hb_mc_eva_t dram_eva;
        size_t sz = 0;
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_init(mc, __FILE__, 0));

        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        origin = hb_mc_config_pod_vcore_origin(cfg, pod);
        dim = hb_mc_config_get_dimension_vcore(cfg);

        target.x = origin.x + dim.x - 1;
        target.y = origin.y + dim.y - 1;

        // Construct DRAM EVA for cache at index 0.
        dram = hb_mc_config_pod_dram_start(cfg, pod);
        npa.x = dram.x;
        npa.y = dram.y;
        npa.epa = HB_MC_VCACHE_EPA_BASE;
        BSG_MANYCORE_CALL(mc, hb_mc_npa_to_eva(mc, &default_map, &target, &npa, &dram_eva, &sz));

        // Write Buffer to DRAM
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_eva_write(mc, &default_map, &target, &dram_eva, data, sizeof(data)));

        // Writes to DMEM, use the target x/y
        npa.x = target.x;
        npa.y = target.y;

        bsg_pr_test_info("Writing buffer EVA to Tile DMEM\n");
        write_data = dram_eva;
        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + 4;
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

        bsg_pr_test_info("Writing buffer length to Tile DMEM\n");
        write_data = len;
        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + 8;
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

        bsg_pr_test_info("Writing iteration pointer into to Tile DMEM\n");
        write_data = dram_eva;
        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + 12;
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

        // Set the origin of the target tile
        BSG_MANYCORE_CALL(mc, hb_mc_tile_set_origin(mc, &target, &origin));

        // Fence to make sure host credits return to their origin.
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_host_request_fence(mc, -1));

        bsg_pr_test_info("Unfreezing target(s)...\n");
        BSG_MANYCORE_CALL(mc, hb_mc_tile_unfreeze(mc, &target));

        // The target(s) will now add all of the numbers in the buffer together.


        bsg_pr_test_info("Waiting for finish packet...\n");
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_wait_finish(mc, -1));

        // After the finish packet, read the result from DMEM
        bsg_pr_test_info("Reading result from DMEM\n");
        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + hb_mc_config_get_dmem_size(cfg) - 4;

        BSG_MANYCORE_CALL(mc, hb_mc_manycore_read_mem(mc, &npa, &read_data, sizeof(read_data)));

        if(read_data != expected){
                bsg_pr_err("%s: Read data (%d) and expected (%d) do not match\n",
                           __func__,read_data, expected);
                return HB_MC_FAIL;
        }

        bsg_pr_test_info("Read successful\n");

        // Freeze the tile
        BSG_MANYCORE_CALL(mc, hb_mc_tile_freeze(mc, &target));

        // Fence to make sure host credits return to their origin.
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_host_request_fence(mc, -1));

        BSG_MANYCORE_CALL(mc, hb_mc_manycore_exit(mc));
        bsg_pr_test_pass_fail(err == HB_MC_SUCCESS);
        return err;
};

