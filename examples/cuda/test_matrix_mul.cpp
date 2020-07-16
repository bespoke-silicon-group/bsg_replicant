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


// This test performs naive paralle tiled matrix multiplication
// *** WARNING ***: Tile group dimensions are fixed at 4x4
// If you change tile group dimensions, you have to use matching 
// tile group dimensions in the kernel code residing at 
// "bsg_bladerunner/bsg_manycore/software/spmd/bsg_cuda_lite_runtime/
// matrix_mul/kernel_matrix_mul.cpp"

#include "test_matrix_mul.hpp"

#define DMA

// Matrix sizes:
#define A_HEIGHT 32        // M
#define A_WIDTH  32        // N
#define B_HEIGHT A_WIDTH
#define B_WIDTH  32        // P
#define C_HEIGHT A_HEIGHT
#define C_WIDTH  B_WIDTH
#define NUM_ITER 1

// Host Matrix multiplication code (to compare results)
template <typename TA, typename TB, typename TC>
void matrix_mult (TA *A, TB *B, TC *C, uint64_t M, uint64_t N, uint64_t P) {
        for (uint64_t y = 0; y < M; y ++) {
                for (uint64_t x = 0; x < P; x ++) {
                        TC res = 0.0f;
                        for (uint64_t k = 0; k < N; k++) {
                                res += A[y * N + k] * B[k * P + x];
                        }
                        C[y * P + x] = res;
                }
        }
        return;
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

// Print matrix A (M x N). This works well for small matricies.
template <typename T>
void matrix_print_diff(T *A, T *B, uint64_t M, uint64_t N, T max_diff) {
        T sum = 0;
        for (uint64_t y = 0; y < M; y ++) {
                for (uint64_t x = 0; x < N; x ++) {
                        T diff = A[y * N + x] - B[y * N + x];
                        if (diff * diff <= max_diff)
                            //std::cout << A[y * N + x] << " ";
                            printf(BSG_GREEN("%.2f "), A[y * N + x]);
                        else
                            //std::cout << A[y * N + x] << " ";
                            printf(BSG_RED("%.2f "), A[y * N + x]);
                }
                std::cout << '\n';

        }
}


int kernel_matrix_mul (int argc, char **argv) {

        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Tile-Group Matrix-Matrix "
                         "Multiplication Kernel.\n\n");

        // Define block_size_x/y: amount of work for each tile group
        // Define tg_dim_x/y: number of tiles in each tile group
        // Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
        uint32_t block_size_x = 0;
        uint32_t block_size_y = 0;
        block_size_x = C_WIDTH;
        block_size_y = C_HEIGHT;
        hb_mc_dimension_t tg_dim = { .x = 4, .y = 4 };
        hb_mc_dimension_t grid_dim = {.x = 1, .y = 1};

        // Initialize the random number generators
        std::numeric_limits<int8_t> lim; // Used to get INT_MIN and INT_MAX in C++
        std::default_random_engine generator;
        generator.seed(42);
        std::uniform_real_distribution<float> distribution(lim.min(),lim.max());

        // Allocate A, B, BT, C and R (result) on the host
        float A[A_HEIGHT * A_WIDTH];
        float B[B_HEIGHT * B_WIDTH];
        float C[C_HEIGHT * C_WIDTH];
        float R[C_HEIGHT * C_WIDTH];

        // Generate random numbers. Since the Manycore can't handle infinities,
        // subnormal numbers, or NANs, filter those out.
        auto res = distribution(generator);

        for (uint64_t i = 0; i < A_HEIGHT * A_WIDTH; i++) {
                do{
                        res = distribution(generator);
                }while(!std::isnormal(res) ||
                       !std::isfinite(res) ||
                       std::isnan(res));

                A[i] = static_cast<float>(res);
        }

        for (uint64_t i = 0; i < B_HEIGHT * B_WIDTH; i++) {
                do{
                        res = distribution(generator);
                }while(!std::isnormal(res) ||
                       !std::isfinite(res) ||
                       std::isnan(res));

                B[i] = static_cast<float>(res);
        }

        for (uint64_t i = 0; i < C_HEIGHT * C_WIDTH; i++) {
            C[i] = 0;
        }

        // Generate the known-correct results on the host
        matrix_mult (A, B, R, A_HEIGHT, A_WIDTH, B_WIDTH);

        // Initialize device, load binary and unfreeze tiles.
        hb_mc_device_t device;
        BSG_CUDA_CALL(hb_mc_device_init_custom_dimensions(&device, test_name, 0, tg_dim));

        // if DMA is not supported just return SUCCESS 
        if (!hb_mc_manycore_supports_dma_write(device.mc)
            || !hb_mc_manycore_supports_dma_read(device.mc)) {
                bsg_pr_test_info("DMA not supported for this machine: returning fail.\n");
                return HB_MC_FAIL;
        }

        BSG_CUDA_CALL(hb_mc_device_program_init(&device, bin_path, "default_allocator", 0));

        // Allocate memory on the device for A, B and C. Since sizeof(float) ==
        // sizeof(int32_t) > sizeof(int16_t) > sizeof(int8_t) we'll reuse the
        // same buffers for each test (if multiple tests are conducted)
        hb_mc_eva_t A_device, B_device, C_device;

        constexpr size_t A_size = A_HEIGHT * A_WIDTH * sizeof(float);
        constexpr size_t B_size = B_HEIGHT * B_WIDTH * sizeof(float);
        constexpr size_t C_size = C_HEIGHT * C_WIDTH * sizeof(float);


        // Allocate A on the device
        BSG_CUDA_CALL(hb_mc_device_malloc(&device, A_size, &A_device));

        // Allocate B on the device
        BSG_CUDA_CALL(hb_mc_device_malloc(&device, B_size, &B_device));

        // Allocate C on the device
        BSG_CUDA_CALL(hb_mc_device_malloc(&device, C_size, &C_device));

        // Copy A & B from host onto device DRAM.
        hb_mc_dma_htod_t htod_jobs [] = {
                {
                        .d_addr = A_device,
                        .h_addr = A,
                        .size   = A_size
                },
                {
                        .d_addr = B_device,
                        .h_addr = B,
                        .size   = B_size
                },
                {
                        .d_addr = C_device,
                        .h_addr = C,
                        .size   = C_size
                }
        };

        BSG_CUDA_CALL(hb_mc_device_dma_to_device(&device, htod_jobs, 2));

        // // Copy A & B from host onto device DRAM.
        // void *dst = (void *) ((intptr_t) A_device);
        // void *src = (void *) &A[0];
        // BSG_CUDA_CALL(hb_mc_device_memcpy (&device, dst, src,
        //                                    (A_HEIGHT * A_WIDTH) * sizeof(A[0]),
        //                                    HB_MC_MEMCPY_TO_DEVICE));


        // dst = (void *) ((intptr_t) B_device);
        // src = (void *) &B[0];
        // BSG_CUDA_CALL(hb_mc_device_memcpy (&device, dst, src,
        //                                    (B_HEIGHT * B_WIDTH) * sizeof(B[0]),
        //                                    HB_MC_MEMCPY_TO_DEVICE));

        // Prepare list of input arguments for kernel.
        uint32_t cuda_argv[8] = {A_device, B_device, C_device,
                                 A_HEIGHT, A_WIDTH, B_WIDTH,
                                 block_size_y, block_size_x};

        // Enquque grid of tile groups, pass in grid and tile group dimensions,
        // kernel name, number and list of input arguments
        BSG_CUDA_CALL(hb_mc_kernel_enqueue (&device, grid_dim, tg_dim,
                                            "kernel_matrix_mul", 8, cuda_argv))

        // Start the tracer (vanilla_operation_trace.csv)
        // hb_mc_manycore_trace_enable((&device)->mc);

        // Start the logger (vanilla.log)
        // hb_mc_manycore_log_enable((&device)->mc);


        // Launch and execute all tile groups on device and wait for all to finish.
        BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));


        // Finish logging
        // hb_mc_manycore_log_disable((&device)->mc);

        // Finish Tracing
        // hb_mc_manycore_trace_disable((&device)->mc);


        // Copy result matrix back from device DRAM into host memory.
        hb_mc_dma_dtoh_t dtoh_job = {
                .d_addr = C_device,
                .h_addr = C,
                .size   = C_size 
        };

        BSG_CUDA_CALL(hb_mc_device_dma_to_host(&device, &dtoh_job, 1));

        // // Copy result matrix back from device DRAM into host memory.
        // src = (void *) ((intptr_t) C_device);
        // dst = (void *) &C[0];
        // BSG_CUDA_CALL(hb_mc_device_memcpy (&device, dst, src,
        //                                    (C_HEIGHT * C_WIDTH) * sizeof(float),
        //                                    HB_MC_MEMCPY_TO_HOST));


        // Freeze the tiles and memory manager cleanup.
        BSG_CUDA_CALL(hb_mc_device_finish(&device));

        // Compare the known-correct matrix (R) and the result matrix (C)
        float max = 1.0;
        double sse = matrix_sse(R, C, C_HEIGHT, C_WIDTH);

        if (std::isnan(sse) || sse > max) {
                bsg_pr_test_info(BSG_RED("Matrix Mismatch. SSE: %f\n"), sse);
                return HB_MC_FAIL;
        }

        bsg_pr_test_info(BSG_GREEN("Matrix Match.\n"));
        return HB_MC_SUCCESS;
}



#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif
        bsg_pr_test_info("test_vec_add Regression Test\n");
        int rc = kernel_matrix_mul(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}

