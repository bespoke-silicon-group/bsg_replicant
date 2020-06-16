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

/******************************************************************************/
/* Dram load store test.                                                      */
/* Does not execute any kernels on the tile group                             */
/* Simply stores a large array into DRAM and loads it back to compare.        */
/* This tests uses the software/spmd/bsg_cuda_lite_runtime/dram_load_store/   */
/* manycore binary in the BSG Manycore repository.                            */
/******************************************************************************/


#include "test_dram_load_store.h"

#define ALLOC_NAME "default_allocator"


int kernel_dram_load_store(int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA DRAM Load Store test.\n\n");

        srand(time); 


        /**********************************************************************/
        /* Define path to binary.                                             */
        /* Initialize device, load binary and unfreeze tiles.                 */
        /**********************************************************************/
        hb_mc_device_t device;
        rc = hb_mc_device_init(&device, test_name, 0);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to initialize device.\n");
                return rc;
        }

        rc = hb_mc_device_program_init(&device, bin_path, ALLOC_NAME, 0);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to initialize program.\n");
                return rc;
        }


        /**********************************************************************/
        /* Allocate memory on the device for A.                               */
        /**********************************************************************/
        uint32_t N = 4096;

        eva_t A_device; 
        rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &A_device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        /**********************************************************************/
        /* Allocate memory on the host for A                                  */
        /* Initialize A_host_in with random values.                           */
        /* Set all elements in A_host_out to zero.                            */
        /**********************************************************************/
        uint32_t A_host_in[N]; 
        uint32_t A_host_out[N];
        for (int i = 0; i < N; i++) { 
                A_host_in[i] = rand() & 0xFFFF;
                A_host_out[i] = 0;
        }


        /**********************************************************************/
        /* Copy A from host onto device DRAM.                                 */
        /**********************************************************************/
        void *dst = (void *) ((intptr_t) A_device);
        void *src = (void *) &A_host_in[0];
        rc = hb_mc_device_memcpy (&device, dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE);     
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory to device.\n");
                return rc;
        }


        /**********************************************************************/
        /* Copy A from device back into host.                                 */
        /**********************************************************************/
        src = (void *) ((intptr_t) A_device);
        dst = (void *) &A_host_out[0];
        rc = hb_mc_device_memcpy (&device, (void *) dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_HOST);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory from device.\n");
                return rc;
        }


        /**********************************************************************/
        /* Freeze the tiles and memory manager cleanup.                       */
        /**********************************************************************/
        rc = hb_mc_device_finish(&device); 
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to de-initialize device.\n");
                return rc;
        }


        /**********************************************************************/
        /* Calculate the expected result using host code and compare.         */ 
        /**********************************************************************/
        int mismatch = 0; 
        for (int i = 0; i < N; i++) {
                if (A_host_in[i] != A_host_out[i]) {
                        bsg_pr_err(BSG_RED("Mismatch: ") "A[%d] =  0x%08" PRIx32 "\t Expected: 0x%08" PRIx32 "\n",
                                                         i,
                                                         A_host_in[i],
                                                         A_host_out[i]);
                        mismatch = 1;
                }
        } 

        if (mismatch) { 
                return HB_MC_FAIL;
        }
        return HB_MC_SUCCESS;
}

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif
        bsg_pr_test_info("test_dram_load_store Regression Test \n");
        int rc = kernel_dram_load_store(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}


