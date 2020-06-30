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

// Matrix-Matrix Multiplication (A * B = C) on a single tile.  

// A is A_HEIGHT * A_WIDTH, B is B_HEIGHT * B_WIDTH and C is C_HEIGHT *
// C_WIDTH. Each time the kernel is called, it is run for (NUM_ITER + 1)
// iterations and the first iteration is discarded.
// 
// NOTE: A_HEIGHT * A_WIDTH + B_HEIGHT * B_WIDTH + C_HEIGHT * C_WIDTH <= 4KB,
// the size of DMEM on the tile.

#include "test_profiler.hpp"

// Matrix sizes:
#define A_HEIGHT 8
#define A_WIDTH  8
#define B_HEIGHT A_WIDTH
#define B_WIDTH  8
#define C_HEIGHT A_HEIGHT
#define C_WIDTH  B_WIDTH
#define NUM_ITER 10

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

// Matrix utility functions. 

// Transpose Matrix A (M x N) into Matrix B (N x M)
template <typename T>
void matrix_transpose (const T *A, T *B, uint64_t M, uint64_t N) {
        double sum = 0;
        for (uint64_t y = 0; y < M; y ++) {
                for (uint64_t x = 0; x < N; x ++) {
                        B[x * N + y] = A[y * M + x];
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
             const TA *A, const TB *B, TC *C, const TC *gold,
             const eva_t &A_device,
             const eva_t &B_device,
             const eva_t &C_device,
             const hb_mc_dimension_t &tg_dim,
             const hb_mc_dimension_t &grid_dim){
        int rc;

        // Copy A & B from host onto device DRAM.
        void *dst = (void *) ((intptr_t) A_device);
        void *src = (void *) &A[0];
        rc = hb_mc_device_memcpy (&device, dst, src,
                                  (A_HEIGHT * A_WIDTH) * sizeof(TA),
                                  HB_MC_MEMCPY_TO_DEVICE);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_test_err("failed to copy memory to device.\n");
                return rc;
        }

        dst = (void *) ((intptr_t) B_device);
        src = (void *) &B[0];
        rc = hb_mc_device_memcpy (&device, dst, src,
                                  (B_HEIGHT * B_WIDTH) * sizeof(TB),
                                  HB_MC_MEMCPY_TO_DEVICE);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_test_err("failed to copy memory to device.\n");
                return rc;
        }


        // Prepare list of input arguments for kernel. See the kernel source
        // file for the argument uses.
        uint32_t cuda_argv[7] = {A_device, B_device, C_device,
                                  A_HEIGHT, A_WIDTH, B_WIDTH, 
                                  NUM_ITER};

        // Enquque grid of tile groups, pass in grid and tile group dimensions,
        // kernel name, number and list of input arguments
        rc = hb_mc_kernel_enqueue (&device, grid_dim, tg_dim,
                                   kernel, 7, cuda_argv);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_test_err("failed to initialize grid.\n");
                return rc;
        }

        // Launch and execute all tile groups on device and wait for all to
        // finish.
        int instr_start, fops_start;
        int instr_end, fops_end;
        uint64_t cycle_start, cycle_end;
        hb_mc_manycore_get_icount((&device)->mc, e_instr_float, &fops_start);
        hb_mc_manycore_get_icount((&device)->mc, e_instr_all, &instr_start);
        hb_mc_manycore_get_cycle((&device)->mc, &cycle_start);

        rc = hb_mc_device_tile_groups_execute(&device);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_test_err("failed to execute tile groups.\n");
                return rc;
        }

        hb_mc_manycore_get_cycle((&device)->mc, &cycle_end);
        hb_mc_manycore_get_icount((&device)->mc, e_instr_float, &fops_end);
        hb_mc_manycore_get_icount((&device)->mc, e_instr_all, &instr_end);
        
        // Copy result matrix back from device DRAM into host memory.
        src = (void *) ((intptr_t) C_device);
        dst = (void *) &C[0];
        rc = hb_mc_device_memcpy (&device, (void *) dst, src,
                                  (C_HEIGHT * C_WIDTH) * sizeof(TC),
                                  HB_MC_MEMCPY_TO_HOST);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_test_err("failed to copy memory from device.\n");
                return rc;
        }

        // Compare the known-correct matrix (gold) and the result matrix (C)
        float max = 0.1;
        double sse = matrix_sse(gold, C, C_HEIGHT, C_WIDTH);

        if (std::isnan(sse) || sse > max) {
                bsg_pr_test_err(BSG_RED("Matrix Mismatch. SSE: %f\n"), sse);
                return HB_MC_FAIL;
        }
        bsg_pr_test_info(BSG_GREEN("Matrix Match.\n"));

        bsg_pr_info("\n\n====== EXECUTION STATISTICS ====== \n");
        bsg_pr_info("Cycles: %d\n", cycle_end-cycle_start);
        bsg_pr_info("Instrs: %d, Flop Count: %d\n", instr_end-instr_start, fops_end-fops_start);
        bsg_pr_info("Flop Rate: %3.2f\%\n", 100.0f  * static_cast<float>(fops_end-fops_start) / static_cast<float>(instr_end-instr_start));
        bsg_pr_info("IPC: %.2f\%\n", static_cast<float>(instr_end-instr_start) / static_cast<float>(cycle_end-cycle_start));
        bsg_pr_info("====== END EXECUTION STATISTICS ====== \n\n\n");

        return HB_MC_SUCCESS;
}

// Run a series of Matrix-Matrix multiply tsts on the Manycore device
int test_profiler (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running CUDA test_profiler\n");

        // Define tg_dim_x/y: number of tiles in each tile group
        // Calculate grid_dim_x/y: number of tile groups needed
        hb_mc_dimension_t tg_dim = { .x = 1, .y = 1 };
        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1 };

        // Initialize the random number generators
        std::numeric_limits<int8_t> lim; // Used to get INT_MIN and INT_MAX in C++
        std::default_random_engine generator;
        generator.seed(42);
        std::uniform_real_distribution<float> distribution(lim.min(),lim.max());

        // Allocate A, B, BT (B-Transposed), C and R (result) on the host for each datatype.
        // Allocate pointers for B to abstract between when we use B or BT. B is
        // used on kernel versions v0, v1, and v2. BT is used on all subsequent versions.
        float A_f[A_HEIGHT * A_WIDTH];
        float B_f[B_HEIGHT * B_WIDTH], BT_f[B_HEIGHT * B_WIDTH], *B_fp;
        float C_f[C_HEIGHT * C_WIDTH];
        float R_f[C_HEIGHT * C_WIDTH];
        
        // Generate random numbers. Since the infinities, subnormal
        // numbers, or NANs, are annoying (though supported) filter
        // those out.
        auto res = distribution(generator);

        for (uint64_t i = 0; i < A_HEIGHT * A_WIDTH; i++) {
                do{
                        res = distribution(generator);
                }while(!std::isnormal(res) ||
                       !std::isfinite(res) ||
                       std::isnan(res));

                A_f[i] = static_cast<float>(res);
        }

        for (uint64_t i = 0; i < B_HEIGHT * B_WIDTH; i++) {
                do{
                        res = distribution(generator);
                }while(!std::isnormal(res) ||
                       !std::isfinite(res) ||
                       std::isnan(res));

                B_f[i] = static_cast<float>(res);
        }

        // Generate the known-correct results on the host
        matrix_mult (A_f, B_f, R_f, A_HEIGHT, A_WIDTH, B_WIDTH);

        matrix_transpose(B_f, BT_f, B_HEIGHT, B_WIDTH);

        // Initialize device, load binary and unfreeze tiles.
        hb_mc_device_t device;
        rc = hb_mc_device_init(&device, test_name, 0);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_test_err("failed to initialize device.\n");
                return rc;
        }

        // Initialize the device with a kernel file
        rc = hb_mc_device_program_init(&device, bin_path, "default_allocator", 0);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_test_err("failed to initialize program.\n");
                return rc;
        }

        // Allocate memory on the device for A, B and C. 

        eva_t A_device, B_device, C_device;

        // Allocate A on the device
        rc = hb_mc_device_malloc(&device,
                                 A_HEIGHT * A_WIDTH * sizeof(float),
                                 &A_device);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_test_err("failed to allocate memory on device.\n");
                return rc;
        }

        // Allocate B on the device
        rc = hb_mc_device_malloc(&device,
                                 B_HEIGHT * B_WIDTH * sizeof(float),
                                 &B_device);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_test_err("failed to allocate memory on device.\n");
                return rc;
        }

        // Allocate C on the device
        rc = hb_mc_device_malloc(&device,
                                 C_HEIGHT * C_WIDTH * sizeof(float),
                                 &C_device);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_test_err("failed to allocate memory on device.\n");
                return rc;
        }

        // Run the 32-bit integer test and check the result
        rc = run_test(device, "kernel_matrix_multiply_transpose_nomul_unroll_init_local",
                      A_f, BT_f, C_f, R_f,
                      A_device, B_device, C_device,
                      tg_dim, grid_dim);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_test_err("kernel_matrix_multiply test failed\n");
                return rc;
        }
        bsg_pr_test_info("kernel_matrix_multiply test passed!\n");

        // Freeze the tiles and memory manager cleanup.
        rc = hb_mc_device_finish(&device);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_test_err("failed to de-initialize device.\n");
                return rc;
        }

        return HB_MC_SUCCESS;
}

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif
        bsg_pr_test_info("test_profiler Regression Test\n");
        int rc = test_profiler(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
