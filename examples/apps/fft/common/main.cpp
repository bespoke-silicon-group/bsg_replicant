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
#include <math.h>
#include <complex.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <bsg_manycore_regression.h>

#define ALLOC_NAME "default_allocator"

// Do a 256x256 point FFT using four-step method

// Generated input: cos(n*pi/8)
// This signal is known to have only two pulses of magnitude N/2 at N/16 and
// 15*N/16
#define LOCAL_POINTS (256)
/* #define NUM_POINTS 16 */
/* #define NUM_POINTS 256 */
#define NUM_POINTS (LOCAL_POINTS*LOCAL_POINTS)

// When NUM_POINTS is large the FP error will be huge
#define PRECISION (15)

/*!
 * Runs FFT.
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/fft/
 * Manycore binary in the BSG Manycore bitbucket repository.  
*/

int is_close(float complex n, float complex r) {
    double nr = crealf(n), ni = cimagf(n);
    double rr = crealf(r), ri = cimagf(r);
    double dr = nr-rr, di = ni-ri;
    double dist = sqrt(dr*dr+di*di);
    if (dist < PRECISION)
        return 1;
    else
        return 0;
}


int verify_fft (float complex *out, int N) { 
    int r  = N / 16;
    int ar = N - r;
    for (int i = 0; i < N; i++) {
        float rr = crealf(out[i]), ii = cimagf(out[i]);
        bsg_pr_test_info("%d-th result is %.6f+%.6fi (0x%08X 0x%08X)\n", i, rr, ii, *(uint32_t*)&rr, *(uint32_t*)&ii);
    }
    for (int i = 0; i < N; i++) {
        double complex ref = 0.0;
        bsg_pr_test_info("%d-th component is %.3f+%.3fi\n", i, crealf(out[i]), cimagf(out[i]));
        if ((i == r) || (i == ar)) {
            ref = N/2.0;
        } else {
            ref = 0.0;
        }
        if (!is_close(out[i], ref)) {
            bsg_pr_err(BSG_RED("Mismatch: ") "out[%d]: %.3f+%.3fi; ref is %.3f+%.3fi",
                       i , crealf(out[i]), cimagf(out[i]), crealf(ref), cimagf(ref));
            return 1;
        }
    }
    return 0;
}


int kernel_fft_256x256_all_no_twiddle (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA FFT Kernel on one 8x16 tile group.\n\n");

        srand(time); 

        /*****************************************************************************************************************
        * Define path to binary.
        * Initialize device, load binary and unfreeze tiles.
        ******************************************************************************************************************/
        hb_mc_device_t device;
        BSG_CUDA_CALL(hb_mc_device_init(&device, test_name, 0));

        hb_mc_pod_id_t pod;
        hb_mc_device_foreach_pod_id(&device, pod)
        {
                bsg_pr_info("Loading program for test %s onto pod %d\n", test_name, pod);
                BSG_CUDA_CALL(hb_mc_device_set_default_pod(&device, pod));
                BSG_CUDA_CALL(hb_mc_device_program_init(&device, bin_path, ALLOC_NAME, 0));

                /*****************************************************************************************************************
                 * Allocate memory on the device for A and B
                 ******************************************************************************************************************/

                uint32_t N = NUM_POINTS;

                eva_t A_device, B_device, TW_device;
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, N * sizeof(float complex), &A_device)); /* allocate A[N] on the device */
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, N * sizeof(float complex), &B_device)); /* allocate B[N] on the device */
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, N * sizeof(float complex), &TW_device)); /* allocate B[N] on the device */

                /*****************************************************************************************************************
                 * Allocate memory on the host for A and initialize with cosine values
                 ******************************************************************************************************************/

                float complex A_host[N]; /* allocate A[N] on the host */
                for (int i = 0; i < N; i++) { /* fill A with arbitrary data */
                        A_host[i] = cosf(i*M_PI/8.0);
                }

                for (int i = 0; i < N; i++) {
                    float rr = crealf(A_host[i]), ii = cimagf(A_host[i]);
                    /* bsg_pr_info("%d-th item is %.3f+%.3fi (0x%08X 0x%08X)\n", i, rr, ii, *(uint32_t*)&rr, *(uint32_t*)&ii); */
                }

                float complex TW_host[N];

                for (int r = 0; r < LOCAL_POINTS; r++) {
                    for (int c = 0; c < LOCAL_POINTS; c++) {
                        float ref_sinf = sinf(-2.0f*M_PI*(float)(r*c)/(float)(N));
                        float ref_cosf = cosf(-2.0f*M_PI*(float)(r*c)/(float)(N));
                        TW_host[r*LOCAL_POINTS+c] = ref_cosf + I*ref_sinf;
                    }
                }

                /*****************************************************************************************************************
                 * Copy A from host onto device DRAM.
                 ******************************************************************************************************************/

                /* void *dst = (void *) ((intptr_t) A_device); */
                /* void *src = (void *) &A_host[0]; */

                hb_mc_dma_htod_t htod_A_job [] = {
                    {
                        .d_addr = A_device,
                        .h_addr = (void *) &A_host[0],
                        .size   = N * sizeof(float complex)
                    },
                    {
                        .d_addr = TW_device,
                        .h_addr = (void *) &TW_host[0],
                        .size   = N * sizeof(float complex)
                    }
                };

                BSG_CUDA_CALL(hb_mc_device_dma_to_device(&device, htod_A_job, 2));
                /* BSG_CUDA_CALL(hb_mc_device_memcpy (&device, dst, src, N * sizeof(float complex), HB_MC_MEMCPY_TO_DEVICE)); /1* Copy A to the device  *1/ */

                /*****************************************************************************************************************
                 * Define block_size_x/y: amount of work for each tile group
                 * Define tg_dim_x/y: number of tiles in each tile group
                 * Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
                 ******************************************************************************************************************/

                // NOTE: If you change NUM_POINTS you need to update the tg size accordingly
                hb_mc_dimension_t tg_dim = { .x = 16, .y = 8};
                hb_mc_dimension_t grid_dim = { .x = 1, .y = 1};

                /*****************************************************************************************************************
                 * Prepare list of input arguments for kernel.
                 ******************************************************************************************************************/

                uint32_t cuda_argv[4] = {A_device, B_device, TW_device, N};

                /*****************************************************************************************************************
                 * Enquque grid of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
                 ******************************************************************************************************************/

                BSG_CUDA_CALL(hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_fft_256x256_all_no_twiddle", 4, cuda_argv));

                /*****************************************************************************************************************
                 * Launch and execute all tile groups on device and wait for all to finish.
                 ******************************************************************************************************************/

                BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));

                /*****************************************************************************************************************
                 * Copy result matrix back from device DRAM into host memory.
                 ******************************************************************************************************************/

                float complex B_host[N];
                /* src = (void *) ((intptr_t) B_device); */
                /* dst = (void *) &B_host[0]; */
                /* BSG_CUDA_CALL(hb_mc_device_memcpy (&device, (void *) dst, src, N * sizeof(float complex), HB_MC_MEMCPY_TO_HOST)); /1* copy B to the host *1/ */

                hb_mc_dma_dtoh_t dtoh_A_job = {
                        .d_addr = B_device,
                        .h_addr = (void *) &B_host[0],
                        .size   = N * sizeof(float complex)
                };

                BSG_CUDA_CALL(hb_mc_device_dma_to_host(&device, &dtoh_A_job, 1));

                /*****************************************************************************************************************
                 * Freeze the tiles and memory manager cleanup.
                 ******************************************************************************************************************/

                BSG_CUDA_CALL(hb_mc_device_program_finish(&device));

                /*****************************************************************************************************************
                 * Verify the FFT results
                 ******************************************************************************************************************/

                int mismatch = verify_fft(B_host, N);

                if (mismatch) {
                        return HB_MC_FAIL;
                }
        }
        BSG_CUDA_CALL(hb_mc_device_finish(&device));

        return HB_MC_SUCCESS;
}

declare_program_main("test_fft_256x256_all_no_twiddle", kernel_fft_256x256_all_no_twiddle);
