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
#include <libgen.h>
#include <bsg_manycore.h>
#include <bsg_manycore_cuda.h>
#include <bsg_manycore_tile.h>
#include <bsg_manycore_printing.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <bsg_manycore_loader.h>
#include <bsg_manycore_errno.h>
#include <bsg_manycore_printing.h>

#include <bsg_manycore_regression.h>


int test_manycore_atomic_packets(int argc, char *argv[]) {
        hb_mc_manycore_t manycore = {0}, *mc = &manycore;
        int err;

        /*****************************/
        /* Initializing the manycore */
        /*****************************/
        BSG_CUDA_CALL(hb_mc_manycore_init(mc, "test_manycore_atomic_packets", 0));

        /*************************/
        /* Seeding the test data */
        /*************************/
        srand(time(0));
       
        hb_mc_coordinate_t host = hb_mc_manycore_get_host_coordinate(mc);
        uint32_t host_x = hb_mc_coordinate_get_x(host);
        uint32_t host_y = hb_mc_coordinate_get_y(host);

        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        hb_mc_coordinate_t vcore_base = hb_mc_config_get_origin_vcore(cfg);
        uint8_t target_x = vcore_base.x;
        uint8_t target_y = vcore_base.y - 1;
        uint32_t addr = 0xbee0;
        uint32_t data  = 1;
        uint32_t load_id = 31;

        hb_mc_request_packet_t req;
        hb_mc_response_packet_t res;

        /**************************/
        /* Test packet formatting */
        /**************************/
        bsg_pr_test_info("Manually formatting packet\n");
        hb_mc_request_packet_set_x_dst(&req, target_x);
        hb_mc_request_packet_set_y_dst(&req, target_y);
        hb_mc_request_packet_set_x_src(&req, host_x);
        hb_mc_request_packet_set_y_src(&req, host_y);
        hb_mc_request_packet_set_data (&req, data);
        hb_mc_request_packet_set_load_id(&req, load_id);
        hb_mc_request_packet_set_op   (&req, HB_MC_PACKET_OP_REMOTE_AMOADD);
        hb_mc_request_packet_set_addr (&req, addr);

        /****************************************************/
        /* Test transmitting a manually formatted AMO packet */
        /****************************************************/
        uint32_t n = 8;
        bsg_pr_test_info("Sending %d manual amo packets to tile (%d, %d)\n", n, target_x, target_y);
        for (int i = 0; i < n; i++) {
            BSG_CUDA_CALL(hb_mc_manycore_packet_tx(mc, (hb_mc_packet_t*)&req, HB_MC_MMIO_FIFO_TO_DEVICE, -1));
        }
        
        bsg_pr_test_info(BSG_GREEN("Succesfully Wrote packet to FIFO (%s)") "\n",
                         hb_mc_direction_to_string(HB_MC_MMIO_FIFO_TO_DEVICE));

        bsg_pr_test_info("Reading %d amo responses from tile\n", n);
        for (int i = 0; i < n; i++) {
            BSG_CUDA_CALL(hb_mc_manycore_packet_rx(mc, (hb_mc_packet_t*)&res, HB_MC_MMIO_FIFO_TO_DEVICE, -1));
            data = hb_mc_response_packet_get_data(&res);
            bsg_pr_test_info("Data received: %d\n", data);
            if (data != i) {
                bsg_pr_test_err("Data mismatch: %d != %d", data, i);
                return HB_MC_FAIL;
            }
        }

        /****************************************************/
        /* Test transmitting an automatically formatted AMO packet */
        /****************************************************/
        hb_mc_coordinate_t dram_coord;
        dram_coord.x = target_x;
        dram_coord.y = target_y;
        hb_mc_npa_t npa = hb_mc_npa(dram_coord, addr<<2);

        bsg_pr_test_info("Sending %d auto amo packets\n", n);
        for (int i = 0; i < n; i++) {
            BSG_CUDA_CALL(hb_mc_manycore_amoadd(mc, &npa, 1, &data));
            bsg_pr_test_info("Data received: %d\n", data);
            if (data != n+i) {
                bsg_pr_test_err("Data mismatch: %d != %d", data, n+i);
                return HB_MC_FAIL;
            }
        }

        /*******/
        /* END */
        /*******/
        BSG_CUDA_CALL(hb_mc_manycore_exit(mc));
        return HB_MC_SUCCESS;   
}

declare_program_main(basename(__FILE__), test_manycore_atomic_packets);
