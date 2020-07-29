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

#include "test_multiple_binary_load.h"


#define ALLOC_NAME "default_allocator"

/*!
 * Runs an empty kernel on a 2x2 tile group twice, with devince_finish and device_init in between
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/multiple_binary_load/ Manycore binary in the BSG Manycore github repository.  
*/

int kernel_multiple_binary_load (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Init Finish Kernel on a 2x2 tile group.\n\n");

        bsg_pr_test_info("PART 1 -- Kernel: empty -- Tile group: 2x2.\n");
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
        * Define grid_dim_x/y: total number of tile groups
        * Define tg_dim_x/y: number of tiles in each tile group
        * Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
        ******************************************************************************************************************/
        hb_mc_dimension_t grid_dim = {.x = 1, .y = 1};
        hb_mc_dimension_t tg_dim = {.x = 2, .y = 2};


        /*****************************************************************************************************************
        * Prepare list of input arguments for kernel.
        ******************************************************************************************************************/
        int cuda_argv[1];


        /*****************************************************************************************************************
        * Enquque grid of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
        ******************************************************************************************************************/
        rc = hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_empty", 0, cuda_argv);
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
        * Freeze the tiles and memory manager cleanup. 
        ******************************************************************************************************************/
        rc = hb_mc_device_finish(&device); /* freeze the tiles and memory manager cleanup */
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to de-initialize device.\n");
                return rc;
        }
        bsg_pr_test_info(BSG_GREEN("Part 1 Passed.\n"));



        bsg_pr_test_info("PART 2 -- Kernel: empty -- Tile group: 2x2.\n");
        /*****************************************************************************************************************
        * Define path to binary.
        * Initialize device, load binary and unfreeze tiles.
        ******************************************************************************************************************/
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
        * Define grid_dim_x/y: total number of tile groups
        * Define tg_dim_x/y: number of tiles in each tile group
        * Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
        ******************************************************************************************************************/
        grid_dim = hb_mc_dimension (1, 1);
        tg_dim = hb_mc_dimension (2, 2);


        /*****************************************************************************************************************
        * Prepare list of input arguments for kernel.
        ******************************************************************************************************************/


        /*****************************************************************************************************************
        * Enquque grid of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
        ******************************************************************************************************************/
        rc = hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_empty", 0, argv);
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
        * Freeze the tiles and memory manager cleanup. 
        ******************************************************************************************************************/
        rc = hb_mc_device_finish(&device); /* freeze the tiles and memory manager cleanup */
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to de-initialize device.\n");
                return rc;
        }
        bsg_pr_test_info(BSG_GREEN("Part 2 Passed.\n"));



        return HB_MC_SUCCESS;
}

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif
        bsg_pr_test_info("test_multiple_binary_load Regression Test \n");
        int rc = kernel_multiple_binary_load(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}


