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

#include "test_vec_add_serial_multi_grid.h"

#define ALLOC_NAME "default_allocator"

/*!
 * Runs two separate vector addition kernels sequentially a two grids of 2x2 tile groups. A1[N] + B1[N] --> C1[N], A2[M] + B2[M] --> C2[M]
 * Grid dimensions are determines by how much of a load we want for each tile group (block_size_x)
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/vec_add_serial_multi_grid/ Manycore binary in the BSG Manycore bitbucket repository.  
*/


void host_vec_add (int *A, int *B, int *C, int N) { 
        for (int i = 0; i < N; i ++) { 
                C[i] = A[i] + B[i];
        }
        return;
}


int kernel_vec_add_serial_multi_grid (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running Two Separate CUDA Vector Addition Kernels on two grids of 2x2 tile groups.\n\n");

        srand(time); 


        /*****************************************************************************************************************
        * Define path to binary.
        * Initialize device, load binary and unfreeze tiles.
        ******************************************************************************************************************/
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


        /*****************************************************************************************************************
        * Allocate memory on the device for A1, B1 and C1.
        ******************************************************************************************************************/
        uint32_t N = 1024;

        eva_t A1_device, B1_device, C1_device; 
        rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &A1_device); /* allocate A1[N] on the device */
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allcoate memory on device.\n");
                return rc;
        }


        rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &B1_device); /* allocate B1[N] on the device */
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &C1_device); /* allocate C1[N] on the device */
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        /*****************************************************************************************************************
        * Allocate memory on the device for A2, B2 and C2.
        ******************************************************************************************************************/
        uint32_t M = 512;

        eva_t A2_device, B2_device, C2_device; 
        rc = hb_mc_device_malloc(&device, M * sizeof(uint32_t), &A2_device); /* allocate A2[M] on the device */
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        rc = hb_mc_device_malloc(&device, M * sizeof(uint32_t), &B2_device); /* allocate B2[M] on the device */
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        rc = hb_mc_device_malloc(&device, M * sizeof(uint32_t), &C2_device); /* allocate C2[M] on the device */
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        /*****************************************************************************************************************
        * Allocate memory on the host for A1 & B1 and initialize with random values.
        ******************************************************************************************************************/
        uint32_t A1_host[N]; /* allocate A1[N] on the host */ 
        uint32_t B1_host[N]; /* allocate B1[N] on the host */
        for (int i = 0; i < N; i++) { /* fill A1 & B1 with arbitrary data */
                A1_host[i] = rand() & 0xFFFF;
                B1_host[i] = rand() & 0xFFFF;
        }


        /*****************************************************************************************************************
        * Allocate memory on the host for A2 & B2 and initialize with random values.
        ******************************************************************************************************************/
        uint32_t A2_host[M]; /* allocate A2[M] on the host */ 
        uint32_t B2_host[M]; /* allocate B2[M] on the host */
        for (int i = 0; i < M; i++) { /* fill A2 & B2 with arbitrary data */
                A2_host[i] = rand() & 0xFFFF;
                B2_host[i] = rand() & 0xFFFF;
        }



        /*****************************************************************************************************************
        * Copy A1 & B1 from host onto device DRAM.
        ******************************************************************************************************************/
        void *dst = (void *) ((intptr_t) A1_device);
        void *src = (void *) &A1_host[0];
        rc = hb_mc_device_memcpy (&device, dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE); /* Copy A1 to the device  */        
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory to device.\n");
                return rc;
        }


        dst = (void *) ((intptr_t) B1_device);
        src = (void *) &B1_host[0];
        rc = hb_mc_device_memcpy (&device, dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE); /* Copy B1 to the device */ 
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory to device.\n");
                return rc;
        }


        /*****************************************************************************************************************
        * Copy A2 & B2 from host onto device DRAM.
        ******************************************************************************************************************/
        dst = (void *) ((intptr_t) A2_device);
        src = (void *) &A2_host[0];
        rc = hb_mc_device_memcpy (&device, dst, src, M * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE); /* Copy A2 to the device  */        
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory to device.\n");
                return rc;
        }


        dst = (void *) ((intptr_t) B2_device);
        src = (void *) &B2_host[0];
        rc = hb_mc_device_memcpy (&device, dst, src, M * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE); /* Copy B2 to the device */ 
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory to device.\n");
                return rc;
        }



        /*****************************************************************************************************************
        * Define block_size_x/y: amount of work for each tile group
        * Define tg_dim_x/y: number of tiles in each tile group
        * Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
        ******************************************************************************************************************/
        uint32_t block_size_x1 = 64;
        uint32_t block_size_x2 = 256;

        hb_mc_dimension_t tg_dim_1 = { .x = 2, .y = 2 } ;
        hb_mc_dimension_t tg_dim_2 = { .x = 2, .y = 2 } ;


        hb_mc_dimension_t grid_dim_1 = { .x = N / block_size_x1, .y = 1 }; 
        hb_mc_dimension_t grid_dim_2 = { .x = M / block_size_x2, .y = 1 };



        /*****************************************************************************************************************
        * Prepare list of input arguments for kernel.
        ******************************************************************************************************************/
        int cuda_argv1[5] = {A1_device, B1_device, C1_device, N, block_size_x1};
        int cuda_argv2[5] = {A2_device, B2_device, C2_device, M, block_size_x2};

        /*****************************************************************************************************************
        * Enquque grid 1 of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
        ******************************************************************************************************************/
        rc = hb_mc_kernel_enqueue (&device, grid_dim_1, tg_dim_1, "kernel_vec_add_serial_multi_grid", 5, cuda_argv1);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to initialize grid.\n");
                return rc;
        }


        /*****************************************************************************************************************
        * Launch and execute all tile groups for grid 1 on device and wait for all to finish. 
        ******************************************************************************************************************/
        rc = hb_mc_device_tile_groups_execute(&device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to execute tile groups.\n");
                return rc;
        }



        /*****************************************************************************************************************
        * Enquque grid 2 of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
        ******************************************************************************************************************/
        rc = hb_mc_kernel_enqueue (&device, grid_dim_2, tg_dim_2, "kernel_vec_add_serial_multi_grid", 5, cuda_argv2);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to initialize grid.\n");
                return rc;
        }



        /*****************************************************************************************************************
        * Launch and execute all tile groups for grid 2 on device and wait for all to finish. 
        ******************************************************************************************************************/
        rc = hb_mc_device_tile_groups_execute(&device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to execute tile groups.\n");
                return rc;
        }



        /*****************************************************************************************************************
        * Copy result matrix back from device DRAM into host memory for grid 1. 
        ******************************************************************************************************************/
        uint32_t C1_host[N];
        src = (void *) ((intptr_t) C1_device);
        dst = (void *) &C1_host[0];
        rc = hb_mc_device_memcpy (&device, (void *) dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_HOST); /* copy C1 to the host */
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory from device.\n");
                return rc;
        }


        /*****************************************************************************************************************
        * Copy result matrix back from device DRAM into host memory for grid 2.
        ******************************************************************************************************************/
        uint32_t C2_host[N];
        src = (void *) ((intptr_t) C2_device);
        dst = (void *) &C2_host[0];
        rc = hb_mc_device_memcpy (&device, (void *) dst, src, M * sizeof(uint32_t), HB_MC_MEMCPY_TO_HOST); /* copy C2 to the host */
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory from device.\n");
                return rc;
        }


        /*****************************************************************************************************************
        * Freeze the tiles and memory manager cleanup. 
        ******************************************************************************************************************/
        rc = hb_mc_device_finish(&device); 
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to de-initialize device.\n");
                return rc;
        }


        /*****************************************************************************************************************
        * Calculate the expected result using host code and compare the results. 
        ******************************************************************************************************************/
        uint32_t C1_expected[N]; 
        host_vec_add (A1_host, B1_host, C1_expected, N); 

        uint32_t C2_expected[M]; 
        host_vec_add (A2_host, B2_host, C2_expected, M); 



        int mismatch = 0; 


        for (int i = 0; i < N; i++) {
                if (A1_host[i] + B1_host[i] != C1_host[i]) {
                        bsg_pr_err(BSG_RED("Mismatch: ") "C1[%d]:  0x%08" PRIx32 " + 0x%08" PRIx32 " = 0x%08" PRIx32 "\t Expected: 0x%08" PRIx32 "\n", i , A1_host[i], B1_host[i], C1_host[i], C1_expected[i]);
                        mismatch = 1;
                }
        } 


        for (int i = 0; i < M; i++) {
                if (A2_host[i] + B2_host[i] != C2_host[i]) {
                        bsg_pr_err(BSG_RED("Mismatch: ") "C2[%d]:  0x%08" PRIx32 " + 0x%08" PRIx32 " = 0x%08" PRIx32 "\t Expected: 0x%08" PRIx32 "\n", i , A2_host[i], B2_host[i], C2_host[i], C2_expected[i]);
                        mismatch = 1;
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
        bsg_pr_test_info("test_vec_add_serial_multi_grid Regression Test \n");
        int rc = kernel_vec_add_serial_multi_grid(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}

