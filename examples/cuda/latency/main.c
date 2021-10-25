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
#include <bsg_manycore.h>
#include <stdint.h>


#define ALLOC_NAME "default_allocator"

// Tests Cache/DRAM latency by striding through memory

int kernel_latency (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};
        uint64_t cycle_start, cycle_end;

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        srand(time);

        /*********************/
        /* Initialize device */
        /*********************/
        hb_mc_device_t device;
        BSG_CUDA_CALL(hb_mc_device_init(&device, test_name, 0));


        hb_mc_pod_id_t pod;
        hb_mc_device_foreach_pod_id(&device, pod)
        {
                
                // Initialize device, load binary and unfreeze tiles.
                bsg_pr_test_info("Loading program for %s onto pod %d\n",
                                 test_name, pod);

                BSG_CUDA_CALL(hb_mc_device_set_default_pod(&device, pod));
                hb_mc_program_options_t opts;
                hb_mc_program_options_default(&opts);
                opts.alloc_name = ALLOC_NAME;
                opts.alloc_id = 0;
                opts.program_name = bin_path;
                opts.mesh_dim.x = 1;
                opts.mesh_dim.y = 1;
                BSG_CUDA_CALL(hb_mc_device_pod_program_init_opts(&device, device.default_pod_id, bin_path, &opts));

                uint32_t N = 16384;
                eva_t _A;
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, N * sizeof(uint32_t), &_A));

                uint32_t A[N];
                for (int i = 0; i < N; i++) {
                        A[i] = rand() & 0xFFFF;
                }

                int stride = hb_mc_config_get_vcache_block_words(hb_mc_manycore_get_config(device.mc));
                hb_mc_dimension_t tg_dim = { .x = 1, .y = 1 };
                hb_mc_dimension_t grid_dim = { .x = 1, .y = 1};

                hb_mc_eva_t kernel_argv[3] = {_A, (hb_mc_eva_t)stride, (hb_mc_eva_t)N};

                hb_mc_dma_htod_t htod = {
                        .d_addr = _A,
                        .h_addr = A,
                        .size   = sizeof(A)
                };

                BSG_CUDA_CALL(hb_mc_device_dma_to_device(&device, &htod, 1));

                BSG_CUDA_CALL(hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_latency", sizeof(kernel_argv)/sizeof(*kernel_argv), kernel_argv));

                rc = hb_mc_manycore_get_cycle((device.mc), &cycle_start);
                if(rc != HB_MC_SUCCESS){
                        bsg_pr_test_err("Failed to read cycle counter: %s\n",
                                        hb_mc_strerror(rc));
                        return HB_MC_FAIL;
                }

                BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));

                rc = hb_mc_manycore_get_cycle((device.mc), &cycle_end);
                if(rc != HB_MC_SUCCESS){
                        bsg_pr_test_err("Failed to read cycle counter: %s\n",
                                        hb_mc_strerror(rc));
                        return HB_MC_FAIL;
                }

                int sum = 0;
                for (int i = 0; i < N; i += stride){
                        sum += A[i];
                }

                hb_mc_dma_dtoh_t dtoh = {
                        .d_addr = _A,
                        .h_addr = A,
                        .size   = sizeof(A)
                };

                BSG_CUDA_CALL(hb_mc_device_dma_to_host(&device, &dtoh, 1));

                if (A[0] != sum){
                        bsg_pr_err(BSG_RED("Error! Device and host sum mismatch. Device: %d, Host: %d"),
                                   A[0], sum);
                }

                /*****************************************************************************************************************
                 * Freeze the tiles and memory manager cleanup.
                 ******************************************************************************************************************/
                BSG_CUDA_CALL(hb_mc_device_program_finish(&device));
        }

        BSG_CUDA_CALL(hb_mc_device_finish(&device));

        return HB_MC_SUCCESS;
}

declare_program_main("test_latency", kernel_latency);
