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

#include "test_tracer.hpp"

#define ALLOC_NAME "default_allocator"

/*
 * Runs the vector addition a grid of 2x2 tile groups. A[N] + B[N] --> C[N]
 * Grid dimensions are determines by how much of a load we want for each tile group (block_size_x)
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/tracer/ Manycore binary in the BSG Manycore bitbucket repository.  
*/

void host_vec_add (uint32_t *A, uint32_t *B, uint32_t *C, uint32_t N) { 
        for (uint32_t i = 0; i < N; i ++) { 
                C[i] = A[i] + B[i];
        }
        return;
}


int kernel_tracer (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Vector Addition Kernel on a grid of 2x2 tile groups.\n\n");

        srand(32);


        /***********************************************************************
        * Define path to binary.
        * Initialize device, load binary and unfreeze tiles.
        ************************************************************************/
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

        /************************************************************************
        * Allocate memory on the device for A, B and C.
        *************************************************************************/
        uint32_t N = 16384;

        eva_t A_device, B_device, C_device; 
        rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &A_device); /* allocate A[N] on the device */
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &B_device); /* allocate B[N] on the device */
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        rc = hb_mc_device_malloc(&device, N * sizeof(uint32_t), &C_device); /* allocate C[N] on the device */
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }



        /*************************************************************************
        * Allocate memory on the host for A & B and initialize with random values.
        **************************************************************************/
        uint32_t A_host[N]; /* allocate A[N] on the host */ 
        uint32_t B_host[N]; /* allocate B[N] on the host */
        for (int i = 0; i < N; i++) { /* fill A with arbitrary data */
                A_host[i] = rand() & 0xFFFF;
                B_host[i] = rand() & 0xFFFF;
        }



        /**************************************************************************
        * Copy A & B from host onto device DRAM.
        ***************************************************************************/
        void *dst = (void *) ((intptr_t) A_device);
        void *src = (void *) &A_host[0];
        rc = hb_mc_device_memcpy (&device, dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE); /* Copy A to the device  */ 
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory to device.\n");
                return rc;
        }


        dst = (void *) ((intptr_t) B_device);
        src = (void *) &B_host[0];
        rc = hb_mc_device_memcpy (&device, dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE); /* Copy B to the device */ 
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy  memory to device.\n");
                return rc;
        }


        /***************************************************************************
        * Define block_size_x/y: amount of work for each tile group
        * Define tg_dim_x/y: number of tiles in each tile group
        * Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
        ****************************************************************************/
        uint32_t block_size_x = 1024;

        hb_mc_dimension_t tg_dim = { .x = 2, .y = 2 }; 

        hb_mc_dimension_t grid_dim = { .x = N / block_size_x, .y = 1 } ;


        /****************************************************************************
        * Prepare list of input arguments for kernel.
        *****************************************************************************/
        uint32_t cuda_argv[5] = {A_device, B_device, C_device, N, block_size_x};

        /*****************************************************************************
        * Enquque grid of tile groups, pass in grid and tile group
        * dimensions, kernel name, number and list of input arguments
        ******************************************************************************/
        rc = hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_vec_add_parallel", 5, cuda_argv);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to initialize grid.\n");
                return rc;
        }

        /*****************************************************************************
         * Start the tracer (vanilla_operation_trace.csv)
         *****************************************************************************/
        hb_mc_manycore_trace_enable((&device)->mc);

        /*****************************************************************************
         * Start the logger (vanilla.log)
         *****************************************************************************/
        hb_mc_manycore_log_enable((&device)->mc);

        /******************************************************************************
        * Launch and execute all tile groups on device and wait for all to finish. 
        *******************************************************************************/
        rc = hb_mc_device_tile_groups_execute(&device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to execute tile groups.\n");
                return rc;
        }

        /*******************************************************************************
         * Finish logging
         *******************************************************************************/
        hb_mc_manycore_log_disable((&device)->mc);

        /*******************************************************************************
         * Finish Tracing
         *******************************************************************************/
        hb_mc_manycore_trace_disable((&device)->mc);


        /********************************************************************************
        * Copy result matrix back from device DRAM into host memory. 
        *********************************************************************************/
        uint32_t C_host[N];
        src = (void *) ((intptr_t) C_device);
        dst = (void *) &C_host[0];
        rc = hb_mc_device_memcpy (&device, (void *) dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_HOST); /* copy C to the host */
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy  memory from device.\n");
                return rc;
        }


        /*********************************************************************************
        * Freeze the tiles and memory manager cleanup. 
        **********************************************************************************/
        rc = hb_mc_device_finish(&device); 
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to de-initialize device.\n");
                return rc;
        }


        /**********************************************************************************
        * Calculate the expected result using host code and compare the results. 
        ***********************************************************************************/
        uint32_t C_expected[N]; 
        host_vec_add (A_host, B_host, C_expected, N); 


        int mismatch = 0; 
        for (int i = 0; i < N; i++) {
                if (A_host[i] + B_host[i] != C_host[i]) {
                        bsg_pr_err(BSG_RED("Mismatch: ") "C[%d]:  0x%08" PRIx32 " + 0x%08" PRIx32 " = 0x%08" PRIx32 "\t Expected: 0x%08" PRIx32 "\n", i , A_host[i], B_host[i], C_host[i], C_expected[i]);
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
        bsg_pr_test_info("test_tracer Regression Test \n");
        int rc = kernel_tracer(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}


