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

//#define N 64
//#define K 4


int kernel_multikernel(int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Multi Kernel "
                         "on a grid of 1x1 tile groups.\n\n");

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
        uint32_t N = 8;
        uint32_t K = 4;

        hb_mc_eva_t buffer_device;
        BSG_CUDA_CALL(hb_mc_device_malloc(&device, K * N * sizeof(uint32_t), &buffer_device));
        hb_mc_eva_t sync_device;
        BSG_CUDA_CALL(hb_mc_device_malloc(&device, sizeof(uint32_t), &sync_device));

        /**********************************************************************/
        /* Allocate A & B on the host.                                        */
        /* Fill A with random values and set B to -1 on the host.             */
        /* Copy A from host to device DRAM and set B to 0 on device DRAM.     */
        /**********************************************************************/
        uint32_t buffer_host[K*N];
        for (int j = 0; j < K; j++) {
            for (int i = 0; i < N; i++) { /* fill A with increasing data */
                buffer_host[j*N+i] = i;
            }
        }

        for (int j = 0; j < K; j++) {
            for (int i = 0; i < N; i++) {
                bsg_pr_info("Host Buffer Init[%d]: %d\n", j*N+i, buffer_host[j*N+i]);
            } 
        }

        // Copy buffer from host to device
        void *dst, *src;
        dst = (void *) ((intptr_t) buffer_device);
        src = (void *) &buffer_host[0];
        BSG_CUDA_CALL(hb_mc_device_memcpy(&device, dst, src, K * N * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE));

        // Initialize sync variable to 0
        BSG_CUDA_CALL(hb_mc_device_memset(&device, &sync_device, 0, sizeof(uint32_t)));

        /**********************************************************************/
        /* Define block_size_x/y: amount of work for each tile group          */
        /* Define tg_dim_x/y: number of tiles in each tile group              */
        /* Calculate grid_dim_x/y: number of                                  */
        /* tile groups needed based on block_size_x/y                         */
        /**********************************************************************/
        hb_mc_dimension_t tg_dim = { .x = 1, .y = 1 }; 

        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1 };


        /**********************************************************************/
        /* Prepare list of input arguments for kernel.                        */
        /**********************************************************************/
        uint32_t cuda_argc = 4;
        int cuda_argv[cuda_argc];
        cuda_argv[0] = buffer_device;
        cuda_argv[1] = N;
        cuda_argv[2] = sync_device;
        cuda_argv[3] = K;


        /**********************************************************************/
        /* Enqueue a variable number of kernels. Ensure k <= the kernels      */
        /* compiled.                                                          */
        /**********************************************************************/
        char kernel_name[12];
        for (int i = 0; i < K; i++) {
          sprintf(kernel_name, "kernel%d", i);
          BSG_CUDA_CALL(hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, kernel_name, cuda_argc, cuda_argv));
        }

        /**********************************************************************/
        /* Launch and execute all tile groups on device and wait for finish.  */ 
        /**********************************************************************/
        BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));


        /**********************************************************************/
        /* Copy result vector back from device DRAM into host memory.         */
        /**********************************************************************/
        src = (void *) ((intptr_t) buffer_device);
        dst = (void *) &buffer_host;
        BSG_CUDA_CALL(hb_mc_device_memcpy (&device, (void *) dst, src, K * N * sizeof(uint32_t), HB_MC_MEMCPY_TO_HOST));


        /**********************************************************************/
        /* Freeze the tiles and memory manager cleanup.                       */
        /**********************************************************************/
        BSG_CUDA_CALL(hb_mc_device_finish(&device)); 

        int mismatch = 0;
        for (int j = 0; j < K; j++) {
            for (int i = 0; i < N; i++) {
                bsg_pr_info("Host Buffer Final[%d]: %d\n", j*N+i, buffer_host[j*N+i]);
                if (buffer_host[j*N+i] != (j+1)*100+i) {
                    mismatch = 1;
                }
            } 
        }

        return mismatch ? HB_MC_FAIL : HB_MC_SUCCESS;
}

declare_program_main("test_multikernel", kernel_multikernel);
