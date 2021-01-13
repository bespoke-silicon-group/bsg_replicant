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
#include <cinttypes>
#include <type_traits>
#include <bsg_manycore_dpi_tile.hpp>
#include <bsg_manycore_packet.h>
#include <cl_manycore_regression.h>



#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif
        bsg_pr_test_info(__FILE__ " Regression Test \n");
        bool finish = false;
        hb_mc_packet_t pkt;
        int err = HB_MC_SUCCESS;
        hb_mc_manycore_t manycore = {0}, *mc = &manycore;
        hb_mc_coordinate_t origin, dim, target, max;
        hb_mc_npa_t npa;
        uint32_t write_data = 0, read_data = 0;
        hb_mc_coordinate_t pod = {.x = 0, .y = 0};

        BSG_MANYCORE_CALL(mc, hb_mc_manycore_init(mc, __FILE__, 0));

        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        origin = hb_mc_config_pod_vcore_origin(cfg, pod);
        dim = hb_mc_config_get_dimension_vcore(cfg);

        max.x = origin.x + dim.x - 1;
        max.y = origin.y + dim.y - 1;

        // Write metadata to DMEM on each tile
        bsg_pr_test_info("Writing metadata to DMEM of each tile\n");
        for(hb_mc_idx_t x_i = origin.x; x_i <= max.x; ++x_i){
                for(hb_mc_idx_t y_i = origin.y; y_i <= max.y; ++y_i){
                        npa.x = x_i;
                        npa.y = y_i;
                        target.x = x_i;
                        target.y = y_i;

                        // Tile group dimensions (for CUDA-Lite)
                        // normally reside in DMEM, and are set by the
                        // runtime. We don't have those conveniences
                        // here, so we allocate and set them manually in DRAM

                        // Tile Group X
                        write_data = dim.x;
                        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + 0;
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

                        // Tile Group Y
                        write_data = dim.y;
                        npa.epa = HB_MC_TILE_EPA_DMEM_BASE + 4;
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write_mem(mc, &npa, &write_data, sizeof(write_data)));

                        // Set the origin of the target tile
                        BSG_MANYCORE_CALL(mc, hb_mc_tile_set_origin(mc, &target, &origin));
                }
        }

        bsg_pr_test_info("Unfreezing target(s)...\n");
        for(hb_mc_idx_t x_i = origin.x; x_i <= max.x; ++x_i){
                for(hb_mc_idx_t y_i = origin.y; y_i <= max.y; ++y_i){
                        target.x = x_i;
                        target.y = y_i;
                        BSG_MANYCORE_CALL(mc, hb_mc_tile_unfreeze(mc, &target));
                }
        }

        // The target(s) will now barrier, and then send finish packets

        // Wait for finish from all tiles.
        bsg_pr_test_info("Waiting for finish packet...\n");
        for(hb_mc_idx_t x_i = origin.x; x_i <= max.x; ++x_i){
                for(hb_mc_idx_t y_i = origin.y; y_i <= max.y; ++y_i){
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_wait_finish(mc, -1));
                }
        }
        bsg_pr_info("Received finish packets\n");
        // Fence to make sure host credits return to their origin.
        BSG_MANYCORE_CALL(mc, hb_mc_manycore_host_request_fence(mc, -1));

        BSG_MANYCORE_CALL(mc, hb_mc_manycore_exit(mc));
        bsg_pr_test_pass_fail(err == HB_MC_SUCCESS);
        return err;
};


// This method executes requests to dmem, icache, and csr-space like
// any normal tile.
void BsgDpiTile::execute_request(const hb_mc_request_packet_t *req,
                                 hb_mc_response_packet_t *rsp){
        this->default_request_handler(req, rsp);
}


// This is the traffic generator method. This particular test sends a
// finish packet when dmem address 4 changes from 0 to 1 (The second
// DMEM write in the example above)
void BsgDpiTile::send_request(bool *req_v_o, hb_mc_request_packet_t *req_o){
        uint32_t *dmem_p = reinterpret_cast<uint32_t *>(this->dmem);
        hb_mc_coordinate_t tg_dim = {.x = dmem_p[0], .y = dmem_p[1]};

        // wait_at_barrier provides barrier functionality.
        // wait_at_barrier takes a barrier_id as an integer. The
        // origin tile will verify that all tiles have reached the
        // barrier with the same barrier_id before "unblocking" or
        // causing wait_at_barrier to return false. Subsequent calls
        // to wait_at_barrier with the same id will return false.
        //
        // barrier_id must increase by 1 between call sites.
        if(wait_at_barrier(0, tg_dim))
                return;

        if(wait_at_barrier(1, tg_dim))
                return;

        if(wait_at_barrier(2, tg_dim))
                return;

        *req_v_o = get_packet_finish(req_o);

        // Setting finished to true means this method will no longer
        // be called.
        finished = true;
}

void BsgDpiTile::receive_response(const hb_mc_response_packet_t *rsp_i){
        // Do nothing. Internally, BsgDpiTile will track and free IDs.
}

