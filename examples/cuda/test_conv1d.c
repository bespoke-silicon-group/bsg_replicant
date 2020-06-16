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

#include "test_conv1d.h"
#include <math.h>

#define ALLOC_NAME "default_allocator"

#define MIN_N 4
#define MAX_N 1024

#define MIN_F 1
#define MAX_F 128

#define MIN_P 0
#define MAX_P 8

#define MIN_S 1
#define MAX_S 8

#define MAX_VALUE_MAGNITUDE 1e6

int generate_inclusive(int min, int max)
{
        int range = (max - min + 1);
        return (rand() % range) + min;
}

int compute_M(int N, int F, int P, int S)
{
        return 1 + (N - F + 2 * P) / S;
}

void conv1d(const float *A,
            const int N,
            const float *filter,
            const int F,
            const int P,
            float *B,
            const int S)
{
        int M = compute_M(N, F, P, S);
        for(int i = 0; i < M; i++)
        {
                int window_idx = i * S;
                float res = 0;
                for(int j = 0; j < F; j++)
                {
                        int a_idx = window_idx - P + j;
                        float a = 0;
                        if(0 <= a_idx && a_idx < N)
                                a = A[a_idx];

                        res += filter[j] * a;
                }
                B[i] = res;
        }
}

int kernel_conv1d(int argc, char **argv)
{       
        bsg_pr_test_info("Running CUDA Conv1D Kernel on a 2x2 tile group.\n\n");
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

        int N = generate_inclusive(MIN_N, MAX_N);
        int F = generate_inclusive(MIN_F, MAX_F < N ? MAX_F : N);
        int P = generate_inclusive(MIN_P, MAX_P);
        int S = generate_inclusive(MIN_S, MAX_S);
        int M = compute_M(N, F, P, S);
        
        size_t A_size      = sizeof(float) * N;
        size_t filter_size = sizeof(float) * F;
        size_t B_size      = sizeof(float) * M;
        
        eva_t A_device, B_device, filter_device;
        rc = hb_mc_device_malloc(mc, A_size, &A_device);
        if(rc != HB_MC_SUCCESS)
        {
                bsg_pr_err("Failed to allocate A on the manycore.\n");
                return rc;
        }

        rc = hb_mc_device_malloc(mc, filter_size, &filter_device);
        if(rc != HB_MC_SUCCESS)
        {
                bsg_pr_err("Failed to allocate B on the manycore.\n");
                return rc;
        }
        
        rc = hb_mc_device_malloc(mc, B_size, &B_device);
        if(rc != HB_MC_SUCCESS)
        {
                bsg_pr_err("Failed to allocate B on the manycore.\n");
                return rc;
        }

        float A_host[N];
        for(int i = 0; i < sizeof(A_host) / sizeof(A_host[0]); i++)
        {
                A_host[i] = fmod(hb_mc_generate_float_rand(), MAX_VALUE_MAGNITUDE);
                bsg_pr_test_info("A_host[%d] = %.9f \n",
                                 i, A_host[i]);
        }

        float filter_host[F];
        for(int i = 0; i < sizeof(filter_host) / sizeof(filter_host[0]); i++)
        {
                filter_host[i] = fmod(hb_mc_generate_float_rand(), MAX_VALUE_MAGNITUDE);
                bsg_pr_test_info("filter_host[%d] = %.9f \n",
                                 i, filter_host[i]);
        }
        
        rc = hb_mc_device_memcpy(mc, A_device, A_host, A_size, HB_MC_MEMCPY_TO_DEVICE);
        if(rc != HB_MC_SUCCESS)
        {
                bsg_pr_err("Failed to memcpy A to the manycore.\n");
                return rc;
        }
        
        rc = hb_mc_device_memcpy(mc, filter_device, filter_host, filter_size, HB_MC_MEMCPY_TO_DEVICE);
        if(rc != HB_MC_SUCCESS)
        {
                bsg_pr_err("Failed to memcpy filter to the manycore.\n");
                return rc;
        }

        uint32_t block_size = M;

        hb_mc_dimension_t tilegroup_dim = { .x = 2, .y = 2 };
        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1 };

        int cuda_argv[] = { A_device, N, filter_device, F, P, B_device, S, block_size };
        size_t cuda_argc = sizeof(cuda_argv) / sizeof(cuda_argv[0]);
        rc = hb_mc_kernel_enqueue(mc, grid_dim, tilegroup_dim, "kernel_conv1d", cuda_argc, cuda_argv);
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

        float B_actual[M];
        rc = hb_mc_device_memcpy(mc, B_actual, B_device, B_size, HB_MC_MEMCPY_TO_HOST);
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

        float B_expected[M];
        conv1d(A_host, N, filter_host, F, P, B_expected, S);

        int mismatches = 0;

        bsg_pr_info("(N, F, P, S, M) = (%d, %d, %d, %d, %d)\n", N, F, P, S, M);
        for(int i = 0; i < M; i++)
                if(!hb_mc_floats_match(B_actual[i], B_expected[i]))
                {
                        bsg_pr_err(BSG_RED("Mismatch: ") "B[%d] = %.9f = 0x%x \t Expected: %.9f = 0x%x\n",
                                   i,
                                   B_actual[i],
                                   *((uint32_t *)(B_actual + i)),
                                   B_expected[i],
                                   *((uint32_t *)(B_expected + i)));
                        mismatches++;
                }

        if(!mismatches)
        {
                bsg_pr_test_info(BSG_GREEN("Vectors match!\n"));
                return HB_MC_SUCCESS;
        }
        bsg_pr_test_err(BSG_RED("Vectors don't match!\n"));
        return HB_MC_FAIL;
}

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif
        bsg_pr_test_info("test_conv1d Regression Test \n");
        int rc = kernel_conv1d(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}

