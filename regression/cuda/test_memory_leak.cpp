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

#include "test_memory_leak.hpp"

#define ALLOC_NAME "default_allocator"

#define NUM_ITER 32
#define WIDTH (1 << 24)

/*!
 * Makes sure CUDA-Lite API does not have any memory leaks.
 * Runs an empty test multiple times while allocating large
 * chunks of memory before every kernel call and freeing them after.
 * If the CUDA API doesn't have memory leaks, the address of 
 * allocated spaces should match every time.  
*/

int kernel_memory_leak (int argc, char **argv) {
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Malloc Free Kernel on a 4x4 tile group.\n");

        srand(static_cast<unsigned>(time(0)));

        /* Define path to binary. */
        /* Initialize device, load binary and unfreeze tiles. */
        hb_mc_dimension_t tg_dim = { .x = 2, .y = 2};
        hb_mc_device_t device;
        BSG_CUDA_CALL(hb_mc_device_init_custom_dimensions(&device, test_name, 0, tg_dim));

        BSG_CUDA_CALL(hb_mc_device_program_init(&device, bin_path, ALLOC_NAME, 0));

        /* Allocate memory on the device for A, B and C. */
        constexpr size_t vsize = WIDTH * sizeof(uint32_t);

        bool mismatch = false;

        hb_mc_eva_t A_device, B_device, C_device;
        hb_mc_eva_t A_prev = A_device;
        hb_mc_eva_t B_prev = B_device;
        hb_mc_eva_t C_prev = C_device;

        for (int i = 0; i < NUM_ITER; i ++) {

            BSG_CUDA_CALL(hb_mc_device_malloc(&device, vsize, &A_device)); 
            BSG_CUDA_CALL(hb_mc_device_malloc(&device, vsize, &B_device)); 
            BSG_CUDA_CALL(hb_mc_device_malloc(&device, vsize, &C_device)); 

            if (i) {
                // If the address of current allocation doesn't match the address of 
                // previous allocation, we have a memory leak
                if ((A_device != A_prev) || (B_device != B_prev) || (C_device != C_prev)) {
                    bsg_pr_err("Address Mismatch -- you have a memory leak!\n");
                    mismatch = true;
                    break;
                }
            }

            bsg_pr_test_info("iter %d\tA: 0x%x\tB: 0x%x\tC: 0x%x\n", i, A_device, B_device, C_device);
    
            /* Define tg_dim_x/y: number of tiles in each tile group */
            /* Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y */
            hb_mc_dimension_t grid_dim = { .x = 1, .y = 1};
    
            /* Prepare list of input arguments for kernel. */
            uint32_t cuda_argv[3] = {A_device, B_device, C_device};
    
            /* Enqqueue grid of tile groups, pass in grid and tile group dimensions,
               kernel name, number and list of input arguments */
            BSG_CUDA_CALL(hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_memory_leak", 3, cuda_argv));
    
            /* Launch and execute all tile groups on device and wait for all to finish.  */
            BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));

            A_prev = A_device;
            B_prev = B_device;
            C_prev = C_device;
   
            BSG_CUDA_CALL(hb_mc_device_free(&device, A_device));
            BSG_CUDA_CALL(hb_mc_device_free(&device, B_device));
            BSG_CUDA_CALL(hb_mc_device_free(&device, C_device));
        }


        /* Freeze the tiles and memory manager cleanup.  */
        BSG_CUDA_CALL(hb_mc_device_finish(&device));

        return mismatch ? HB_MC_FAIL : HB_MC_SUCCESS;
}

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif
        bsg_pr_test_info("test_memory_leak Regression Test\n");
        int rc = kernel_memory_leak(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}

