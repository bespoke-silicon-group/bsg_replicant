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
#include <inttypes.h>
#include <cl_manycore_regression.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_cuda.h>
#include <bsg_manycore_tile.h>
#include <inttypes.h>

#define ADDR (HB_MC_TILE_EPA_DMEM_BASE >> 2)
int test_wait (int argc, char **argv) {
    hb_mc_manycore_t mc = {0};
    BSG_CUDA_CALL(hb_mc_manycore_init(&mc, "test_packet", 0));

    hb_mc_request_packet_t write_rqst = {
        .x_dst = 16,
        .y_dst = 7,

        .x_src = 16,
        .y_src = 0,

        .op_v2 = HB_MC_PACKET_OP_REMOTE_STORE,
        .addr  = ADDR,
        .reg_id = 0xF,
        .payload = 0xDEADBEEF,
    };
    
    BSG_CUDA_CALL(hb_mc_manycore_request_tx(&mc, &write_rqst, -1));

    hb_mc_request_packet_t read_rqst = {
        .x_dst = 16,
        .y_dst = 7,

        .x_src = 16,
        .y_src = 0,

        .op_v2 = HB_MC_PACKET_OP_REMOTE_LOAD,
        .addr = ADDR,
        .reg_id = 0,
        .payload = 0,
    };

    hb_mc_request_packet_load_info_t info = {
        .part_sel       = 2,
        .is_byte_op     = 1,
        .is_unsigned_op = 1,
    };

    hb_mc_request_packet_set_load_info(&read_rqst, info);

    BSG_CUDA_CALL(hb_mc_manycore_request_tx(&mc, &read_rqst, -1));

    hb_mc_response_packet_t read_rsp;

    BSG_CUDA_CALL(hb_mc_manycore_response_rx(&mc, &read_rsp, -1));

    char buffer [256];
    bsg_pr_info("Read this packet: %s\n",
                hb_mc_response_packet_to_string(&read_rsp, buffer, sizeof(buffer)));

    BSG_CUDA_CALL(hb_mc_manycore_exit(&mc));
    return HB_MC_SUCCESS;
}

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif

        bsg_pr_test_info("test_rom Regression Test \n");
        int rc = test_wait(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
