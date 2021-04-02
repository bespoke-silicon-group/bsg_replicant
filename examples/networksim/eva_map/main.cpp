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

#include <random>
#include <limits>
#include <string.h>
#include <string>
#ifndef CACHE_START_TYPE
#define CACHE_START_TYPE "ruchy"
#endif
#define xstr(x) str(x)
#define str(x) #x
#ifndef CREDIT_ALLOCATION
#define CREDIT_ALLOCATION 32
#endif

uint32_t get_credit_alloc(uint32_t y,uint32_t x){
        uint32_t allocs[8][16];
        if (strcmp(xstr(CREDIT_ALLOCATION),"doubledist")==0){
                uint32_t allocs[8][16] = {{30, 28, 28, 28, 26, 26, 26, 24, 24, 26, 26, 26, 28, 28, 28, 30},
                                          {28, 26, 26, 26, 24, 24, 24, 22, 22, 24, 24, 24, 26, 26, 26, 28},
                                          {26, 24, 24, 24, 22, 22, 22, 20, 20, 22, 22, 22, 24, 24, 24, 26},
                                          {24, 22, 22, 22, 20, 20, 20, 18, 18, 20, 20, 20, 22, 22, 22, 24},
                                          {24, 22, 22, 22, 20, 20, 20, 18, 18, 20, 20, 20, 22, 22, 22, 24},
                                          {26, 24, 24, 24, 22, 22, 22, 20, 20, 22, 22, 22, 24, 24, 24, 26},
                                          {28, 26, 26, 26, 24, 24, 24, 22, 22, 24, 24, 24, 26, 26, 26, 28},
                                          {30, 28, 28, 28, 26, 26, 26, 24, 24, 26, 26, 26, 28, 28, 28, 30}};
                return allocs[y][x];
        }else if (strcmp(xstr(CREDIT_ALLOCATION),"normdist")==0){
                uint32_t allocs[8][16] = {{12, 11, 11, 11, 10, 10, 10,  9,  9, 10, 10, 10, 11, 11, 11, 12},
                                          {11, 10, 10, 10,  9,  9,  9,  8,  8,  9,  9,  9, 10, 10, 10, 11},
                                          {10,  9,  9,  9,  8,  8,  8,  8,  8,  8,  8,  8,  9,  9,  9, 10},
                                          { 9,  8,  8,  8,  8,  8,  8,  7,  7,  8,  8,  8,  8,  8,  8,  9},
                                          { 9,  8,  8,  8,  8,  8,  8,  7,  7,  8,  8,  8,  8,  8,  8,  9},
                                          {10,  9,  9,  9,  8,  8,  8,  8,  8,  8,  8,  8,  9,  9,  9, 10},
                                          {11, 10, 10, 10,  9,  9,  9,  8,  8,  9,  9,  9, 10, 10, 10, 11},
                                          {12, 11, 11, 11, 10, 10, 10,  9,  9, 10, 10, 10, 11, 11, 11, 12}};
                return allocs[y][x];
        } else {
                uint32_t val = std::stoi(xstr(CREDIT_ALLOCATION));
                uint32_t allocs[8][16] = {{val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val},
                                          {val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val},
                                          {val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val},
                                          {val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val},
                                          {val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val},
                                          {val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val},
                                          {val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val},
                                          {val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val}};
                return allocs[y][x];
        }
         
}

// cache_start_index(y_i-origin.y,x_i-origin.x)

int cache_start_index(int y,int x)
{
  if (strcmp(CACHE_START_TYPE,"ruchy")==0)
    return (y << 2) + ((0x24924924 >> (x << 1)) & 3);
  else
   if (strcmp(CACHE_START_TYPE,"ruchydeux")==0)
     return (y << 2) + (x & 3);
   else
   if (strcmp(CACHE_START_TYPE,"ruchytrois")==0)
     return (y << 2) + (x & 2);
   else
    if (strcmp(CACHE_START_TYPE,"xstripe")==0)
      return ((x << 1) + (y&1));
    else
    if (strcmp(CACHE_START_TYPE,"ystripe")==0)
      return (y << 2);
    else
      if (strcmp(CACHE_START_TYPE,"zero")==0)
	return 0;
      else
	return 0;
}



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
        hb_mc_coordinate_t origin, dim, max, tg, dram, target;
        hb_mc_npa_t npa;
        uint32_t write_data = 0, read_data = 0;
        hb_mc_coordinate_t pod = {.x = 0, .y = 0};

        unsigned int nels, niters, stride, stripe;

        hb_mc_eva_t dram_eva;
        size_t sz = 0;
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_init(mc, __FILE__, 0));

        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        origin = hb_mc_config_pod_vcore_origin(cfg, pod);
        dim = hb_mc_config_get_dimension_vcore(cfg);

        stripe = hb_mc_config_get_vcache_stripe_words(cfg);

        tg.x = dim.x;
        tg.y = dim.y;

        max.x = origin.x + tg.x - 1;
        max.y = origin.y + tg.y - 1;


        // Experiment configuration:
        //   Number of buffer elements
        //   Per-tile start offset
        //   Number of elements to stride
        //   Number of iterations
        nels = 1024;

        stride = 1;
        niters = 1024;

        float initial = 10000;
        float data[nels];
        float expected[max.x + 1][max.y + 1];

        std::default_random_engine generator;
        generator.seed(42);
        std::uniform_real_distribution<float> distribution(.9, 1.0/.9);
        for (uint32_t i = 0 ; i < nels; ++i){
                auto res = distribution(generator);
                data[i] = (res);
        }

        for(hb_mc_idx_t x_i = origin.x; x_i <= max.x; ++x_i){
                for(hb_mc_idx_t y_i = origin.y; y_i <= max.y; ++y_i){
                        for(uint32_t j = 0; j < niters; ++j){
                                expected[x_i][y_i] = initial;
                        }
                }
        }

        hb_mc_idx_t idx = 0;
        for(hb_mc_idx_t y_i = origin.y; y_i <= max.y; ++y_i){
                for(hb_mc_idx_t x_i = origin.x; x_i <= max.x; ++x_i){
		        //unsigned int cache_idx = ((y_i-origin.y) << 2) + ((0x24924924 >> (2*(x_i-origin.x))) & 3);
  		        unsigned int cache_idx = cache_start_index(y_i-origin.y,x_i-origin.x); 
                        int offset;
                        offset = stripe * cache_idx;
                        for(uint32_t j = 0; j < niters; ++j){
                                expected[x_i][y_i] = expected[x_i][y_i] / data[(j * stride + offset) % nels];
                        }
                        idx ++;
                }
        }

        // Set EVA Map on each tile
        bsg_dpi_tile_eva_map_id_t map = BSG_DPI_TILE_EVA_MAP_ID_TOPLRBOTRL;
        for(hb_mc_idx_t y_i = origin.y; y_i <= max.y; ++y_i){
                for(hb_mc_idx_t x_i = origin.x; x_i <= max.x; ++x_i){
                        npa.x = x_i;
                        npa.y = y_i;

                        // Set EVA Map
                        write_data = map;
                        npa.epa = HB_MC_TILE_EPA_CSR_EVA_MAP_OFFSET + HB_MC_TILE_EPA_CSR_BASE;
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));
                }
        }

        // Construct DRAM EVA for cache at index 0.
        dram = hb_mc_config_pod_dram_start(cfg, pod);
        npa.x = dram.x;
        npa.y = dram.y;
        npa.epa = HB_MC_VCACHE_EPA_BASE;
        BSG_MANYCORE_CALL(mc, hb_mc_npa_to_eva(mc, bsg_dpi_tile_eva_maps[map], &origin, &npa, &dram_eva, &sz));

        // Write Buffer to DRAM
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_eva_write(mc, bsg_dpi_tile_eva_maps[map], &origin, &dram_eva, reinterpret_cast<uint32_t*>(data), nels * sizeof(data[0])));

        // Write metadata to DMEM on each tile
        bsg_pr_test_info("Writing metadata to DMEM of each tile\n");
        idx = 0;
        for(hb_mc_idx_t y_i = origin.y; y_i <= max.y; ++y_i){
                for(hb_mc_idx_t x_i = origin.x; x_i <= max.x; ++x_i){
                        npa.x = x_i;
                        npa.y = y_i;
                        target.x = x_i;
                        target.y = y_i;

                        // DRAM Pointer to buffer
                        write_data = dram_eva;
                        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + 0;
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

                        // Number of elements in buffer
                        write_data = nels;
                        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + 4;
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

                        // Per-Tile Offset into buffer
                        //unsigned int cache_idx = ((y_i-origin.y) << 2) + ((0x24924924 >> (2*(x_i-origin.x))) & 3);
                        unsigned int cache_idx = cache_start_index(y_i-origin.y,x_i-origin.x); 
                        int offset;
                        offset = stripe * cache_idx;
                        write_data = offset;
                        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + 8;
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

                        // Linear stride multiple
                        write_data = stride;
                        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + 12;
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

                        // Iteration counter (for the tile)
                        write_data = 0;
                        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + 16;
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

                        // Iteration limit (for the tile)
                        write_data = niters;
                        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + 20;
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

                        // Phase Counter (for the tile)
                        write_data = 0;
                        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + 24;
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

                        // Tile Group X
                        write_data = tg.x;
                        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + 28;
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

                        // Tile Group Y
                        write_data = tg.y;
                        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + 32;
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

                        // Initial value / Final result
                        write_data = *reinterpret_cast<uint32_t*>(&initial);
                        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + hb_mc_config_get_dmem_size(cfg) - 4;
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

                        // Set the origin of the target tile
                        BSG_MANYCORE_CALL(mc, hb_mc_tile_set_origin(mc, &target, &origin));
                        idx ++;
                }
        }

        // Fence to make sure host credits return to their origin.
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_host_request_fence(mc, -1));

        // Enable Tracing
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_trace_enable(mc));

        bsg_pr_test_info("Unfreezing target(s)...\n");
        for(hb_mc_idx_t y_i = origin.y; y_i <= max.y; ++y_i){
                for(hb_mc_idx_t x_i = origin.x; x_i <= max.x; ++x_i){
                        target.x = x_i;
                        target.y = y_i;
                        BSG_MANYCORE_CALL(mc, hb_mc_tile_unfreeze(mc, &target));
                }
        }

        // The target(s) will now iterate through the array

        // Wait for finish from all tiles.
        bsg_pr_test_info("Waiting for finish packet...\n");
        for(hb_mc_idx_t y_i = origin.y; y_i <= max.y; ++y_i){
                for(hb_mc_idx_t x_i = origin.x; x_i <= max.x; ++x_i){
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_wait_finish(mc, -1));
                }
        }

        // Enable Tracing
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_trace_disable(mc));

        bsg_pr_test_info("Reading result from DMEM\n");
        for(hb_mc_idx_t y_i = origin.y; y_i <= max.y; ++y_i){
                for(hb_mc_idx_t x_i = origin.x; x_i <= max.x; ++x_i){
                        npa.x = x_i;
                        npa.y = y_i;
                        target.x = x_i;
                        target.y = y_i;
                        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + hb_mc_config_get_dmem_size(cfg) - 4;

                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_read_mem(mc, &npa, &read_data, sizeof(read_data)));

                        float *val = reinterpret_cast<float *>(&read_data);
                        float result = expected[x_i][y_i];
                        if(*val != result){
                                bsg_pr_err("%s (X:%d, y:%d): Read data (%f) and expected (%f) do not match\n",
                                           __func__, target.x, target.y, *val, result);
                        }
                }
        }

        // Freeze the tile
        bsg_pr_test_info("Freezing Tiles\n");
        for(hb_mc_idx_t y_i = origin.y; y_i <= max.y; ++y_i){
                for(hb_mc_idx_t x_i = origin.x; x_i <= max.x; ++x_i){
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

void BsgDpiTile::send_request(bool *req_v_o, hb_mc_request_packet_t *req_o){
        uint32_t *dmem_p = reinterpret_cast<uint32_t *>(this->dmem);
        // The host writes these values before unfreezing the tile.
        hb_mc_eva_t &base = dmem_p[0];
        uint32_t &nels = dmem_p[1];
        uint32_t &offset = dmem_p[2];
        uint32_t &stride = dmem_p[3];
        // iter is our iteration variable. It is set by the host.
        uint32_t &iter = dmem_p[4];
        uint32_t &limit = dmem_p[5];
        uint32_t &phase = dmem_p[6];
        hb_mc_coordinate_t tg_dim = {.x = dmem_p[7], .y = dmem_p[8]};

        // Wait for everyone to start
        if(wait_at_barrier(0, tg_dim))
                return;


        // Start stats for the vcache (but only send from origin)
        if(phase == 0){
                set_credit_limit(get_credit_alloc(me.y-origin.y, me.x-origin.x));
                if(is_origin()){
                        *req_v_o = get_packet_stat_kernel_start(req_o);
                }
                phase ++;
                return;
        }

        // Start stats for the tiles. Does not send a packet, but will
        // print lines to dpi_stats.csv
        if(phase == 1){
                if(fence())
                        return;
                get_packet_stat_tag_start(req_o, 0);
                phase ++;
        }

        if(wait_at_barrier(1, tg_dim))
                return;

        // This if statement is effectively a loop, since the method
        // is called on every cycle. Returning is effectively a
        // python-esque yield statement.
        if(iter < limit){
                // Every 32 loads, fence until all reads return.
                //if((iter % 32) == 0 && fence_read())
                //        return;
                *req_v_o = get_packet_from_eva<uint32_t>(req_o, base + ((iter*stride + offset) % nels) * sizeof(uint32_t));
                
                // Make each response take 1 cycle to
                // process. Responses are processed in order. This
                // emulates a 32-load, 32-store instruction pattern.
                set_packet_rx_cost(req_o, 1);

                // You MUST increment before returning.
                iter ++;
                return;
        }

        // Wait until all requests have returned, so that the data is
        // accumulated. Do not send packets while we are fencing.
        if(fence_read())
                return;

        // Cause stats to be printed and record finish time
        if(phase == 2){
                get_packet_stat_tag_end(req_o, 0);
                phase ++;
        }

        if(wait_at_barrier(2, tg_dim))
                return;

        if(phase == 3){
                // Print the stats
                if(is_origin())
                        *req_v_o = get_packet_stat_kernel_end(req_o);
                phase ++;
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
        float *dmem_p = reinterpret_cast<float *>(this->dmem);
        float val = *reinterpret_cast<const float *>(&rsp_i->data);

        dmem_p[dmem_sz/sizeof(*dmem_p) - 1] = dmem_p[dmem_sz/sizeof(*dmem_p) - 1] / val;
}

