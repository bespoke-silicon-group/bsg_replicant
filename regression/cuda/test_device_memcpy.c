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
/* Runs the device_memcpy kernel a grid of 2x2 tile groups.                   */
/* Host allcoates space on DRAM and passes the pointer and size to tiles      */
/* Tiles fill the space by copying from array A to array B                    */
/* Host the compares the values with expected.                                */
/* Grid dimensions are prefixed at 1x1.                                       */
/* This tests uses the software/spmd/bsg_cuda_lite_runtime/device_memcpy/     */
/* manycore binary in the BSG Manycore repository.                            */
/******************************************************************************/


#include "test_device_memcpy.h"

#define ALLOC_NAME "default_allocator"


int kernel_device_memcpy (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Memcpy Kernel "
                         "on a grid of 2x2 tile groups.\n\n");

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
        /* Allocate memory on the device for A & B.                           */
        /**********************************************************************/
        uint32_t N = 64;

        hb_mc_eva_t A_device; 
        hb_mc_device_malloc(&device, N * sizeof(uint32_t), &A_device);

        void *dst = (void *) ((intptr_t) A_device);
        void *src = (void *) &A_host[0];
        
        hb_mc_device_memcpy (&device, dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE);
       
        hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_device_memcpy", 3, cuda_argv);
        
        hb_mc_device_tile_groups_execute(&device);        

        hb_mc_device_finish(&device);
        
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to de-initialize device.\n");
                return rc;
        }


        int mismatch = 0; 
        for (int i = 0; i < N; i++) {
                if (A_host[i] != B_host[i]) {
                        bsg_pr_err(BSG_RED("Mismatch: ") "B[%d] =  0x%08" PRIx32 "\t Expected: 0x%08" PRIx32 "\n",
                                                         i,
                                                         B_host[i],
                                                         A_host[i]);
                        mismatch = 1;
                }
        } 

        if (mismatch) { 
                return HB_MC_FAIL;
        }
        return HB_MC_SUCCESS;
}

#ifdef COSIM
void cosim_main(uint32_t *exit_code, char * args) {
        // We aren't passed command line arguments directly so we parse them
        // from *args. args is a string from VCS - to pass a string of arguments
        // to args, pass c_args to VCS as follows: +c_args="<space separated
        // list of args>"
        int argc = get_argc(args);
        char *argv[argc];
        get_argv(args, argc, argv);

#ifdef VCS
        svScope scope;
        scope = svGetScopeFromName("tb");
        svSetScope(scope);
#endif
        bsg_pr_test_info("test_device_memcpy Regression Test (COSIMULATION)\n");
        int rc = kernel_device_memcpy(argc, argv);
        *exit_code = rc;
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return;
}
#else
int main(int argc, char ** argv) {
        bsg_pr_test_info("test_device_memcpy Regression Test (F1)\n");
        int rc = kernel_device_memcpy(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
#endif

