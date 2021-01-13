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
#include <bsg_manycore_dpi_tile.hpp>
#include <cl_manycore_regression.h>


#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif
        bsg_pr_test_info(__FILE__ " Regression Test \n");
        bool finish = false;
        hb_mc_packet_t pkt;
        int err;
        hb_mc_manycore_t manycore = {0}, *mc = &manycore;
        hb_mc_coordinate_t origin, dim, max, dram, target;
        hb_mc_npa_t npa;
        uint32_t write_data = 0, read_data = 0;

        unsigned int *data, nels, fill_factor, expected = 0;

        hb_mc_eva_t dram_eva;
        size_t sz = 0;
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_init(mc, __FILE__, 0));

        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        origin = hb_mc_config_get_origin_vcore(cfg);
        dim = hb_mc_config_get_dimension_vcore(cfg);

        max.x = origin.x + dim.x - 1;
        max.y = origin.y + dim.y - 1;

        // Write enough data to fill all caches by fill_factor
        fill_factor = 1;
        nels = dim.x * 2 * fill_factor *
                hb_mc_config_get_vcache_ways(cfg) *
                hb_mc_config_get_vcache_sets(cfg) *
                hb_mc_config_get_vcache_block_size(cfg) / sizeof(unsigned int);

        data = new unsigned int[nels];

        // Fill buffer with data
        for (int i = 0 ; i < nels; ++i){
                data[i] = i;
                // This will overflow, but that's ok because add is
                // commutative. Other solutions (like xor) are not.
                expected += i;
        }

        // Construct DRAM EVA for cache at index 0.
        dram = hb_mc_config_get_dram_coordinate(cfg, 0);
        npa.x = dram.x;
        npa.y = dram.y;
        npa.epa = HB_MC_VCACHE_EPA_BASE;
        BSG_MANYCORE_CALL(mc, hb_mc_npa_to_eva(mc, &default_map, &origin, &npa, &dram_eva, &sz));

        // Write Buffer to DRAM
        //BSG_MANYCORE_CALL(mc, hb_mc_manycore_eva_write(mc, &default_map, &origin, &dram_eva, data, nels * sizeof(unsigned int)));


        // Write metadata to DMEM on each tile
        bsg_pr_test_info("Writing metadata to DMEM of each tile\n");
        for(hb_mc_idx_t x_i = origin.x; x_i <= max.x; ++x_i){
                for(hb_mc_idx_t y_i = origin.y; y_i <= max.y; ++y_i){
                        npa.x = x_i;
                        npa.y = y_i;
                        target.x = x_i;
                        target.y = y_i;

                        // DRAM Pointer
                        write_data = dram_eva;
                        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + 4;
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

                        // Number of elements in buffer
                        write_data = nels;
                        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + 8;
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

                        // Iteration pointer (for the tile)
                        write_data = dram_eva;
                        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + 12;
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

                        // Tile Group X
                        write_data = dim.x;
                        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + 16;
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

                        // Tile Group Y
                        write_data = dim.y;
                        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + 20;
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

                        // Set the origin of the target tile
                        BSG_MANYCORE_CALL(mc, hb_mc_tile_set_origin(mc, &target, &origin));
                }
        }

        // Fence to make sure host credits return to their origin.
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_host_request_fence(mc, -1));

        bsg_pr_test_info("Unfreezing target(s)...\n");
        for(hb_mc_idx_t x_i = origin.x; x_i <= max.x; ++x_i){
                for(hb_mc_idx_t y_i = origin.y; y_i <= max.y; ++y_i){
                        target.x = x_i;
                        target.y = y_i;
                        BSG_MANYCORE_CALL(mc, hb_mc_tile_unfreeze(mc, &target));
                }
        }


        // The target(s) will now add all of the numbers in the buffer together.

        // Wait for finish from all tiles.
        bsg_pr_test_info("Waiting for finish packet...\n");
        for(hb_mc_idx_t x_i = origin.x; x_i <= max.x; ++x_i){
                for(hb_mc_idx_t y_i = origin.y; y_i <= max.y; ++y_i){
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_wait_finish(mc, -1));
                        bsg_pr_info("Received finish packet\n");
                }
        }

        bsg_pr_test_info("Reading result from DMEM\n");
        for(hb_mc_idx_t x_i = origin.x; x_i <= max.x; ++x_i){
                for(hb_mc_idx_t y_i = origin.y; y_i <= max.y; ++y_i){
                        npa.x = x_i;
                        npa.y = y_i;
                        target.x = x_i;
                        target.y = y_i;
                        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + hb_mc_config_get_dmem_size(cfg) - 4;

                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_read_mem(mc, &npa, &read_data, sizeof(read_data)));

                        if(read_data != expected){
                                bsg_pr_err("%s (X:%d, y:%d): Read data (%x) and expected (%x) do not match\n",
                                           __func__, target.x, target.y, read_data, expected);
                                //return HB_MC_FAIL;
                        }
                }
        }

        // Freeze the tile
        bsg_pr_test_info("Freezing Tiles\n");
        for(hb_mc_idx_t x_i = origin.x; x_i <= max.x; ++x_i){
                for(hb_mc_idx_t y_i = origin.y; y_i <= max.y; ++y_i){
                        target.x = x_i;
                        target.y = y_i;
                        BSG_MANYCORE_CALL(mc, hb_mc_tile_freeze(mc, &target));
                }
        }

        // Fence to make sure host credits return to their origin.
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_host_request_fence(mc, -1));

        BSG_MANYCORE_CALL(mc, hb_mc_manycore_exit(mc));
        bsg_pr_test_pass_fail(true);
        return HB_MC_SUCCESS;
};


// This method executes requests to dmem, icache, and csr-space like
// any normal tile.
void BsgDpiTile::execute_request(const hb_mc_request_packet_t *req,
                                 hb_mc_response_packet_t *rsp){
        this->default_request_handler(req, rsp);
}


// This is the traffic generator method. It will not be called until
// the tile is unfrozen, and then it will be called on every cycle
// that a packet can be sent.

// WARNING: Do not use static variables for iteration, they
// are shared between all instances of a class (unless the
// templates are different)

// Instead, store iteration variables in DMEM.
int idx = 0; // Only used by origin
void BsgDpiTile::send_request(bool *req_v_o, hb_mc_request_packet_t *req_o){
        uint32_t *dmem_p = reinterpret_cast<uint32_t *>(this->dmem);
        // The host writes these values before unfreezing the tile.
        hb_mc_eva_t &base = dmem_p[1];
        uint32_t &nels = dmem_p[2];
        // ptr is our iteration variable. It is set by the host.
        hb_mc_eva_t &ptr = dmem_p[3];
        hb_mc_coordinate_t tg_dim = {.x = dmem_p[4], .y = dmem_p[5]};

        if(wait_at_barrier(0, tg_dim))
                return;

        if(is_origin() && idx == 0){
                *req_v_o = get_packet_stat_kernel_start(req_o);
                idx ++;
                return;
        }

        // This if statement is effectively a loop, since the method
        // is called on every cycle. Returning is effectively a
        // python-esque yield statement.
        hb_mc_eva_t limit = base + nels * sizeof(uint32_t);
        if(ptr < limit){
                // Fence every 32 requests
                if(!((ptr / sizeof(uint32_t)) % max_credits) && fence())
                        return;
                *req_v_o = get_packet_from_eva<uint32_t>(req_o, ptr);
                // You MUST increment before returning.
                ptr += sizeof(uint32_t);
                return;
        }

        // Wait until all requests have returned, so that the data is
        // accumulated. Do not send packets while we are fencing.
        if(fence())
                return;

        if(wait_at_barrier(1, tg_dim))
                return;

        if(is_origin() && idx == 1){
                *req_v_o = get_packet_stat_kernel_start(req_o);
                idx ++;
                return;
        }

        // Send the finish packet, once
        *req_v_o = get_packet_finish(req_o);

        // Setting finished to true means this method will no longer
        // be called.
        finished = true;

        return;
}

void BsgDpiTile::receive_response(const hb_mc_response_packet_t *rsp_i){
        uint32_t *dmem_p = reinterpret_cast<uint32_t *>(this->dmem);

        dmem_p[dmem_sz/sizeof(*dmem_p) - 1] += rsp_i->data;
}

