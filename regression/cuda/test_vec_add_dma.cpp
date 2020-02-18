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

#include "test_vec_add_dma.hpp"

#define ALLOC_NAME "default_allocator"
#define DMA
#define CUDA_CALL(expr)                                                 \
        {                                                               \
                int __err;                                              \
                __err = expr;                                           \
                if (__err != HB_MC_SUCCESS) {                           \
                        bsg_pr_err("'%s' failed: %s\n", #expr, hb_mc_strerror(__err)); \
                        return __err;                                   \
                }                                                       \
        }

/*!
 * Runs the vector addition a one 2x2 tile groups. A[N] + B[N] --> C[N]
 * Grid dimensions are prefixed at 1x1. --> block_size_x is set to N.
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/vec_add/ Manycore binary in the BSG Manycore bitbucket repository.
*/
void host_vec_add (int32_t *A, int32_t *B, int32_t *C, int N) {
        for (int i = 0; i < N; i ++) {
                C[i] = A[i] + B[i];
        }
        return;
}


int kernel_vec_add (int argc, char **argv) {
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Vector Addition Kernel on one 2x2 tile groups.\n");

        srand(static_cast<unsigned>(time(0)));

        /* Define path to binary. */
        /* Initialize device, load binary and unfreeze tiles. */
        hb_mc_dimension_t tg_dim = { .x = 2, .y = 2};
        hb_mc_device_t device;
        CUDA_CALL(hb_mc_device_init_custom_dimensions(&device, test_name, 0, tg_dim));

	/* if DMA is not supported just return SUCCESS */
	if (!hb_mc_manycore_supports_dma_write(device.mc)
	    || !hb_mc_manycore_supports_dma_read(device.mc)) {
		bsg_pr_test_info("DMA not supported for this machine: returning success\n");
		return HB_MC_SUCCESS;
	}

        CUDA_CALL(hb_mc_device_program_init(&device, bin_path, ALLOC_NAME, 0));

        /* Allocate memory on the device for A, B and C. */
        constexpr int N = (1 << 16);
        constexpr size_t vsize = N * sizeof(uint32_t);
	bsg_pr_test_info("Using DMA to write vectors of %d integers\n", N);

        eva_t A_device, B_device, C_device;
        CUDA_CALL(hb_mc_device_malloc(&device, vsize, &A_device)); /* allocate A[N] on the device */
        CUDA_CALL(hb_mc_device_malloc(&device, vsize, &B_device)); /* allocate B[N] on the device */
        CUDA_CALL(hb_mc_device_malloc(&device, vsize, &C_device)); /* allocate C[N] on the device */


        /* Allocate memory on the host for A & B and initialize with random values. */
        int32_t *A_host = new int32_t [N]; /* allocate A[N] on the host */
        int32_t *B_host = new int32_t [N]; /* allocate B[N] on the host */

        /* fill A with arbitrary data */
        for (int i = 0; i < N; i++) {
                A_host[i] = rand() & 0xFFFF;
                B_host[i] = rand() & 0xFFFF;
        }

        /* Copy A & B from host onto device DRAM. */
        hb_mc_dma_htod_t htod_jobs [] = {
                {
                        .d_addr = A_device,
                        .h_addr = A_host,
                        .size   = vsize
                },
                {
                        .d_addr = B_device,
                        .h_addr = B_host,
                        .size   = vsize
                }
        };

        bsg_pr_test_info("Writing A and B to device\n");

        CUDA_CALL(hb_mc_device_dma_to_device(&device, htod_jobs, 2));

        /* Define block_size_x/y: amount of work for each tile group */
        /* Define tg_dim_x/y: number of tiles in each tile group */
        /* Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y */
        uint32_t block_size_x = N;
        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1};


        /* Prepare list of input arguments for kernel. */
        uint32_t cuda_argv[5] = {A_device, B_device, C_device, N, block_size_x};

        /* Enqqueue grid of tile groups, pass in grid and tile group dimensions,
           kernel name, number and list of input arguments */
        CUDA_CALL(hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_vec_add", 5, cuda_argv));

        /* Launch and execute all tile groups on device and wait for all to finish.  */
        CUDA_CALL(hb_mc_device_tile_groups_execute(&device));

        /* Copy result matrix back from device DRAM into host memory.  */
        int32_t *C_host = new int32_t [N];

        hb_mc_dma_dtoh_t dtoh_job = {
                .d_addr = C_device,
                .h_addr = C_host,
                .size   = vsize
        };

        bsg_pr_test_info("Reading C to host\n");

        CUDA_CALL(hb_mc_device_dma_to_host(&device, &dtoh_job, 1));

        /* Freeze the tiles and memory manager cleanup.  */
        CUDA_CALL(hb_mc_device_finish(&device));


        /* Calculate the expected result using host code and compare the results.  */
        int32_t *C_expected = new int32_t [N];
        host_vec_add (A_host, B_host, C_expected, N);


        int mismatch = 0;
        for (int i = 0; i < N; i++) {
                if (A_host[i] + B_host[i] != C_host[i]) {
                        bsg_pr_err(BSG_RED("Mismatch: ")
                                   "C[%d]:  0x%08" PRIx32
                                   " + 0x%08" PRIx32
                                   " = 0x%08" PRIx32
                                   "\t Expected: 0x%08" PRIx32 "\n",
                                   i ,
                                   A_host[i],
                                   B_host[i],
                                   C_host[i],
                                   C_expected[i]);
                        mismatch = 1;
                }
        }

        return mismatch ? HB_MC_FAIL : HB_MC_SUCCESS;
}

#ifdef COSIM
void cosim_main(uint32_t *exit_code, char * args) {
        // We aren't passed command line arguments directly so we parse them
        // from *args. args is a string from VCS - to pass a string of arguments
        // to args, pass c_args to VCS as follows: +c_args="<space separated
        // list of args>"
        int argc = get_argc(args);
        char *argv[argc];
        get_argv(args, argc, argv);

#ifdef VCS
        svScope scope;
        scope = svGetScopeFromName("tb");
        svSetScope(scope);
#endif
        bsg_pr_test_info("test_vec_add Regression Test (COSIMULATION)\n");
        int rc = kernel_vec_add(argc, argv);
        *exit_code = rc;
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return;
}
#else
int main(int argc, char ** argv) {
        bsg_pr_test_info("test_vec_add Regression Test (F1)\n");
        int rc = kernel_vec_add(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
#endif
