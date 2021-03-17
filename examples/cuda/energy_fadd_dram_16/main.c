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

#define ALLOC_NAME "default_allocator"

void host_vec_add (float *A, float *B, float *C, int N) {
        for (int i = 0; i < N; i ++) {
                C[i] = A[i] + B[i];
        }
        return;
}


int kernel_energy_ubmark (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Energy ubmark\n\n");

        srand(time);

        /*********************/
        /* Initialize device */
        /*********************/
        hb_mc_device_t device;
        BSG_CUDA_CALL(hb_mc_device_init(&device, test_name, 0));

        hb_mc_pod_id_t pod;
        hb_mc_device_foreach_pod_id(&device, pod)
        {
                /**********************************************************************/
                /* Define path to binary.                                             */
                /* Initialize device, load binary and unfreeze tiles.                 */
                /**********************************************************************/
                bsg_pr_test_info("Loading program for %s onto pod %d\n",
                                 test_name, pod);

                BSG_CUDA_CALL(hb_mc_device_set_default_pod(&device, pod));
                BSG_CUDA_CALL(hb_mc_device_program_init(&device, bin_path, ALLOC_NAME, 0));

                /*****************************************************************************************************************
                 * Allocate memory on the device for A, B and C.
                 ******************************************************************************************************************/
                uint32_t N = 8192;

                eva_t A_device, B_device, C_device;
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, N * sizeof(float), &A_device)); /* allocate A[N] on the device */
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, N * sizeof(float), &B_device)); /* allocate B[N] on the device */
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, N * sizeof(float), &C_device)); /* allocate C[N] on the device */

                /*****************************************************************************************************************
                 * Allocate memory on the host for A & B and initialize with random values.
                 ******************************************************************************************************************/
                float A_host[N]; /* allocate A[N] on the host */
                float B_host[N]; /* allocate B[N] on the host */
                for (int i = 0; i < N; i++) { /* fill A with arbitrary data */
                        A_host[i] = rand();
                        B_host[i] = rand();
                }

                /*****************************************************************************************************************
                 * Copy A & B from host onto device DRAM.
                 ******************************************************************************************************************/
                void *dst = (void *) ((intptr_t) A_device);
                void *src = (void *) &A_host[0];
                BSG_CUDA_CALL(hb_mc_device_memcpy (&device, dst, src, N * sizeof(float), HB_MC_MEMCPY_TO_DEVICE)); /* Copy A to the device  */

                dst = (void *) ((intptr_t) B_device);
                src = (void *) &B_host[0];
                BSG_CUDA_CALL(hb_mc_device_memcpy (&device, dst, src, N * sizeof(float), HB_MC_MEMCPY_TO_DEVICE)); /* Copy B to the device */

                /*****************************************************************************************************************
                 * Define block_size_x/y: amount of work for each tile group
                 * Define tg_dim_x/y: number of tiles in each tile group
                 * Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
                 ******************************************************************************************************************/
                int block_size = 64;
                hb_mc_dimension_t tg_dim = { .x = 16, .y = 8 };
                hb_mc_dimension_t grid_dim = { .x = 1, .y = 1 };

                /*****************************************************************************************************************
                 * Prepare list of input arguments for kernel.
                 ******************************************************************************************************************/
                int cuda_argv[5] = {A_device, B_device, C_device, N, block_size};

                /*****************************************************************************************************************
                 * Enquque grid of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
                 ******************************************************************************************************************/
                BSG_CUDA_CALL(hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, test_name, 5, cuda_argv));

                /*****************************************************************************************************************
                 * Launch and execute all tile groups on device and wait for all to finish.
                 ******************************************************************************************************************/
                BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));

                /*****************************************************************************************************************
                 * Copy result matrix back from device DRAM into host memory.
                 ******************************************************************************************************************/
                float C_host[N];
                src = (void *) ((intptr_t) C_device);
                dst = (void *) &C_host[0];
                BSG_CUDA_CALL(hb_mc_device_memcpy (&device, (void *) dst, src, N * sizeof(float), HB_MC_MEMCPY_TO_HOST)); /* copy C to the host */

                /*****************************************************************************************************************
                 * Freeze the tiles and memory manager cleanup.
                 ******************************************************************************************************************/
                BSG_CUDA_CALL(hb_mc_device_program_finish(&device));

                /*****************************************************************************************************************
                 * Calculate the expected result using host code and compare the results.
                 ******************************************************************************************************************/
                float C_expected[N];
                host_vec_add (A_host, B_host, C_expected, N);


                int mismatch = 0;
                for (int i = 0; i < N; i++) {
                        if (A_host[i] + B_host[i] != C_host[i]) {
                                bsg_pr_err(BSG_RED("Mismatch: ") "C[%d]:  %f + %f = %f \t Expected: %f\n",
                                           i , A_host[i], B_host[i], C_host[i], C_expected[i]);
                                mismatch = 1;
                        }
                }

                if (mismatch) {
                        return HB_MC_FAIL;
                }
        }

        BSG_CUDA_CALL(hb_mc_device_finish(&device));

        return HB_MC_SUCCESS;
}

#ifdef VCS
int vcs_main(int argc, char ** argv)
#else
int main(int argc, char ** argv)
#endif
{
        bsg_pr_test_info("test_vec_add_parallel Regression Test \n");
        int rc = kernel_energy_ubmark(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}


