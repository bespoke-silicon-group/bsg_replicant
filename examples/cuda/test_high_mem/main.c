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

#define ALLOC_NAME "default_allocator"

int kernel_vec_add (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Vector Addition Kernel on one 2x2 tile groups.\n\n");

        srand(time);

        hb_mc_device_t device;
        BSG_CUDA_CALL(hb_mc_device_init(&device, test_name, DEVICE_ID));

        int r = HB_MC_SUCCESS;
        hb_mc_pod_id_t pod;
        hb_mc_device_foreach_pod_id(&device, pod)
        {
                const hb_mc_config_t * cfg = hb_mc_manycore_get_config(device.mc);
                unsigned block_sz = hb_mc_config_get_vcache_block_size(cfg);

                hb_mc_program_options_t opts;
                hb_mc_program_options_default(&opts);
                opts.mesh_dim = hb_mc_dimension(1,1);
                opts.program_name = "high-mem";

                BSG_CUDA_CALL(hb_mc_device_pod_program_init_opts(&device, pod, bin_path, &opts));

                hb_mc_eva_t bitidx_lo = 8, bitidx_hi = 29;
                hb_mc_eva_t A_dev [bitidx_hi];
                unsigned   *A_host[bitidx_hi];
                hb_mc_dma_htod_t htod [bitidx_hi];
                int A_words = 32;
                size_t A_sz = sizeof(int) * A_words;

                hb_mc_eva_t B_dev;
                BSG_CUDA_CALL(hb_mc_device_pod_malloc(&device, pod, A_sz, &B_dev));
                bsg_pr_test_info("B_dev = 0x%08x\n", B_dev);
                for (hb_mc_eva_t bitidx = bitidx_lo; bitidx < bitidx_hi; ++bitidx) {
                    // construct address (add 1M offset)
                    A_dev[bitidx] = (B_dev + 0x01000000) | (1<<bitidx);

                    // set a host buffer to it
                    unsigned *tmp = malloc(A_sz);
                    for (int i = 0; i < A_words; ++i)
                        tmp[i] = A_dev[bitidx] + i * sizeof(unsigned);
                    A_host[bitidx] = tmp;

                    // setup a dma job
                    htod[bitidx].d_addr = A_dev[bitidx];
                    htod[bitidx].h_addr = A_host[bitidx];
                    htod[bitidx].size   = A_sz;
                }

                BSG_CUDA_CALL(hb_mc_device_pod_dma_to_device(&device, pod,
                                                             &htod[bitidx_lo],
                                                             bitidx_hi-bitidx_lo));

                for (hb_mc_eva_t bitidx = bitidx_lo; bitidx < bitidx_hi; ++bitidx) {
                    // launch kernel
                    unsigned argv [] = {A_dev[bitidx], B_dev, A_words};
                    BSG_CUDA_CALL(hb_mc_device_pod_kernel_enqueue(&device, pod,
                                                                  hb_mc_dimension(1,1),
                                                                  hb_mc_dimension(1,1),
                                                                  "kernel_high_mem", 3, argv));

                    bsg_pr_test_info("A_dev = 0x%08x\n", A_dev[bitidx]);
                    BSG_CUDA_CALL(hb_mc_device_pod_kernels_execute(&device, pod));

                    unsigned B_host[A_words];
                    BSG_CUDA_CALL(hb_mc_device_pod_memcpy_to_host(&device, pod,
                                                                  B_host, B_dev, A_sz));

                    for (int i = 0; i < A_words; i++) {
                            if (B_host[i] != A_dev[bitidx]+ i*sizeof(unsigned)) {
                                    bsg_pr_err("Mismatching address at 0x%08x : found 0x%08x\n",
                                               A_dev[bitidx]+i*sizeof(unsigned), B_host[i]);
                                    r = HB_MC_FAIL;
                            }
                    }
                }

                BSG_CUDA_CALL(hb_mc_device_pod_program_finish(&device, pod));

        }
        BSG_CUDA_CALL(hb_mc_device_finish(&device));

        return r;
}

declare_program_main("test_vec_add", kernel_vec_add);
