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
#include <bsg_manycore_regression.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_cuda.h>
#include <bsg_manycore_tile.h>
#include <inttypes.h>
#include <type_traits>

#define ADDR (HB_MC_TILE_EPA_DMEM_BASE >> 2)

#define CHECK(cond)                                        \
    do {                                                   \
        if (!(cond)) {                                     \
            bsg_pr_err("Condition '%s' failed\n", #cond);  \
            return HB_MC_FAIL;                             \
        }                                                  \
    } while (0)

/* 
   Check how the compiler handles packets.
   i.e. are all packet types the same size and aligned to a word boundary?
*/
int test_compile(int argc, char **argv)
{
    // check size
    static_assert(sizeof(hb_mc_request_packet_t) == sizeof(hb_mc_packet_t),  "Request packet size mismatch");
    static_assert(sizeof(hb_mc_response_packet_t) == sizeof(hb_mc_packet_t), "Response packet size mismatch");

    // check alignment
    hb_mc_request_packet_t rqst;
    hb_mc_response_packet_t rsp;
    uintptr_t rqst_addr = reinterpret_cast<uintptr_t>(&rqst);
    uintptr_t rsp_addr  = reinterpret_cast<uintptr_t>(&rsp);

    CHECK(rqst_addr % sizeof(uint32_t) == 0);
    CHECK(rsp_addr  % sizeof(uint32_t) == 0);
    return HB_MC_SUCCESS;
}
/*
  Transmit a write request.
  Then read it back with by transmitting a read request.
  Finally, receive a response packet for the read request.
 */
int test_wait (int argc, char **argv)
{
    int rc;
    struct arguments_none args = {};
    hb_mc_manycore_t mc = {0};
    rc = argp_parse (&argp_none, argc, argv, 0, 0, &args);
    if(rc != HB_MC_SUCCESS){
            return rc;
    }


    BSG_CUDA_CALL(hb_mc_manycore_init(&mc, "test_packet", args.device_id));

    hb_mc_request_packet_t write_rqst;

    write_rqst.x_dst = 16;
    write_rqst.y_dst = 7;

    write_rqst.x_src = 16;
    write_rqst.y_src = 0;

    write_rqst.op_v2 = HB_MC_PACKET_OP_REMOTE_STORE;
    write_rqst.addr  = ADDR;
    write_rqst.reg_id = 0xF;
    write_rqst.payload = 0xDEADBEEF;
    
    BSG_CUDA_CALL(hb_mc_manycore_request_tx(&mc, &write_rqst, -1));

    hb_mc_request_packet_t read_rqst;
    read_rqst.x_dst = 16;
    read_rqst.y_dst = 7;

    read_rqst.x_src = 16;
    read_rqst.y_src = 0;

    read_rqst.op_v2 = HB_MC_PACKET_OP_REMOTE_LOAD;
    read_rqst.addr = ADDR;
    read_rqst.reg_id = 0;
    read_rqst.payload = 0;

    hb_mc_request_packet_load_info_t info;
    info.part_sel       = 2;
    info.is_byte_op     = 1;
    info.is_unsigned_op = 1;

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

int test_packet(int argc, char **argv)
{
    BSG_CUDA_CALL(test_compile(argc, argv));
    BSG_CUDA_CALL(test_wait(argc, argv));
    return HB_MC_SUCCESS;
}

declare_program_main("test_packet", test_packet);
