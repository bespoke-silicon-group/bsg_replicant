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
#include <sys/stat.h>

#define ALLOC_NAME "default_allocator"
#define ARRAY_SIZE(x)                           \
    (sizeof(x)/sizeof(x[0]))

int test_dma (int argc, char **argv) {
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Unified Main %s "
                         "on a grid of 2x2 tile groups\n\n", test_name);

        srand(time);

        /**********************************************************************/
        /* Define block_size_x/y: amount of work for each tile group          */
        /* Define tg_dim_x/y: number of tiles in each tile group              */
        /* Calculate grid_dim_x/y: number of                                  */
        /* tile groups needed based on block_size_x/y                         */
        /**********************************************************************/
        hb_mc_dimension_t tg_dim = { .x = 1, .y = 1 };
        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1 };

        /**********************************************************************/
        /* Define path to binary.                                             */
        /* Initialize device, load binary and unfreeze tiles.                 */
        /**********************************************************************/
        hb_mc_device_t device;
        BSG_CUDA_CALL(hb_mc_device_init_custom_dimensions(&device, test_name, 0, tg_dim));
        const hb_mc_config_t *cfg = &device.mc->config;
        hb_mc_eva_t alignment
            = cfg->pod_shape.x
            * 2
            * cfg->vcache_block_words
            * 4;

        hb_mc_pod_id_t pod;
        hb_mc_device_foreach_pod_id(&device, pod)
        {

                bsg_pr_test_info("loading program for %s onto pod %d\n",
                                 test_name, pod);

                BSG_CUDA_CALL(hb_mc_device_set_default_pod(&device, pod));
                BSG_CUDA_CALL(hb_mc_device_program_init(&device, bin_path, ALLOC_NAME, 0));

                /**********/
                /* Read A */
                /**********/
                hb_mc_eva_t A_dev, B_dev;
                int N = alignment/4; // array should span all caches
                int A_host[N], B_host[N];

                for (int i = 0; i < N; i++) A_host[i] = i;

                BSG_CUDA_CALL(hb_mc_device_malloc(&device, sizeof(uint32_t) * N + alignment, &A_dev));
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, sizeof(uint32_t) * N + alignment, &B_dev));

                hb_mc_eva_t rem;
                // align A_dev + B_dev to cache 0
                rem = A_dev % alignment;
                A_dev += (alignment - rem);

                rem = B_dev % alignment;
                B_dev += (alignment - rem);

                hb_mc_dma_htod_t htod = {
                        .d_addr = A_dev,
                        .h_addr = &A_host[0],
                        .size   = sizeof(A_host)
                };

                BSG_CUDA_CALL(hb_mc_device_dma_to_device(&device, &htod, 1));

                /**********************************************************************/
                /* Prepare list of input arguments for kernel.                        */
                /**********************************************************************/
                hb_mc_eva_t kernel_argv[] = {A_dev, B_dev, (hb_mc_eva_t)N};

                char kernel_name [] = "kernel_dma";

                /**********************************************************************/
                /* Enquque grid of tile groups, pass in grid and tile group dimensions*/
                /* kernel name, number and list of input arguments                    */
                /**********************************************************************/
                BSG_CUDA_CALL(hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, kernel_name,
                                                    ARRAY_SIZE(kernel_argv), kernel_argv));

                /**********************************************************************/
                /* Launch and execute all tile groups on device and wait for finish.  */
                /**********************************************************************/
                BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));

                /**********/
                /* Read B */
                /**********/
                hb_mc_dma_dtoh_t dtoh = {
                        .d_addr = B_dev,
                        .h_addr = &B_host[0],
                        .size   = sizeof(B_host)
                };

                BSG_CUDA_CALL(hb_mc_device_dma_to_host(&device, &dtoh, 1));


                /***********************************/
                /* Check that the arrays are equal */
                /***********************************/
                int rc = HB_MC_SUCCESS;
                for (int i = 0; i < N; i++) {
                        if (A_host[i] != B_host[i]) {
                                bsg_pr_err("%s: Mismatch: B_host[%d] = %d, Expected %d\n",
                                           __func__, i, B_host[i], A_host[i]);
                                rc = HB_MC_FAIL;
                        }
                }

                if (rc != HB_MC_SUCCESS) {
                        BSG_CUDA_CALL(hb_mc_device_finish(&device));
                        return rc;
                }

                /**********************************************************************/
                /* Freeze the tiles and memory manager cleanup.                       */
                /**********************************************************************/
                BSG_CUDA_CALL(hb_mc_device_program_finish(&device));
        }

        BSG_CUDA_CALL(hb_mc_device_finish(&device));


        return HB_MC_SUCCESS;
}

declare_program_main("DMA", test_dma);
