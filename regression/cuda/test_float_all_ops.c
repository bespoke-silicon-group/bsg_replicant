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
/* A[N] [op] B[N] --> C[N]                                                    */
/* Runs all floating point operations on a vector of input data               */
/* Operations include <add, sub, mul, div, compare, convert>                  */
/* Includes testing of corner cases (+- inf, div by zero, etc.)               */
/* Runs on a 1x1 grid of 2x2 tile group.                                      */
/* Grid dimensions are prefixed at 1x1. --> block_size_x is set to N.         */
/* This tests uses the software/spmd/bsg_cuda_lite_runtime/float_all_ops/     */
/* manycore binary in the BSG Manycore repository.                            */
/******************************************************************************/


#include "test_float_all_ops.h"
//#include <math.h>

#define ALLOC_NAME "default_allocator"

#define NUM_OPS 6
#define NUM_CORNER_CASES 6

char *operations[NUM_OPS] = {"ADD", "SUB", "MUL", "DIV", "CMPR", "CVRT"};

void host_float_all_ops (float *A, float *B,
                         float *res_add, float *res_sub,
                         float *res_mul, float *res_div,
                         float *res_compare, float *res_convert,
                         int N){
        for (int i = 0; i < N; i ++) { 
                res_add[i] = A[i] + B[i];
                res_sub[i] = A[i] - B[i];
                res_mul[i] = A[i] * B[i];
                res_div[i] = A[i] / B[i];
                res_compare[i] = (A[i] >= B[i] ? 1 : 0);
                res_convert[i] = (float) hb_mc_float_to_int(A[i]);
        }
        return;
}

float corner_cases[NUM_CORNER_CASES] = {INFINITY, -INFINITY, NAN, -NAN, 0.0, -0.0};

int kernel_float_all_ops (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Floating Point All Operations "
                         "<add, sub, mul, div, compare, convert> "
                         "Kernel on a 1x1 grid of 2x2 tile group.\n\n");

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
        /* Allocate memory on the device for A, B and all result vectors.     */
        /**********************************************************************/
        uint32_t N = 1024;

        hb_mc_eva_t A_device, B_device;
        hb_mc_eva_t res_add_device, res_sub_device;
        hb_mc_eva_t res_mul_device, res_div_device;
        hb_mc_eva_t res_compare_device, res_convert_device;

        rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &A_device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &B_device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &res_add_device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &res_sub_device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &res_mul_device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &res_div_device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &res_compare_device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &res_convert_device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }



        /**********************************************************************/
        /* Allocate memory on the host for A & B                              */
        /* and initialize with random values and corner cases.                */
        /**********************************************************************/
        float A_host[N]; 
        float B_host[N]; 
        
        // Create every possible combination of corner cases for A & B 
        for (int y = 0; y < NUM_CORNER_CASES; y++) { 
                for (int x = 0; x < NUM_CORNER_CASES; x++) { 
                        A_host[y * NUM_CORNER_CASES + x] = corner_cases[x]; 
                        B_host[y * NUM_CORNER_CASES + x] = corner_cases[y]; 
                }
        }

        // Create every possible combination of A=corner case and B=random float 
        for (int i = 0; i < NUM_CORNER_CASES; i ++) { 
                A_host[NUM_CORNER_CASES * NUM_CORNER_CASES + i] = corner_cases[i];
                B_host[NUM_CORNER_CASES * NUM_CORNER_CASES + i] = hb_mc_generate_float_rand();
        }
        
        // Create every possible combination of A=random float and B=corner case 
        for (int i = 0; i < NUM_CORNER_CASES; i ++) { 
                A_host[NUM_CORNER_CASES * NUM_CORNER_CASES + NUM_CORNER_CASES + i] = hb_mc_generate_float_rand();
                B_host[NUM_CORNER_CASES * NUM_CORNER_CASES + NUM_CORNER_CASES + i] = corner_cases[i];
        }

        // Fill the rest of A and B with random floats
        for (int i = NUM_CORNER_CASES * NUM_CORNER_CASES + 2 * NUM_CORNER_CASES; i < N; i++) { 
                A_host[i] = hb_mc_generate_float_rand();
                B_host[i] = hb_mc_generate_float_rand();
        }
        
        /**********************************************************************/
        /* Copy A & B from host onto device DRAM.                             */
        /**********************************************************************/
        void *dst = (void *) ((intptr_t) A_device);
        void *src = (void *) &A_host[0];
        rc = hb_mc_device_memcpy (&device, dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE);     
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory to device.\n");
                return rc;
        }


        dst = (void *) ((intptr_t) B_device);
        src = (void *) &B_host[0];
        rc = hb_mc_device_memcpy (&device, dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory to device.\n");
                return rc;
        }


        /**********************************************************************/
        /* Initialize values in all result vectors to 0.                      */
        /**********************************************************************/
        rc = hb_mc_device_memset(&device, &res_add_device, 0, N * sizeof(uint32_t));
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to set memory on device.\n");
                return rc;
        } 


        rc = hb_mc_device_memset(&device, &res_sub_device, 0, N * sizeof(uint32_t));
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to set memory on device.\n");
                return rc;
        } 


        rc = hb_mc_device_memset(&device, &res_mul_device, 0, N * sizeof(uint32_t));
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to set memory on device.\n");
                return rc;
        } 


        rc = hb_mc_device_memset(&device, &res_div_device, 0, N * sizeof(uint32_t));
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to set memory on device.\n");
                return rc;
        } 


        rc = hb_mc_device_memset(&device, &res_compare_device, 0, N * sizeof(uint32_t));
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to set memory on device.\n");
                return rc;
        } 


        rc = hb_mc_device_memset(&device, &res_convert_device, 0, N * sizeof(uint32_t));
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
        uint32_t block_size_x = N;

        hb_mc_dimension_t tg_dim = { .x = 2, .y = 2}; 

        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1}; 


        /**********************************************************************/
        /* Prepare list of input arguments for kernel.                        */
        /**********************************************************************/
        int cuda_argv[10] = {A_device, B_device,
                       res_add_device, res_sub_device,
                       res_mul_device, res_div_device,
                       res_compare_device, res_convert_device,
                       N, block_size_x};

        
        /**********************************************************************/
        /* Enquque grid of tile groups, pass in grid and tile group dimensions*/
        /* kernel name, number and list of input arguments                    */
        /**********************************************************************/
        rc = hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_float_all_ops", 10, cuda_argv);
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
        /* Copy all result vectors  back from device DRAM into host memory.   */
        /**********************************************************************/
        float res_add_host[N], res_sub_host[N];
        float res_mul_host[N], res_div_host[N];
        float res_compare_host[N], res_convert_host[N];

        src = (void *) ((intptr_t) res_add_device);
        dst = (void *) &res_add_host[0];
        rc = hb_mc_device_memcpy (&device, (void *) dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_HOST);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory from device.\n");
                return rc;
        }


        src = (void *) ((intptr_t) res_sub_device);
        dst = (void *) &res_sub_host[0];
        rc = hb_mc_device_memcpy (&device, (void *) dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_HOST);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory from device.\n");
                return rc;
        }


        src = (void *) ((intptr_t) res_mul_device);
        dst = (void *) &res_mul_host[0];
        rc = hb_mc_device_memcpy (&device, (void *) dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_HOST);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory from device.\n");
                return rc;
        }


        src = (void *) ((intptr_t) res_div_device);
        dst = (void *) &res_div_host[0];
        rc = hb_mc_device_memcpy (&device, (void *) dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_HOST);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory from device.\n");
                return rc;
        }


        src = (void *) ((intptr_t) res_compare_device);
        dst = (void *) &res_compare_host[0];
        rc = hb_mc_device_memcpy (&device, (void *) dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_HOST);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory from device.\n");
                return rc;
        }


        src = (void *) ((intptr_t) res_convert_device);
        dst = (void *) &res_convert_host[0];
        rc = hb_mc_device_memcpy (&device, (void *) dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_HOST);
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
        float res_add_expected[N], res_sub_expected[N];
        float res_mul_expected[N], res_div_expected[N];
        int res_compare_expected[N], res_convert_expected[N];

        host_float_all_ops (A_host, B_host,
                            res_add_expected, res_sub_expected,
                            res_mul_expected, res_div_expected,
                            res_compare_expected, res_convert_expected,
                            N); 

        float ferror = 0;
        float max_ferror[NUM_OPS];
        for (int op = 0; op < NUM_OPS; op ++) {
                max_ferror[op] = 0.0;
        }


        float *expected [NUM_OPS] = {res_add_expected, res_sub_expected, res_mul_expected, res_div_expected,
                                     res_compare_expected, res_convert_expected};
        float *result [NUM_OPS] =   {res_add_host, res_sub_host, res_mul_host, res_div_host,
                                     res_compare_host, res_convert_host};

        int mismatch = 0;
        for (int op = 0; op < NUM_OPS; op++) { 
                for (int i = 0; i < N; i++) {
                        ferror = hb_mc_calculate_float_error (expected[op][i], result[op][i]);
                        max_ferror[op] = fmax (max_ferror[op], ferror);

                        if ( ferror > MAX_FLOAT_ERROR_TOLERANCE ) { 
                                bsg_pr_err(BSG_RED("Mismatch- %s: ") "C[%d]: %.32f\tExpected: %.32f\tRelative error: %.32f\n",
                                                   operations[op],
                                                   i,
                                                   result[op][i],
                                                   expected[op][i],
                                                   ferror);
                                mismatch = 1;
                        }
                } 
        }

        for (int op = 0; op < NUM_OPS; op ++){  
                bsg_pr_test_info ("MAX relative FP error - %s: %e\n", operations[op], max_ferror[op]); 
        }
                

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
        bsg_pr_test_info("test_float_all_ops Regression Test \n");
        int rc = kernel_float_all_ops(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}


