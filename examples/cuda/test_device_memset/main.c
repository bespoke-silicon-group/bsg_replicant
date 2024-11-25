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
/* Runs the device_memset kernel a grid of 2x2 tile groups.                   */
/* Host allcoates space on DRAM and passes the pointer and size to tiles      */
/* Tiles fill the space with the requested value from host                    */
/* Host the compares the values with expected.                                */
/* Grid dimensions are prefixed at 1x1.                                       */
/* This tests uses the software/spmd/bsg_cuda_lite_runtime/device_memset/     */
/* manycore binary in the BSG Manycore repository.                            */
/******************************************************************************/


#include <bsg_manycore_tile.h>
#include <bsg_manycore_errno.h>
#include <bsg_manycore_tile.h>
#include <bsg_manycore_loader.h>
#include <bsg_manycore_cuda.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <bsg_manycore_regression.h>

#define ALLOC_NAME "default_allocator"
#define TEST_VALUE 0x1234


int kernel_device_memset (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Memset Kernel "
                         "on a grid of 2x2 tile groups.\n\n");

        srand(time); 



        /**********************************************************************/
        /* Define path to binary.                                             */
        /* Initialize device, load binary and unfreeze tiles.                 */
        /**********************************************************************/
        hb_mc_device_t device;
        rc = hb_mc_device_init(&device, test_name, HB_MC_DEVICE_ID);
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
        /* Allocate memory on the device for A_ptr.                           */
        /**********************************************************************/
        uint32_t N = 64;

        hb_mc_eva_t A_device; 
        rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &A_device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }

        // Set the entire space to zero
        rc = hb_mc_device_memset(&device, &A_device, 0, N * sizeof(uint8_t));
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to set memory on device.\n");
                return rc;
        } 


        /**********************************************************************/
        /* Define block_size_x/y: amount of work for each tile group          */
        /* Define tg_dim_x/y: number of tiles in each tile group              */
        /* Calculate grid_dim_x/y: number of                                  */
        /* tile groups needed based on block_size_x/y                         */
        /**********************************************************************/
        hb_mc_dimension_t tg_dim = { .x = 2, .y = 2 }; 

        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1 };


        /**********************************************************************/
        /* Prepare list of input arguments for kernel.                        */
        /**********************************************************************/
        int cuda_argv[3] = {A_device, TEST_VALUE , N};


        /**********************************************************************/
        /* Enquque grid of tile groups, pass in grid and tile group dimensions*/
        /* kernel name, number and list of input arguments                    */
        /**********************************************************************/
        rc = hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_device_memset", 3, cuda_argv);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to initialize grid.\n");
                return rc;
        }



        /**********************************************************************/
        /* Launch and execute all tile groups on device and wait for finish.  */ 
        /**********************************************************************/
        rc = hb_mc_device_tile_groups_execute(&device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to execute tile groups.\n");
                return rc;
        }       


        /**********************************************************************/
        /* Copy result vector back from device DRAM into host memory.         */
        /**********************************************************************/
        uint32_t A_host[N];
        void *src = (void *) ((intptr_t) A_device);
        void *dst = (void *) &A_host;
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


        int mismatch = 0; 
        for (int i = 0; i < N; i++) {
                if (A_host[i] != TEST_VALUE) {
                        bsg_pr_err(BSG_RED("Mismatch: ") "A[%d] =  0x%08" PRIx32 "\t Expected: 0x%08" PRIx32 "\n",
                                                         i,
                                                         A_host[i],
                                                         TEST_VALUE);
                        mismatch = 1;
                }
        } 

        if (mismatch) { 
                return HB_MC_FAIL;
        }
        return HB_MC_SUCCESS;
}

declare_program_main("test_device_memset", kernel_device_memset);
