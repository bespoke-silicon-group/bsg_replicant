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

#include "test_stack_load.h"

#define ALLOC_NAME "default_allocator"

/*!
 * Runs the stack_load test on a grid of one tile group
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/stack_load/ Manycore binary in the BSG Manycore bitbucket repository.  
*/


#define NUM_ARGS        15              // Number of arguments passed to kernel
#define SUM_ARGS        120             // Sum of arguments passed to kernel


int kernel_stack_load (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Stack Load Kernel on a 1x1 grid of 2x2 tile group.\n\n");

        /*****************************************************************************************************************
        * Define path to binary.
        * Initialize device, load binary and unfreeze tiles.
        ******************************************************************************************************************/
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


        /*****************************************************************************************************************
        * Define tg_dim_x/y: number of tiles in each tile group
        * Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
        ******************************************************************************************************************/
        hb_mc_dimension_t tg_dim = { .x = 2, .y = 2 }; 
        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1 }; 


        /*****************************************************************************************************************
        * Allocate memory on the device for sum of input arguments, one element for each tile.
        ******************************************************************************************************************/
        eva_t sum_device;
        rc = hb_mc_device_malloc(&device, tg_dim.x * tg_dim.y * sizeof (uint32_t), &sum_device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        /*****************************************************************************************************************
        * Prepare list of input arguments for kernel. {Num of arguments, Sum of arguments, arguments}
        ******************************************************************************************************************/
        int cuda_argv[NUM_ARGS + 2] = {sum_device, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};


        /*****************************************************************************************************************
        * Enquque grid of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
        ******************************************************************************************************************/
        rc = hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_stack_load", NUM_ARGS + 1, cuda_argv);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to initialize grid.\n");
                return rc;
        }



        /*****************************************************************************************************************
        * Launch and execute all tile groups on device and wait for all to finish. 
        ******************************************************************************************************************/
        rc = hb_mc_device_tile_groups_execute(&device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to execute tile groups.\n");
                return rc;
        }


        /*****************************************************************************************************************
        * Copy result sum back from device DRAM into host memory. 
        ******************************************************************************************************************/
        uint32_t sum_host[tg_dim.x * tg_dim.y];
        void *src = (void *) ((intptr_t) sum_device);
        void *dst = (void *) &sum_host[0];
        rc = hb_mc_device_memcpy (&device, (void *) dst, src, (tg_dim.x * tg_dim.y) * sizeof(uint32_t), HB_MC_MEMCPY_TO_HOST); /* copy sum_device to the host */
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory from device.\n");
                return rc;
        }



        /*****************************************************************************************************************
        * Freeze the tiles and memory manager cleanup. 
        ******************************************************************************************************************/
        rc = hb_mc_device_finish(&device); 
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to de-initialize device.\n");
                return rc;
        }


        /*****************************************************************************************************************
        * Compare the expected sum and the manycore sum. 
        ******************************************************************************************************************/
        int mismatch = 0;
        for (int y = 0; y < tg_dim.y; y ++) { 
                for (int x = 0; x < tg_dim.x; x ++) { 
                        if (sum_host[y * tg_dim.x + x] != SUM_ARGS) { 
                                bsg_pr_err(BSG_RED("Mismatch: ") "sum[%d][%d] = %d\tExpected %d.\n", y, x, sum_host[y * tg_dim.x + x], SUM_ARGS);
                                mismatch = 1; 
                        }
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
        bsg_pr_test_info("test_stack_load Regression Test \n");
        int rc = kernel_stack_load(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}


