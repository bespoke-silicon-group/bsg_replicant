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
#include <bsg_manycore_regression.h>

#define ALLOC_NAME "default_allocator"
#define Index3D(_nx,_ny,_i,_j,_k) ((_i)+_nx*((_j)+_ny*(_k)))


void host_jacobi(int c0, int c1, float *A0, float * Anext,
                 const int nx, const int ny, const int nz)
{

	for(int i=1;i<nx-1;i++)
	{
		for(int j=1;j<ny-1;j++)
		{
			for(int k=1;k<nz-1;k++)
			{
				Anext[Index3D (nx, ny, i, j, k)] =
				(A0[Index3D (nx, ny, i, j, k + 1)] +
				A0[Index3D (nx, ny, i, j, k - 1)] +
				A0[Index3D (nx, ny, i, j + 1, k)] +
				A0[Index3D (nx, ny, i, j - 1, k)] +
				A0[Index3D (nx, ny, i + 1, j, k)] +
				A0[Index3D (nx, ny, i - 1, j, k)]) * c1
				- A0[Index3D (nx, ny, i, j, k)] * c0;
			}
		}
	}

}

void DMA_host_to_device(hb_mc_device_t* device, void* dst,
                        const void* src, uint32_t nbytes) {
  hb_mc_dma_htod_t job = {.d_addr=(eva_t)((intptr_t)dst), .h_addr=src, .size=nbytes};
  BSG_CUDA_CALL(hb_mc_device_dma_to_device(device, &job, 1));
}

void DMA_device_to_host(hb_mc_device_t* device, void* dst,
                        const void* src, uint32_t nbytes) {
  hb_mc_dma_dtoh_t job = {.d_addr=(eva_t)((intptr_t)src), .h_addr=dst, .size=nbytes};
  BSG_CUDA_CALL(hb_mc_device_dma_to_host(device, &job, 1));
}

int kernel_jacobi (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Jacobi Kernel on %dx%d tile groups.\n\n", TILE_GROUP_DIM_Y, TILE_GROUP_DIM_X);

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
                 * Allocate memory on the device for A0 and Anext
                 ******************************************************************************************************************/
                // JACOBI:
                // input 512 x 512 x 64
                // this impl distribute Ny and Nz along tileX and tileY
                // it reads 64 Nx values at a time and with a step of 62
                // 560 x 18 x 10 gives you 1 / (32*8) of the work
                int32_t Nx = 560;
                int32_t Ny = 18;
                int32_t Nz = 10;
                int32_t N  = Nx * Ny * Nz;

                eva_t A0_device, Anext_device;
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, N * sizeof(float), &A0_device));    /* allocate A0 on the device */
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, N * sizeof(float), &Anext_device)); /* allocate Anext on the device */

                /*****************************************************************************************************************
                 * Allocate memory on the host for A & B and initialize with random values.
                 ******************************************************************************************************************/
                float A0_host[N];    /* allocate A0 on the host */
                float Anext_host[N]; /* allocate Anext on the host */
                for (int i = 0; i < N; i++) { /* fill A0 with arbitrary data */
                        A0_host[i] = (float)(i % 32);
                }

                /*****************************************************************************************************************
                 * Copy A0 from host onto device DRAM.
                 ******************************************************************************************************************/
                void *dst = (void *) ((intptr_t) A0_device);
                void *src = (void *) &A0_host[0];
                //DMA_host_to_device(&device, dst, src, N * sizeof(float)); /* Copy A0 to the device  */
                BSG_CUDA_CALL(hb_mc_device_memcpy (&device, dst, src, N * sizeof(float), HB_MC_MEMCPY_TO_DEVICE));
                dst = (void *) ((intptr_t) Anext_device);
                src = (void *) &Anext_host[0];
                //BSG_CUDA_CALL(hb_mc_device_memcpy (&device, dst, src, N * sizeof(float), HB_MC_MEMCPY_TO_DEVICE));

                /*****************************************************************************************************************
                 * Define block_size_x/y: amount of work for each tile group
                 * Define tg_dim_x/y: number of tiles in each tile group
                 * Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
                 ******************************************************************************************************************/
                hb_mc_dimension_t tg_dim = { .x = 16, .y = 8};
                hb_mc_dimension_t grid_dim = { .x = 1, .y = 1};

                /*****************************************************************************************************************
                 * Prepare list of input arguments for kernel.
                 ******************************************************************************************************************/
                int c0 = 2;
                int c1 = 4;

                int cuda_argv[7] = {c0, c1, A0_device, Anext_device, Nx, Ny, Nz};

                /*****************************************************************************************************************
                 * Enquque grid of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
                 ******************************************************************************************************************/
                BSG_CUDA_CALL(hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_jacobi", 7, cuda_argv));

                /*****************************************************************************************************************
                 * Launch and execute all tile groups on device and wait for all to finish.
                 ******************************************************************************************************************/
                BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));

                /*****************************************************************************************************************
                 * Copy result matrix back from device DRAM into host memory.
                 ******************************************************************************************************************/
                src = (void *) ((intptr_t) Anext_device);
                dst = (void *) &Anext_host[0];
                DMA_device_to_host(&device, dst, src, N * sizeof(float)); /* copy C to the host */

                /*****************************************************************************************************************
                 * Freeze the tiles and memory manager cleanup.
                 ******************************************************************************************************************/
                BSG_CUDA_CALL(hb_mc_device_program_finish(&device));

                /*****************************************************************************************************************
                 * Calculate the expected result using host code and compare the results.
                 ******************************************************************************************************************/
                float Anext_expected[N];
                for (int i = 0; i < N; i++) { // fill with 0
                  Anext_expected[i] = 0;
                }
                host_jacobi(c0, c1, A0_host, Anext_expected, Nx, Ny, Nz);

                bsg_pr_test_info("end of test ... checking ... \n\n");

                int mismatch = 0;
                for (int i = 0; i < N; i++) {
                        if (Anext_host[i] != Anext_expected[i]) {
                                printf("idx %d computed = %f -- expecting = %f\n", i, Anext_host[i], Anext_expected[i]);
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

declare_program_main("test_jacobi", kernel_jacobi);
