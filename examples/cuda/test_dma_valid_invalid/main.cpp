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


// This program ensures that the DMA invalidates cache lines
// correctly. It writes a data buffer using the packet-based memcpy
// method to validate cache lines. Then, it uses DMA to copy different
// data to the same buffer, bypassing cache. Finally, it copies the
// data buffer back using the packet-based memcpy method.

// This test ensures that the DMA correctly invalidates the cache
// lines that were previously validated by first memcpy -- if it does
// not, this test will fail. This failure could be in software (if the
// DMA does not correctly issue invalidates), or in hardware (if the
// cache doesn't apply invalidations correclty). The latter case was
// encountered after SW was added to manycore, but was missing in the
// tag ram case statement of the cache.

#include <bsg_manycore_tile.h>
#include <bsg_manycore_errno.h>
#include <bsg_manycore_cuda.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <bsg_manycore_regression.h>

#define ALLOC_NAME "default_allocator"
#define ARRAY_SIZE(x)                           \
    (sizeof(x)/sizeof(x[0]))

int test_dma (int argc, char **argv) {
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        // Define block_size_x/y: amount of work for each tile group
        // Define tg_dim_x/y: number of tiles in each tile group
        // Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
        // This is irrelevant for this test...
        hb_mc_dimension_t tg_dim = { .x = 2, .y = 2 };
        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1 };

        // Initialize device
        hb_mc_device_t device;
        BSG_CUDA_CALL(hb_mc_device_init(&device, test_name, args.device_id));

        hb_mc_pod_id_t pod;
        hb_mc_device_foreach_pod_id(&device, pod)
        {

                // We can load any program. We don't execute it in this test.
                BSG_CUDA_CALL(hb_mc_device_set_default_pod(&device, pod));
                BSG_CUDA_CALL(hb_mc_device_program_init(&device, bin_path, ALLOC_NAME, 0));

                hb_mc_eva_t A_dev, B_dev;
                int N = 32 * 2;
                int A_host[N], B_host[N];

                BSG_CUDA_CALL(hb_mc_device_malloc(&device, sizeof(uint32_t) * N, &A_dev));

                // Generate initial A
                // This will be overwritten by the DMA, but the cache shouldn't return this stale data.
                for (int i = 0; i < N; i++) A_host[i] = i;

                // Copy A to device, using Packets
                BSG_CUDA_CALL(hb_mc_device_pod_memcpy_to_device(&device, pod, A_dev, &A_host[0], sizeof(A_host)));

                // Generate new data for A
                for (int i = 0; i < N; i++) B_host[i] = A_host[i] = N-i;

                hb_mc_dma_htod_t htod = {
                        .d_addr = A_dev,
                        .h_addr = &A_host[0],
                        .size   = sizeof(A_host)
                };

                // Copy new A to device, using DMA, overwriting the
                // original data. This should ALSO invalidate the
                // cache.
                BSG_CUDA_CALL(hb_mc_device_dma_to_device(&device, &htod, 1));

                // Copy A back to the host, using packets. It should
                // get the DMA'd data, not the packet copied (stale)
                // data
                BSG_CUDA_CALL(hb_mc_device_pod_memcpy_to_host(&device, pod, &B_host[0], A_dev, sizeof(A_host)));

                // Check that the arrays are equal
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

                // Freeze the tiles and memory manager cleanup
                BSG_CUDA_CALL(hb_mc_device_program_finish(&device));
        }

        BSG_CUDA_CALL(hb_mc_device_finish(&device));


        return HB_MC_SUCCESS;
}

declare_program_main("DMA Valid/Invalid", test_dma);
