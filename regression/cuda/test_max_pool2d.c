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
/* Runs the max pool 2d test on a grid of 2x2 tile groups.                    */
/* Reduce a MxN matrix to a PxW matrix. A[MxN] --> B[PxW]                     */
/* M and N should divide evenly by P and W, respectively.                     */
/* Each element in result matrix is the max of a sub-matrix in input matrix.  */
/* Grid dimensions are prefixed at 1x1.                                       */
/* This tests uses the software/spmd/bsg_cuda_lite_runtime/max_pool2d/        */
/* manycore binary in the BSG Manycore repository.                            */
/******************************************************************************/


#include "test_max_pool2d.h"

#define ALLOC_NAME "default_allocator"

#define M 32
#define N 16 
#define P 4
#define W 4


int host_max_pool2d (int *A, int *B, int MM, int NN, int PP, int WW) { 
        if (!A) { 
                bsg_pr_err("%s: invalid memory pointer to A in host.\n",
                           __func__);
                return HB_MC_INVALID;
        }  
        if (!B) { 
                bsg_pr_err("%s: invalid memory pointer to B in host.\n",
                           __func__);
                return HB_MC_INVALID;
        }  
        
        int sub_block_y = MM / PP; // Must divide evenly
        int sub_block_x = NN / WW; // Must divide evenly

        for (int iter_y = 0; iter_y < PP; iter_y ++) { 
                for (int iter_x = 0; iter_x < WW; iter_x ++) { 
                        int start_y = iter_y * sub_block_y; 
                        int start_x = iter_x * sub_block_x;
                        int end_y = start_y + sub_block_y;
                        int end_x = start_x + sub_block_x;
                        int sub_max = A[start_y * NN + start_x];                        

                        for (int y = start_y; y < end_y; y ++) { 
                                for (int x = start_x; x < end_x; x ++) {
                                        if (A[y * NN + x] > sub_max) { 
                                                sub_max = A[y * NN + x];
                                        }
                                }
                        }

                        B[iter_y * WW + iter_x] = sub_max;
                }
        }

        return HB_MC_SUCCESS;
}



int kernel_max_pool2d(int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Max Pool 2D Kernel "
                         "on a grid of 2x2 tile groups.\n\n");

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
        /* Allocate memory on the device for A & B.                           */
        /**********************************************************************/
        hb_mc_eva_t A_device; 
        rc = hb_mc_device_malloc(&device, M * N * sizeof(uint32_t), &A_device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }

        hb_mc_eva_t B_device; 
        rc = hb_mc_device_malloc(&device, P * W * sizeof(uint32_t), &B_device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        /**********************************************************************/
        /* Allocate memory on the host for A & B and initialize.              */
        /**********************************************************************/
        uint32_t A_host[M*N]; 
        uint32_t B_result[P*W];
        for (int i = 0; i < M * N; i++) {
                A_host[i] = i & 0xFFFF;
        }

        for (int i = 0; i < P * W; i++) {
                B_result[i] = 0 & 0xFFFF;
        }


        /**********************************************************************/
        /* Copy A to device and set B to zero.                                */
        /**********************************************************************/
        void *dst = (void *) ((intptr_t) A_device);
        void *src = (void *) &A_host[0];
        rc = hb_mc_device_memcpy (&device, dst, src, M * N * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory to device.\n");
                return rc;
        }

        rc = hb_mc_device_memset(&device, &B_device, 0 , P * W * sizeof(uint8_t));
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

        hb_mc_dimension_t grid_dim = { .x = W / block_size_x, .y = P / block_size_y };


        /**********************************************************************/
        /* Prepare list of input arguments for kernel.                        */
        /**********************************************************************/
        int cuda_argv[8] = {A_device, B_device, M, N, P, W, block_size_y, block_size_x};


        /**********************************************************************/
        /* Enquque grid of tile groups, pass in grid and tile group dimensions*/
        /* kernel name, number and list of input arguments                    */
        /**********************************************************************/
        rc = hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_max_pool2d", 8, cuda_argv);
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
        src = (void *) ((intptr_t) B_device);
        dst = (void *) &B_result;
        rc = hb_mc_device_memcpy (&device, (void *) dst, src, P * W * sizeof(uint32_t), HB_MC_MEMCPY_TO_HOST);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory from device.\n");
                return rc;
        }


        /**********************************************************************/
        /* Freeze the tiles and memory manager cleanup.                       */
        /**********************************************************************/
        rc = hb_mc_device_free(&device, A_device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to free A_device.\n", __func__);
                return rc;
        }
        rc = hb_mc_device_free(&device, B_device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to free B_device.\n", __func__);
                return rc;
        }


        rc = hb_mc_device_finish(&device); 
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to de-initialize device.\n");
                return rc;
        }


        /**********************************************************************/
        /* Calculate expected results on the host and compare.                */
        /**********************************************************************/
        uint32_t B_expected[P * W]; 
        rc = host_max_pool2d (A_host, B_expected, M, N, P, W); 
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to calculate result on host.\n",
                           __func__);
                return rc;
        }


        /**********************************************************************/
        /* Compare results from device with results calculated by host.       */
        /**********************************************************************/
        int mismatch = 0;
        for (int y = 0; y < P; y ++) { 
                for (int x = 0; x < W; x ++) { 
                        if (B_result[y * W + x] != B_expected[y * W + x]) { 
                                bsg_pr_err(BSG_RED("Mismatch: ") "B[%d][%d] = %d\t Expected: %d.\n", y, x, B_result[y * P + x], B_expected[y * P + x]); 
                                mismatch = 1;
                        }
                }
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
        bsg_pr_test_info("test_max_pool2d Regression Test\n");
        int rc = kernel_max_pool2d(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}


