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

#include "test_softmax.h"
#include <math.h>

#define ALLOC_NAME "default_allocator"

#define max(x, y) (((x) > (y)) ? (x) : (y))

void softmax(float *A, float *B, int M, int N)
{
        float m = -INFINITY;
        for(int i = 0; i < M * N; i++)
                m = max(A[i], m);

        for(int i = 0; i < M * N; i++)
                B[i] = expf(A[i] - m);

        float sum = 0;
        for(int i = 0; i < M * N; i++)
                sum += B[i];

        for(int i = 0; i < M * N; i++)
                B[i] /= sum;
}

int kernel_softmax(int argc, char **argv)
{       
        bsg_pr_test_info("Running CUDA Softmax Kernel on a 2x2 tile group.\n\n");
        char *elf, *test_name;
        struct arguments_path args = { NULL, NULL };
        argp_parse(&argp_path, argc, argv, 0, 0, &args);
        elf = args.path;
        test_name = args.name;

        srand(time(0));
        int rc;
        hb_mc_device_t manycore, *mc = &manycore;
        rc = hb_mc_device_init(mc, test_name, 0);
        if(rc != HB_MC_SUCCESS)
        {
                bsg_pr_err("Failed to initialize device.\n");
                return rc;
        }

        rc = hb_mc_device_program_init(mc, elf, ALLOC_NAME, 0);
        if(rc != HB_MC_SUCCESS)
        {
                bsg_pr_err("Failed to initialize the program.\n");
                return rc;
        }

        int M = 32;
        int N = 64;
        eva_t A_device, B_device;
        rc = hb_mc_device_malloc(mc, M * N * sizeof(float), &A_device);
        if(rc != HB_MC_SUCCESS)
        {
                bsg_pr_err("Failed to allocate A on the manycore.\n");
                return rc;
        }

        rc = hb_mc_device_malloc(mc, M * N * sizeof(float), &B_device);
        if(rc != HB_MC_SUCCESS)
        {
                bsg_pr_err("Failed to allocate B on the manycore.\n");
                return rc;
        }

        float A_host[M * N];
        for(int i = 0; i < M * N; i++)
        {
                A_host[i] = hb_mc_generate_float_rand();
                bsg_pr_test_info("A_host[%d] = %.9f \n",
                                 i, A_host[i]);
        }

        rc = hb_mc_device_memcpy(mc, A_device, A_host, M * N * sizeof(float), HB_MC_MEMCPY_TO_DEVICE);
        if(rc != HB_MC_SUCCESS)
        {
                bsg_pr_err("Failed to memcpy A to the manycore.\n");
                return rc;
        }

        uint32_t block_size_x = N;
        uint32_t block_size_y = M;

        hb_mc_dimension_t tilegroup_dim = { .x = 2, .y = 2 };
        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1 };

        int cuda_argv[] = { A_device, B_device, M, N, block_size_y, block_size_x };
        size_t cuda_argc = sizeof(cuda_argv) / sizeof(cuda_argv[0]);
        rc = hb_mc_kernel_enqueue(mc, grid_dim, tilegroup_dim, "kernel_softmax", cuda_argc, cuda_argv);
        if(rc != HB_MC_SUCCESS)
        {
                bsg_pr_err("Failed to initialize grid.\n");
                return rc;
        }

        rc = hb_mc_device_tile_groups_execute(mc);
        if(rc != HB_MC_SUCCESS)
        {
                bsg_pr_err("Failed to execute tilegroups.\n");
                return rc;
        }

        float B_actual[M * N];
        rc = hb_mc_device_memcpy(mc, B_actual, B_device, M * N * sizeof(float), HB_MC_MEMCPY_TO_HOST);
        if(rc != HB_MC_SUCCESS)
        {
                bsg_pr_err("Failed to mempcy result to host.\n");
                return rc;
        }

        rc = hb_mc_device_finish(mc);
        if(rc != HB_MC_SUCCESS)
        {
                bsg_pr_err("Failed to deinitialize the manycore.\n");
                return rc;
        }

        float B_expected[M * N];
        softmax(A_host, B_expected, M, N);

        int mismatches = 0;
        for(int y = 0; y < M; y++)
                for(int x = 0; x < N; x++)
                {
                        if(hb_mc_calculate_float_error(B_actual[y * N + x], B_expected[y * N + x]) > MAX_FLOAT_ERROR_TOLERANCE ||
                           (!isnormal(B_actual[y * N + x]) && B_actual[y * N + x] != 0.0f))
                        {
                                bsg_pr_err(BSG_RED("Mismatch: ") "B[%d][%d] = %.9f = %x \t Expected: %.9f = %x\n",
                                           y, x,
                                           B_actual[y * N + x],
                                           B_actual[y * N + x],
                                           B_expected[y * N + x],
                                           B_expected[y * N + x]);
                                mismatches++;
                        }
                }
        if(!mismatches)
        {
                bsg_pr_test_info(BSG_GREEN("Matrices match!\n"));
                return HB_MC_SUCCESS;
        }
        bsg_pr_test_err(BSG_RED("Matrices don't match!\n"));
        return HB_MC_FAIL;
}

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif
        bsg_pr_test_info("test_softmax Regression Test\n");
        int rc = kernel_softmax(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}


