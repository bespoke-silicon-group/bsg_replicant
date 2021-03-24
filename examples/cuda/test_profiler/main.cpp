// Copyright (c) 2020, University of Washington All rights reserved.
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
#include <random>
#include <iostream>

// Matrix sizes:
#define HEIGHT (16 * 8)
#define WIDTH  (16 * 8)

// Transpose Matrix A (M x N) into Matrix B (N x M)
template <typename T>
void matrix_transpose (const T *A, T *B, uint64_t M, uint64_t N) {
        for (uint64_t y = 0; y < M; y ++) {
                for (uint64_t x = 0; x < N; x ++) {
                        B[x * M + y] = A[y * N + x];
                }
        }
}

// Matrix C (M x N) = A (M x N) op Matrix B (M x N) =
template <typename T, typename F>
void matrix_op (F op, const T *A, const T *B, T *C, uint64_t M, uint64_t N) {
        for (uint64_t y = 0; y < M; y ++) {
                for (uint64_t x = 0; x < N; x ++) {
                        C[y * N + x] = op(A[y * N + x], B[y * N + x]);
                }
        }
}

// Compute the sum of squared error between matricies A and B (M x N)
template <typename T>
double matrix_sse (const T *A, const T *B, uint64_t M, uint64_t N) {
        double sum = 0;
        for (uint64_t y = 0; y < M; y ++) {
                for (uint64_t x = 0; x < N; x ++) {
                        T diff = A[y * N + x] - B[y * N + x];
                        if(std::isnan(diff)){
                                return diff;
                        }
                        sum += diff * diff;
                }
        }
        return sum;
}

// Print matrix A (M x N). This works well for small matricies.
template <typename T>
void matrix_print(T *A, uint64_t M, uint64_t N) {
        T sum = 0;
        for (uint64_t y = 0; y < M; y ++) {
                for (uint64_t x = 0; x < N; x ++) {
                        std::cout << A[y * N + x] << " ";
                }
                std::cout << '\n';

        }
}


// Run a Matrix-Matrix Multiply test on the Manycore, and compare the result.  A
// and B are the input matricies, C is the destination, and gold is the
// known-good result computed by the host.
template<typename TA, typename TB, typename TC>
int run_test(hb_mc_device_t &device, const char* kernel,
             TA *A, TB *B, TC *C, TC *gold,
             const eva_t &A_device,
             const eva_t &B_device,
             const eva_t &C_device,
             hb_mc_dimension_t tg_dim,
             hb_mc_dimension_t grid_dim){

        void *dst = (void *) ((intptr_t) A_device);
        void *src = (void *) &A[0];
        /* if DMA is not supported just use the normal API */
        if (!hb_mc_manycore_supports_dma_write(device.mc)){

                // Copy A & B from host onto device DRAM.
                BSG_CUDA_CALL(hb_mc_device_memcpy (&device, dst, src,
                                                   (HEIGHT * WIDTH) * sizeof(TA),
                                                   HB_MC_MEMCPY_TO_DEVICE));

                dst = (void *) ((intptr_t) B_device);
                src = (void *) &B[0];
                BSG_CUDA_CALL(hb_mc_device_memcpy (&device, dst, src,
                                                   (HEIGHT * WIDTH) * sizeof(TB),
                                                   HB_MC_MEMCPY_TO_DEVICE));

                dst = (void *) ((intptr_t) C_device);
                src = (void *) &C[0];
                BSG_CUDA_CALL(hb_mc_device_memcpy (&device, dst, src,
                                                   (HEIGHT * WIDTH) * sizeof(TC),
                                                   HB_MC_MEMCPY_TO_DEVICE));

        } else {
                hb_mc_dma_htod_t htod_jobs [] = {
                        {
                                .d_addr = A_device,
                                .h_addr = A,
                                .size   = (HEIGHT * WIDTH) * sizeof(TA)
                        },
                        {
                                .d_addr = B_device,
                                .h_addr = B,
                                .size   = (HEIGHT * WIDTH) * sizeof(TB)
                        },
                        {
                                .d_addr = C_device,
                                .h_addr = C,
                                .size   = (HEIGHT * WIDTH) * sizeof(TC)
                        }
                };

                BSG_CUDA_CALL(hb_mc_device_dma_to_device(&device, htod_jobs, 2));
        }

        // Prepare list of input arguments for kernel. See the kernel source
        // file for the argument uses.
        uint32_t cuda_argv[7] = {A_device, B_device, C_device, HEIGHT, WIDTH, HEIGHT/tg_dim.y, WIDTH/tg_dim.x};

        // Enquque grid of tile groups, pass in grid and tile group dimensions,
        // kernel name, number and list of input arguments
        BSG_CUDA_CALL(hb_mc_kernel_enqueue (&device, grid_dim, tg_dim,
                                            kernel, 7, cuda_argv));

        // Launch and execute all tile groups on device and wait for all to
        // finish.
        uint64_t cycle_start, cycle_end;
        hb_mc_manycore_get_cycle((&device)->mc, &cycle_start);

        BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));

        hb_mc_manycore_get_cycle((&device)->mc, &cycle_end);

        if (!hb_mc_manycore_supports_dma_read(device.mc)){
                // Copy result matrix back from device DRAM into host memory.
                src = (void *) ((intptr_t) C_device);
                dst = (void *) &C[0];
                BSG_CUDA_CALL(hb_mc_device_memcpy (&device, (void *) dst, src,
                                                   (HEIGHT * WIDTH) * sizeof(TC),
                                                   HB_MC_MEMCPY_TO_HOST));
        } else {
                hb_mc_dma_dtoh_t dtoh_job = {
                        .d_addr = C_device,
                        .h_addr = C,
                        .size   = (HEIGHT * WIDTH) * sizeof(TC)
                };

                BSG_CUDA_CALL(hb_mc_device_dma_to_host(&device, &dtoh_job, 1));
        }

        // Generate the known-correct results on the host
        auto add = [](const float A, const float B){return (A + B);};
        auto sub = [](const float A, const float B){return (A - B);};
        auto mul = [](const float A, const float B){return (A * B);};
        matrix_op(add, A, B, gold, HEIGHT, WIDTH);
        matrix_transpose(gold, A, HEIGHT, WIDTH);
        matrix_op(sub, A, B, gold, HEIGHT, WIDTH);
        matrix_op(mul, B, gold, A, HEIGHT, WIDTH);
        matrix_transpose(A, gold, HEIGHT, WIDTH);

        // Compare the known-correct matrix (gold) and the result matrix (C)
        float max = 0.1;
        double sse = matrix_sse(gold, C, HEIGHT, WIDTH);

        matrix_print(C, HEIGHT, WIDTH);

        if (std::isnan(sse) || sse > max) {
                bsg_pr_test_err(BSG_RED("Matrix Mismatch. SSE: %f\n"), sse);
                return HB_MC_FAIL;
        }
        bsg_pr_test_info(BSG_GREEN("Matrix Match.\n"));

        bsg_pr_info("\n\n====== EXECUTION STATISTICS ====== \n");
        bsg_pr_info("Cycles: %lu\n", cycle_end-cycle_start);
        bsg_pr_info("====== END EXECUTION STATISTICS ====== \n\n\n");

        return HB_MC_SUCCESS;
}

// Run a series of Matrix-Matrix multiply tsts on the Manycore device
#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif
        bsg_pr_test_info("test_profiler Regression Test\n");
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running CUDA test_profiler\n");

        // Initialize the random number generators
        std::numeric_limits<int8_t> lim; // Used to get INT_MIN and INT_MAX in C++
        std::default_random_engine generator;
        generator.seed(42);
        std::uniform_real_distribution<float> distribution(lim.min(),lim.max());

        // Allocate A, B, C and R (result) on the host for each datatype.
        // Allocate pointers for B to abstract between when we use B or BT. B is
        // used on kernel versions v0, v1, and v2. BT is used on all subsequent versions.
        float A[HEIGHT * WIDTH];
        float B[HEIGHT * WIDTH];
        float C[HEIGHT * WIDTH];
        float R[HEIGHT * WIDTH];

        // Generate random numbers. Since the infinities, subnormal
        // numbers, or NANs, are annoying (though supported) filter
        // those out.
        auto res = distribution(generator);

        for (uint64_t i = 0; i < HEIGHT * WIDTH; i++) {
                do{
                        res = distribution(generator);
                }while(!std::isnormal(res) ||
                       !std::isfinite(res) ||
                       std::isnan(res));

                A[i] = i;//static_cast<float>(res);
                C[i] = 0.0f;
                R[i] = 0.0f;
        }

        for (uint64_t i = 0; i < HEIGHT * WIDTH; i++) {
                do{
                        res = distribution(generator);
                }while(!std::isnormal(res) ||
                       !std::isfinite(res) ||
                       std::isnan(res));

                B[i] = 0;//static_cast<float>(res);
        }

        // Initialize device, load binary and unfreeze tiles.
        hb_mc_device_t device;
        BSG_CUDA_CALL(hb_mc_device_init(&device, test_name, 0));

        // Define tg_dim_x/y: number of tiles in each tile group
        // Calculate grid_dim_x/y: number of tile groups needed
        hb_mc_dimension_t tg_dim = hb_mc_config_get_dimension_vcore(&(device.mc->config));
        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1 };

        // Initialize the device with a kernel file
        BSG_CUDA_CALL(hb_mc_device_program_init(&device, bin_path, "default_allocator", 0));

        // Allocate memory on the device for A, B and C.
        eva_t A_device, B_device, C_device;

        // Allocate A on the device
        BSG_CUDA_CALL(hb_mc_device_malloc(&device, HEIGHT * WIDTH * sizeof(*A), &A_device));

        // Allocate B on the device
        BSG_CUDA_CALL(hb_mc_device_malloc(&device, HEIGHT * WIDTH * sizeof(*B), &B_device));

        // Allocate C on the device
        BSG_CUDA_CALL(hb_mc_device_malloc(&device, HEIGHT * WIDTH * sizeof(*C), &C_device));

        // Run the test and check the result
        BSG_CUDA_CALL(run_test(device, "kernel_profiler", A, B, C, R,
                               A_device, B_device, C_device, tg_dim, grid_dim));

        // Freeze the tiles and memory manager cleanup.
        BSG_CUDA_CALL(hb_mc_device_finish(&device));

        bsg_pr_test_pass_fail(true);
        return HB_MC_SUCCESS;
}
