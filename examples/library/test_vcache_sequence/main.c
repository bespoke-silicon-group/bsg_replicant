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
#include <bsg_manycore_config_pod.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <bsg_manycore_loader.h>
#include <bsg_manycore_errno.h>
#include <bsg_manycore_printing.h>

#include <bsg_manycore_regression.h>


#define ARRAY_LEN 4096
#define BASE_ADDR 0x0000

int test_vcache_sequence() {
        srand(time(0));
        hb_mc_manycore_t manycore = {0}, *mc = &manycore;
        int err, r = HB_MC_FAIL;

        err = hb_mc_manycore_init(mc, "test_vcache_sequence", HB_MC_DEVICE_ID);
        if (err != HB_MC_SUCCESS) {
                bsg_pr_err("%s: failed to init manycore: %s\n",
                           __func__, hb_mc_strerror(err));
                return err;
        }
        

        const hb_mc_config_t *config = hb_mc_manycore_get_config(mc);

        int mismatch = 0; 
        int dram_count = 0, dram_break_after = 1;
        hb_mc_coordinate_t pod;
        hb_mc_config_foreach_pod(pod, config)
        {
                hb_mc_coordinate_t dram;
                hb_mc_config_pod_foreach_dram(dram, pod, config)
                {
                        if (dram_count++ >= dram_break_after)
                                break;

                        uint32_t dram_coord_x, dram_coord_y;
                        dram_coord_x = hb_mc_coordinate_get_x(dram);
                        dram_coord_y = hb_mc_coordinate_get_y(dram);
                        bsg_pr_test_info("Testing DRAM bank (%d,%d).\n", dram_coord_x, dram_coord_y);
                        uint32_t A_host;
                        uint32_t A_device;

                        hb_mc_response_packet_t buf[1]; 
                        int mismatch = 0;

                        hb_mc_coordinate_t dram_coord = hb_mc_coordinate (dram_coord_x, dram_coord_y); 

                        for (int i = 0; i < ARRAY_LEN; i ++) { 
                                A_host = rand();
                                hb_mc_epa_t dram_epa = BASE_ADDR + i*sizeof(A_host);
                                hb_mc_npa_t dram_npa = hb_mc_npa (dram_coord, dram_epa); 

                                err = hb_mc_manycore_write32(mc, &dram_npa, A_host); 
                                if (err != HB_MC_SUCCESS) {
                                        bsg_pr_err("%s: failed to write A[%d] = %d to DRAM coord(%d,%d) addr 0x%08x.\n",
                                                   __func__, i, A_host, dram_coord_x, dram_coord_y, dram_epa);
                                        return err;
                                }


                                err = hb_mc_manycore_read32(mc, &dram_npa, &A_device); 
                                if ( err != HB_MC_SUCCESS) {
                                        bsg_pr_err("%s: failed to read A[%d] from DRAM coord (%d,%d) addr 0x%08x.\n",
                                                   __func__, i, dram_coord_x, dram_coord_y, dram_epa);
                                        return err;
                                }

                                if (A_host != A_device) {
                                        bsg_pr_err(BSG_RED("Mismatch: ") "A_host[%d] = %d   !=   A_device[%d] = %d -- EPA: 0x%08x.\n",
                                                   i, A_host, i, A_device, dram_epa);
                                        mismatch = 1;
                                }
                        }
                }
        }


        if (mismatch) 
                return HB_MC_FAIL;
        return HB_MC_SUCCESS;           
}

declare_program_main("test_vcache_sequence", test_vcache_sequence);
