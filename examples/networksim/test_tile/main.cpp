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
#include <bsg_manycore_packet.h>
#include <bsg_manycore_regression.h>
#include <bsg_manycore_tile.h>

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif
        bsg_pr_test_info(__FILE__ " Regression Test \n");
        int err = HB_MC_SUCCESS;
        hb_mc_manycore_t manycore = {0}, *mc = &manycore;
        hb_mc_coordinate_t origin, dim, target;
        hb_mc_npa_t npa;
        uint32_t write_data = 0, read_data = 0;

        BSG_MANYCORE_CALL(mc, hb_mc_manycore_init(mc, __FILE__, 0));

        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        origin = hb_mc_config_get_origin_vcore(cfg);
        dim = hb_mc_config_get_dimension_vcore(cfg);

        target.x = origin.x + dim.x - 1;
        target.y = origin.y + dim.y - 1;
        npa.x = target.x;
        npa.y = target.y;


        bsg_pr_test_info("Writing to DMEM... \n");
        write_data = rand();
        npa.epa =  HB_MC_TILE_EPA_DMEM_BASE;
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));
        bsg_pr_test_info("Write successful\n");


        bsg_pr_test_info("Reading from DMEM...\n");
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_read_mem(mc, &npa, &read_data, sizeof(read_data)));
        bsg_pr_test_info("Read successful\n");

        if(read_data != write_data){
                bsg_pr_err("%s: Read data (%x) and write data (%x) do not match\n",
                           __func__,read_data, write_data);
                return HB_MC_FAIL;
        }
        read_data = 0;

        // Phase Counter (for the tile)
        bsg_pr_test_info("Writing to Phase COunter... \n");
        write_data = 0;
        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + 4;
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

        bsg_pr_test_info("Writing to ICACHE...\n");
        write_data = rand();
        npa.epa =  HB_MC_TILE_EPA_ICACHE_BASE;
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));
        bsg_pr_test_info("Write successful\n");


        // Read from ICACHE. This isn't supported in RTL, but might be
        // a feature for extra storage in the future. This will cause
        // a warning from the DPI Tile.
        bsg_pr_test_info("Reading from ICACHE...\n");
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_read_mem(mc, &npa, &read_data, sizeof(read_data)));
        bsg_pr_test_info("Read successful\n");

        if(read_data != write_data){
                bsg_pr_err("%s: Read data (%x) and write data (%x) do not match\n",
                           __func__,read_data, write_data);
                return HB_MC_FAIL;
        }

        // Fence to make sure host credits return to their origin.
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_host_request_fence(mc, -1));

        // Unfreeze the target and wait for a finish packet
        bsg_pr_test_info("Unfreezing target(s)...\n");
        BSG_MANYCORE_CALL(mc, hb_mc_tile_unfreeze(mc, &target));

        bsg_pr_test_info("Waiting for finish packet...\n");
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_wait_finish(mc, -1));

        // Freeze the tile
        BSG_MANYCORE_CALL(mc, hb_mc_tile_freeze(mc, &target));

        // Fence to make sure host credits return to their origin.
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_host_request_fence(mc, -1));

        BSG_MANYCORE_CALL(mc, hb_mc_manycore_exit(mc));
        bsg_pr_test_pass_fail(err == HB_MC_SUCCESS);
        return err;
};
