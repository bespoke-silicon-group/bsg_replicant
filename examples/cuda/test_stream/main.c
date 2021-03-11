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
#include <cl_manycore_regression.h>
#include <sys/stat.h>

#define ALLOC_NAME "default_allocator"
#define ARRAY_SIZE(x)                           \
        (sizeof(x)/sizeof(x[0]))

#define DIM(x,y)                                \
        (hb_mc_coordinate(x,y))

static int xmalloc(size_t sz, void **ptr)
{
        void *p = malloc(sz);
        if (p == NULL)
                return HB_MC_NOMEM;

        *ptr = p;
        return HB_MC_SUCCESS;
}

int test_dma (int argc, char **argv) {
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        hb_mc_device_t device;
        BSG_CUDA_CALL(hb_mc_device_init(&device, test_name, 0));

        /* make each array the size of cache */
        const hb_mc_config_t *cfg =  &device.mc->config;
        hb_mc_eva_t size
                = hb_mc_config_get_vcache_size(cfg)
                * hb_mc_config_get_num_dram_coordinates(cfg);

        bsg_pr_info("Streaming %uKB buffers\n", size);
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
                hb_mc_eva_t A_dev, B_dev, C_dev, D_dev;
                int N = size/sizeof(float);

                float *A_host, *B_host, *C_host, *D_host;
                BSG_CUDA_CALL(xmalloc(size, &A_host));
                BSG_CUDA_CALL(xmalloc(size, &B_host));
                BSG_CUDA_CALL(xmalloc(size, &C_host));
                BSG_CUDA_CALL(xmalloc(size, &D_host));

                for (int i = 0; i < N; i++) {
                        A_host[i] = 0*N + i;
                        B_host[i] = 1*N + i;
                        C_host[i] = 2*N + i;
                        D_host[i] = 3*N + i;
                }

                BSG_CUDA_CALL(hb_mc_device_malloc(&device, sizeof(float) * N, &A_dev));
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, sizeof(float) * N, &B_dev));
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, sizeof(float) * N, &C_dev));
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, sizeof(float) * N, &D_dev));

                hb_mc_dma_htod_t htod [] = {
                        {
                                .d_addr = A_dev,
                                .h_addr = &A_host[0],
                                .size   = sizeof(A_host)
                        },
                        {
                                .d_addr = B_dev,
                                .h_addr = &B_host[0],
                                .size   = sizeof(B_host)
                        },
                        {
                                .d_addr = C_dev,
                                .h_addr = &C_host[0],
                                .size   = sizeof(C_host)
                        },
                        {
                                .d_addr = D_dev,
                                .h_addr = &D_host[0],
                                .size   = sizeof(D_host)
                        }
                };

                BSG_CUDA_CALL(hb_mc_device_dma_to_device(&device, htod, 3));
                bsg_pr_info("Running read() kernel\n");
                // read kernel
                {
                        hb_mc_eva_t argv [] = { A_dev, N };
                        BSG_CUDA_CALL(hb_mc_kernel_enqueue(&device, DIM(1,1), DIM(cfg->pod_shape.x,cfg->pod_shape.y),
                                                           "cuda_stream_read", 2, argv));

                        BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));
                }
                bsg_pr_info("Running write() kernel\n");
                // write kernel
                {
                        hb_mc_eva_t argv [] = { B_dev, N };
                        BSG_CUDA_CALL(hb_mc_kernel_enqueue(&device, DIM(1,1), DIM(cfg->pod_shape.x,cfg->pod_shape.y),
                                                           "cuda_stream_write", 2, argv));
                        BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));
                }
                bsg_pr_info("Running copy() kernel\n");
                // copy kernel
                {
                        hb_mc_eva_t argv [] = { D_dev, C_dev, N };
                        BSG_CUDA_CALL(hb_mc_kernel_enqueue(&device, DIM(1,1), DIM(cfg->pod_shape.x,cfg->pod_shape.y),
                                                           "cuda_stream_copy", 3, argv));
                        BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));
                }
                BSG_CUDA_CALL(hb_mc_device_program_finish(&device));
        }

        BSG_CUDA_CALL(hb_mc_device_finish(&device));


        return HB_MC_SUCCESS;
}

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
        int main(int argc, char ** argv) {
#endif
                bsg_pr_test_info("Unified Main CUDA Regression Test\n");
                int rc = test_dma(argc, argv);
                bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
                return rc;
        }


