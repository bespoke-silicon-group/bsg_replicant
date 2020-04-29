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

/******************************************************************************/
/* A[N] * B[N] --> C[N]                                                       */
/* Runs the floating point matrix multiplication on a group of 2x2 tile groups*/
/* Grid dimensions are defined based on total work / block_size_x/y           */
/* This tests uses the software/spmd/bsg_cuda_lite_runtime/float_matrix_mul/  */
/* manycore binary in the BSG Manycore repository.                            */
/******************************************************************************/


#include "test_float_matrix_mul.h"

#define ALLOC_NAME "default_allocator"


/*! 
 * Matrix multiplication code on the host side to compare the results
 */
void host_float_matrix_mul (float *A, float *B, float *C, int M, int N, int P) { 
        for (int y = 0; y < M; y ++) { 
                for (int x = 0; x < P; x ++) { 
                        float res = 0;
                        for (int k = 0; k < N; k++) { 
                                res += A[y * N + k] * B[k * P + x];
                        }
                        C[y * P + x] = res;
                }
        }
        return;
}
        


int kernel_float_matrix_mul (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Floating Point Matrix Multiplication "
                         "Kernel on a grid of 2x2 tile group.\n\n");

        srand(time(NULL)); 


        /**********************************************************************/
        /* Define path to binary.                                             */
        /* Initialize device, load binary and unfreeze tiles.                 */
        /**********************************************************************/
        hb_mc_device_t device;
        rc = hb_mc_device_init(&device, test_name, 0);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to initialize device.\n");
                return rc;
        }


        rc = hb_mc_device_program_init(&device, bin_path, ALLOC_NAME, 0);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to initialize program.\n");
                return rc;
        }


        /**********************************************************************/
        /* Allocate memory on the device for A, B and C.                      */
        /**********************************************************************/
        uint32_t M = 32;
        uint32_t N = 64;
        uint32_t P = 16;

        hb_mc_eva_t A_device, B_device, C_device; 
        rc = hb_mc_device_malloc(&device, M * N * sizeof(uint32_t), &A_device); /* allocate A[M][N] on the device */
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        rc = hb_mc_device_malloc(&device, N * P * sizeof(uint32_t), &B_device); /* allocate B[N][P] on the device */
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        rc = hb_mc_device_malloc(&device, M * P * sizeof(uint32_t), &C_device); /* allocate C[M][P] on the device */
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        /**********************************************************************/
        /* Allocate memory on the host for A & B                              */
        /* and initialize with random values.                                 */
        /**********************************************************************/
        float A_host[M * N]; 
        float B_host[N * P]; 
        for (int i = 0; i < M * N; i++) { 
                A_host[i] = hb_mc_generate_float_rand();

        }

        for (int i = 0; i < N * P; i++) { 
                B_host[i] = hb_mc_generate_float_rand();
        }


        /**********************************************************************/
        /* Copy A & B from host onto device DRAM.                             */
        /**********************************************************************/
        void *dst = (void *) ((intptr_t) A_device);
        void *src = (void *) &A_host[0];
        rc = hb_mc_device_memcpy (&device, dst, src, M * N * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE); 
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory to device.\n");
                return rc;
        }


        dst = (void *) ((intptr_t) B_device);
        src = (void *) &B_host[0];
        rc = hb_mc_device_memcpy (&device, dst, src, N * P * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory to device.\n");
                return rc;
        }


        /**********************************************************************/
        /* Initialize values in C_device to 0.                                */
        /**********************************************************************/
        rc = hb_mc_device_memset(&device, &C_device, 0, M * P * sizeof(uint32_t));
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to set memory on device.\n");
                return rc;
        } 



        /**********************************************************************/
        /* Define block_size_x/y: amount of work for each tile group          */
        /* Define tg_dim_x/y: number of tiles in each tile group              */
        /* Calculate grid_dim_x/y: number of                                  */
        /* tile groups needed based on block_size_x/y                         */
        /**********************************************************************/
        uint32_t block_size_x = 4;
        uint32_t block_size_y = 4;

        hb_mc_dimension_t tg_dim = { .x = 2, .y = 2 };

        hb_mc_dimension_t grid_dim = { .x = P / block_size_x, .y = M / block_size_y };



        /**********************************************************************/
        /* Prepare list of input arguments for kernel.                        */
        /**********************************************************************/
        int cuda_argv[8] = {A_device, B_device, C_device, M, N, P,  block_size_y, block_size_x};

        
        /**********************************************************************/
        /* Enquque grid of tile groups, pass in grid and tile group dimensions*/
        /* kernel name, number and list of input arguments                    */
        /**********************************************************************/
        rc = hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_float_matrix_mul", 8, cuda_argv);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to initialize grid.\n");
                return rc;
        }



        /**********************************************************************/
        /* Launch and execute all tile groups on device and wait for finish.  */ 
        /**********************************************************************/
        rc = hb_mc_device_tile_groups_execute(&device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to execute tile groups.\n");
                return rc;
        }


        /**********************************************************************/
        /* Copy result matrix back from device DRAM into host memory.         */
        /**********************************************************************/
        float C_host[M * P];
        src = (void *) ((intptr_t) C_device);
        dst = (void *) &C_host[0];
        rc = hb_mc_device_memcpy (&device, (void *) dst, src, M * P * sizeof(uint32_t), HB_MC_MEMCPY_TO_HOST);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory from device.\n");
                return rc;
        }


        /**********************************************************************/
        /* Freeze the tiles and memory manager cleanup.                       */
        /**********************************************************************/
        rc = hb_mc_device_finish(&device); 
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to de-initialize device.\n");
                return rc;
        }


        /**********************************************************************/
        /* Calculate the expected result using host code and compare.         */ 
        /**********************************************************************/
        float C_expected[M * P]; 
        host_float_matrix_mul (A_host, B_host, C_expected, M, N, P); 

        float max_ferror = 0; 
        float ferror = 0;

        int mismatch = 0; 
        for (int y = 0; y < M; y ++) { 
                for (int x = 0; x < P; x ++){ 
                        ferror = hb_mc_calculate_float_error(C_expected[y * P + x], C_host[y * P + x]);
                        max_ferror = fmax ( max_ferror, ferror);        
                        if ( ferror > MAX_FLOAT_ERROR_TOLERANCE ) { 
                                bsg_pr_err(BSG_RED("Mismatch: ") "C[%d][%d]: %.32f\tExpected: %.32f\tRelative error: %.32f\n",
                                                   y,
                                                   x,
                                                   C_host[y * P + x],
                                                   C_expected[y * P + x],
                                                   ferror);
                                mismatch = 1;
                        }
                } 
        }

        bsg_pr_test_info ("MAX relative FP error: %e\n", max_ferror); 

        if (mismatch) { 
                return HB_MC_FAIL;
        }
        return HB_MC_SUCCESS;
}

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif
        bsg_pr_test_info("test_float_matrix_mul Regression Test\n");
        int rc = kernel_float_matrix_mul(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}


