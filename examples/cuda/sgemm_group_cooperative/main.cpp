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

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <random>
#include <limits>
#include <bsg_manycore_cuda.h>
#include <bsg_manycore_regression.h>
#include <bsg_manycore_eva.h>

#define ALLOC_NAME "default_allocator"

typedef struct {
        uint32_t N;    // Number of elements in the tensor
        uint32_t dims; // Number of dimensions
        eva_t strides; // Pointer to stride vector; Number of elements to skip between indexes in a dimension
        eva_t sizes;   // Pointer to sizes vector; Number of elements in each dimension
        eva_t data;    // Pointer to raw data
} hb_mc_device_tensor_t;

template <typename T>
struct hb_mc_host_tensor_t{
        uint32_t N;    // Number of elements in the tensor
        uint32_t dims; // Number of dimensions
        uint32_t *strides; // Pointer to stride vector; Number of elements
        uint32_t *sizes;   // Pointer to sizes vector; Number of elements in each dimension
        T *data;    // Pointer to raw data
};

void host_mm_opt(hb_mc_host_tensor_t<float> *result,
                hb_mc_host_tensor_t<float> *mat1,
                hb_mc_host_tensor_t<float> *mat2) {

        for (unsigned int y = 0; y < result->sizes[0]; ++y) {
                for (unsigned int x = 0; x < result->sizes[1]; ++x) {
                        float res = 0;
                        for (unsigned int k = 0; k < mat1->sizes[1]; ++k) {
                                res += mat1->data[mat1->strides[0] * y + k] * mat2->data[mat2->strides[0] * k + x];
                        }

                        result->data[y * result->strides[0] + x] = res;
                }
        }
        return;
}

int kernel_matrix_mul (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        //
        // Define path to binary.
        // Initialize device, load binary and unfreeze tiles.
        //
        hb_mc_device_t device;
        rc = hb_mc_device_init(&device, test_name, 0);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("Failed to initialize device.\n");
                return rc;
        }


        rc = hb_mc_device_program_init(&device, bin_path, ALLOC_NAME, 0);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("Failed to initialize program.\n");
                return rc;
        }

        //************************************************************
        // Define tg_dim_x/y: number of tiles in each tile group
        // Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
        //************************************************************
        hb_mc_dimension_t dev_dim = hb_mc_config_get_dimension_vcore(hb_mc_manycore_get_config(device.mc));

        hb_mc_dimension_t tg_dim = { .x = bsg_tiles_X, .y = bsg_tiles_Y };

        hb_mc_dimension_t grid_dim = { .x = dev_dim.x/tg_dim.x, .y = dev_dim.y/tg_dim.y};


        //************************************************************
        // Allocate memory on the device for mat1, mat2, and out
        //************************************************************
        hb_mc_host_tensor_t<float> Hmat1, Hmat2, Hout, Hresult;

        // 256 x 256 x 256
        uint32_t M = BLOCK_DIM * dev_dim.y;
        uint32_t N = BLOCK_DIM * 16;
        uint32_t P = BLOCK_DIM * dev_dim.x;

        /*
        // 768 x 768 x 768
        uint32_t M = BLOCK_DIM * dev_dim.y * 6;
        uint32_t N = BLOCK_DIM * 48;
        uint32_t P = BLOCK_DIM * dev_dim.x * 3;
        */

        /*
        // 64 x 32 x 512
        uint32_t M = BLOCK_DIM * dev_dim.y;
        uint32_t N = BLOCK_DIM * 4;
        uint32_t P = BLOCK_DIM * dev_dim.x * 4;
        */

        bsg_pr_info("Matrix Dimensions: %d x %d x %d \n", M, N, P);

        eva_t _mat2, _out, _mat1;
        hb_mc_device_tensor_t mat1, mat2, out;
        size_t s;
        // Set up Tensor Metadata (Host and Device)
        // N = Number of Elements, dims = number of dimensions
        Hmat1.N = mat1.N = M * N;
        Hmat1.dims = mat1.dims = 2;

        Hmat2.N = mat2.N = N * P;
        Hmat2.dims = mat2.dims = 2;

        Hresult.N = Hout.N = out.N = M * P;
        Hresult.dims = Hout.dims = out.dims = 2;

        // Construct mat1
        rc = hb_mc_device_malloc(&device, sizeof(hb_mc_device_tensor_t), &_mat1);
        rc |= hb_mc_device_malloc(&device, mat1.dims * sizeof(uint32_t), &mat1.strides);
        rc |= hb_mc_device_malloc(&device, mat1.dims * sizeof(uint32_t), &mat1.sizes);
        // Allocate twice the size for alignment
        s = (mat1.N    * sizeof(*Hmat1.data));
        rc |= hb_mc_device_malloc(&device, 2 * s, &mat1.data);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("Failed to allocate mat1 on device.\n");
                return rc;
        }

        // Align mat1 to itself
        mat1.data = (s - (mat1.data % s)) + mat1.data;

        // Construct mat2
        rc = hb_mc_device_malloc(&device, sizeof(hb_mc_device_tensor_t), &_mat2);
        rc |= hb_mc_device_malloc(&device, mat2.dims * sizeof(uint32_t), &mat2.strides);
        rc |= hb_mc_device_malloc(&device, mat2.dims * sizeof(uint32_t), &mat2.sizes);
        // Allocate twice the size for alignment
        s = (mat2.N    * sizeof(*Hmat2.data));
        rc |= hb_mc_device_malloc(&device, 2 * s, &mat2.data);

        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("Failed to allocate mat2 on device.\n");
                return rc;
        }

        // Align mat2 to itself
        mat2.data = (s - (mat2.data % s)) + mat2.data;

        // Construct out
        rc = hb_mc_device_malloc(&device, sizeof(hb_mc_device_tensor_t), &_out);
        rc |= hb_mc_device_malloc(&device, out.dims * sizeof(uint32_t), &out.strides);
        rc |= hb_mc_device_malloc(&device, out.dims * sizeof(uint32_t), &out.sizes);
        // Allocate twice the size for alignment
        s = (out.N    * sizeof(*Hout.data));
        rc |= hb_mc_device_malloc(&device, 2 * s, &out.data);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("Failed to allocate out on device.\n");
                return rc;
        }

        // Align out to itself
        out.data = (s - (out.data % s)) + out.data;

        // Initialize RNG
        // Initialize the random number generators
        std::numeric_limits<float> lim;

        std::mt19937 generator;
        generator.seed(42);
        std::uniform_real_distribution<float> distribution(-1,1);

        //************************************************************
        // Allocate memory on the host for mat1, mat2, and result
        // and initialize the first three with random values.
        //************************************************************
        Hmat1.data = new float[Hmat1.N];
        Hmat1.sizes = new uint32_t[Hmat1.dims];
        Hmat1.sizes[0] = M;
        Hmat1.sizes[1] = N;
        Hmat1.strides = new uint32_t[Hmat1.dims];
        Hmat1.strides[0] = Hmat1.sizes[1];
        Hmat1.strides[1] = 1;
        for (unsigned int y = 0; y < Hmat1.sizes[0]; ++y) {
                for (unsigned int x = 0; x < Hmat1.sizes[1]; ++x) {
                        if(x == y)
                                Hmat1.data[y * Hmat1.strides[0] + x] = 1;
                }
        }

        Hmat2.data = new float[Hmat2.N];
        Hmat2.sizes = new uint32_t[Hmat2.dims];
        Hmat2.sizes[0] = N;
        Hmat2.sizes[1] = P;
        Hmat2.strides = new uint32_t[Hmat2.dims];
        Hmat2.strides[0] = Hmat2.sizes[1];
        Hmat2.strides[1] = 1;
        for (uint64_t i = 0; i < Hmat2.N; ++i) {
                Hmat2.data[i] = i;
                //Hmat2.data[i] = distribution(generator);
        }

        Hout.data = new float[Hout.N];
        Hout.sizes = new uint32_t[Hout.dims];
        Hout.sizes[0] = M;
        Hout.sizes[1] = P;
        Hout.strides = new uint32_t[Hout.dims];
        Hout.strides[0] = Hout.sizes[1];
        Hout.strides[1] = 1;

        Hresult.data = new float[Hresult.N];
        Hresult.sizes = new uint32_t[Hresult.dims];
        Hresult.sizes[0] = M;
        Hresult.sizes[1] = P;
        Hresult.strides = new uint32_t[Hresult.dims];
        Hresult.strides[0] = Hresult.sizes[1];
        Hresult.strides[1] = 1;

        //************************************************************
        // Copy mat1 and mat2, from host onto device
        //************************************************************
        void *dst, *src;
        hb_mc_dma_htod_t htod;
        hb_mc_dma_dtoh_t dtoh;

        // Copy mat1
        bsg_pr_info("Copying mat1\n");
        dst = (void *) ((intptr_t) _mat1);
        src = (void *) &mat1;
        rc = hb_mc_device_memcpy (&device, dst, src, sizeof(mat1), HB_MC_MEMCPY_TO_DEVICE);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("Failed to copy mat1 to device.\n");
                return rc;
        }

        dst = (void *) ((intptr_t) mat1.strides);
        src = (void *) Hmat1.strides;
        rc = hb_mc_device_memcpy (&device, dst, src, Hmat1.dims * sizeof(*Hmat1.strides), HB_MC_MEMCPY_TO_DEVICE);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("Failed to copy mat1.strides to device.\n");
                return rc;
        }

        dst = (void *) ((intptr_t) mat1.sizes);
        src = (void *) Hmat1.sizes;
        rc = hb_mc_device_memcpy (&device, dst, src, Hmat1.dims * sizeof(*Hmat1.sizes), HB_MC_MEMCPY_TO_DEVICE);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("Failed to copy mat1.sizes to device.\n");
                return rc;
        }

        dst = (void *) ((intptr_t) mat1.data);
        src = (void *) Hmat1.data;

        /*
        rc = hb_mc_device_memcpy (&device, dst, src, Hmat1.N * sizeof(*Hmat1.data), HB_MC_MEMCPY_TO_DEVICE);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("Failed to copy mat1.data to device.\n");
                return rc;
        }*/

        htod.d_addr = mat1.data;
        htod.h_addr = src;
        htod.size   = Hmat1.N * sizeof(*Hmat1.data);

        BSG_CUDA_CALL(hb_mc_device_dma_to_device(&device, &htod, 1));

        // Copy mat2
        bsg_pr_info("Copying mat2\n");
        dst = (void *) ((intptr_t) _mat2);
        src = (void *) &mat2;
        rc = hb_mc_device_memcpy (&device, dst, src, sizeof(mat2), HB_MC_MEMCPY_TO_DEVICE);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("Failed to copy _mat2 to device.\n");
                return rc;
        }

        dst = (void *) ((intptr_t) mat2.strides);
        src = (void *) Hmat2.strides;
        rc = hb_mc_device_memcpy (&device, dst, src, Hmat2.dims * sizeof(*Hmat2.strides), HB_MC_MEMCPY_TO_DEVICE);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("Failed to copy mat2.strides to device.\n");
                return rc;
        }

        dst = (void *) ((intptr_t) mat2.sizes);
        src = (void *) Hmat2.sizes;
        rc = hb_mc_device_memcpy (&device, dst, src, Hmat2.dims * sizeof(*Hmat2.sizes), HB_MC_MEMCPY_TO_DEVICE);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("Failed to copy mat2.sizes to device.\n");
                return rc;
        }

        dst = (void *) ((intptr_t) mat2.data);
        src = (void *) Hmat2.data;
        /*
        rc = hb_mc_device_memcpy (&device, dst, src, Hmat2.N * sizeof(*Hmat2.data), HB_MC_MEMCPY_TO_DEVICE);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("Failed to copy mat2.data to device.\n");
                return rc;
        }*/

        htod.d_addr = mat2.data;
        htod.h_addr = src;
        htod.size   = Hmat2.N * sizeof(*Hmat2.data);

        BSG_CUDA_CALL(hb_mc_device_dma_to_device(&device, &htod, 1));


        // Copy out
        bsg_pr_info("Copying out\n");
        dst = (void *) ((intptr_t) _out);
        src = (void *) &out;
        rc = hb_mc_device_memcpy (&device, dst, src, sizeof(out), HB_MC_MEMCPY_TO_DEVICE);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("Failed to copy _out to device.\n");
                return rc;
        }

        dst = (void *) ((intptr_t) out.strides);
        src = (void *) Hout.strides;
        rc = hb_mc_device_memcpy (&device, dst, src, Hout.dims * sizeof(*Hout.strides), HB_MC_MEMCPY_TO_DEVICE);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("Failed to copy out.strides to device.\n");
                return rc;
        }

        dst = (void *) ((intptr_t) out.sizes);
        src = (void *) Hout.sizes;
        rc = hb_mc_device_memcpy (&device, dst, src, Hout.dims * sizeof(*Hout.sizes), HB_MC_MEMCPY_TO_DEVICE);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("Failed to copy out.sizes to device.\n");
                return rc;
        }

        // Copying data is not necessary
        /*
          dst = (void *) ((intptr_t) out.data);
          src = (void *) Hout.data;
          rc = hb_mc_device_memcpy (&device, dst, src, Hout.N * sizeof(*Hout.data), HB_MC_MEMCPY_TO_DEVICE);
          if (rc != HB_MC_SUCCESS) {
          bsg_pr_err("Failed to copy out.data to device.\n");
          return rc;
          }
        */

        //************************************************************
        // Prepare list of mat1 arguments for kernel.
        //************************************************************
        uint32_t cuda_argv[3] = {_out, _mat1, _mat2};

        //************************************************************
        // Enquque grid of tile groups, pass in grid and tile group
        // dimensions, kernel name, number and list of mat1 arguments
        //************************************************************
        bsg_pr_info("Enqueue Kernel\n");
        rc = hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_mm_opt", sizeof(cuda_argv)/sizeof(cuda_argv[0]), cuda_argv);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to initialize grid.\n");
                return rc;
        }


        //************************************************************
        // Launch and execute all tile groups on device and wait for all to finish.
        //************************************************************
        bsg_pr_info("Execute Kernel\n");

        uint64_t cycle_start, cycle_end;
        hb_mc_manycore_get_cycle((&device)->mc, &cycle_start);

        rc = hb_mc_device_tile_groups_execute(&device);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("failed to execute tile groups.\n");
                return rc;
        }

        hb_mc_manycore_get_cycle((&device)->mc, &cycle_end);

        //************************************************************
        // Copy result matrix back from device DRAM into host memory.
        //************************************************************
        bsg_pr_info("Copying result back\n");
        src = (void *) ((intptr_t) out.data);
        dst = (void *) Hout.data;
        /*
        rc = hb_mc_device_memcpy (&device, dst, src, Hout.N * sizeof(*Hout.data), HB_MC_MEMCPY_TO_HOST);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("failed to copy memory from device.\n");
                return rc;
        }*/

        
        dtoh.d_addr = out.data;
        dtoh.h_addr = dst;
        dtoh.size   = Hout.N * sizeof(*Hout.data);

        BSG_CUDA_CALL(hb_mc_device_dma_to_host(&device, &dtoh, 1));


        //************************************************************
        // Freeze the tiles and memory manager cleanup.
        //************************************************************
        rc = hb_mc_device_finish(&device);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("failed to de-initialize device.\n");
                return rc;
        }


        //************************************************************
        // Calculate the expected result matrix using host code and
        // compare the results.
        // ************************************************************
        host_mm_opt(&Hresult, &Hmat1, &Hmat2);

        double sse = 0;
        for(unsigned int i = 0; i < Hresult.N; ++i){
                sse += (Hresult.data[i] - Hout.data[i]) * (Hresult.data[i] - Hout.data[i]);
        }
        /*
        for (int y = 0; y < Hresult.sizes[0]; ++y) {
                for (int x = 0; x < Hresult.sizes[1]; ++x) {
                        printf("%4.0f ", Hout.data[y * Hout.strides[0] + x]);
                }
                printf("\n");
        }

                bsg_pr_info("\n");
        for (int y = 0; y < Hresult.sizes[0]; ++y) {
                for (int x = 0; x < Hresult.sizes[1]; ++x) {
                        printf("%4.0f ", Hresult.data[y * Hresult.strides[0] + x]);
                }
                printf("\n");
                }*/

        if (sse >= .01) {
                bsg_pr_err(BSG_RED("Matrix Mismatch.(SSE: %f)\n"), sse);
                return HB_MC_FAIL;
        }
        bsg_pr_test_info(BSG_GREEN("Matrix Match. (SSE: %f)\n"), sse);

        bsg_pr_info("\n\n====== EXECUTION STATISTICS ====== \n");
        bsg_pr_info("Cycles: %lu\n", cycle_end-cycle_start);
        bsg_pr_info("====== END EXECUTION STATISTICS ====== \n\n\n");

        return HB_MC_SUCCESS;
}

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
        int main(int argc, char ** argv) {
#endif
                bsg_pr_test_info("test_matrix_mul Regression Test\n");
                int rc = kernel_matrix_mul(argc, argv);
                bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
                return rc;
        }


