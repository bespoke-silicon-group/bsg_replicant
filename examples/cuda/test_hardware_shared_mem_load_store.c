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

#include "test_hardware_shared_mem_load_store.h"
#include <string.h>

#define ALLOC_NAME "default_allocator"

/*!
 * Runs a tile group shared memory load/store kernel. Loads a M * N matrix into tile group shared memory and stores it back to another location.
 * Grid dimensions are determines by how much of a load we want for each tile group (block_size_y/x)
 * This tests uses the software/spmd/bsg_cuda_lite_runtime/shared_mem_load_store/ Manycore binary in the BSG Manycore bitbucket repository.  
*/

static hb_mc_device_t device;
static hb_mc_dimension_t gd = HB_MC_DIMENSION(1,1); // test is one tile group
static hb_mc_dimension_t td = HB_MC_DIMENSION(4,4); // test is 4x4
static int *A_host, *B_host;
static int BLOCK_SIZE = 1024;

#define ARRAY_SIZE(x)                           \
    (sizeof(x)/sizeof(x[0]))


int init_A() {
    for (int i = 0; i < BLOCK_SIZE; ++i) {
        A_host[i] = rand();
    }
}


int kernel_shared_mem_load_store (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Shared Memory Load Store Kernel.\n\n");

        //srand(time);
        A_host = malloc(BLOCK_SIZE * sizeof(int));
        B_host = malloc(BLOCK_SIZE * sizeof(int));
        if (A_host == NULL|| B_host == NULL) {
            bsg_pr_test_info("Bad allocation of A and B: %s\n", strerror(errno));
            return HB_MC_FAIL;
        }

        init_A();

        bsg_pr_test_info("Device initialization\n");
        BSG_CUDA_CALL(hb_mc_device_init(&device, test_name, 0));
        BSG_CUDA_CALL(hb_mc_device_program_init(&device, bin_path, test_name, 0));

        hb_mc_eva_t A_dev, B_dev; // vectors on the device
        BSG_CUDA_CALL(hb_mc_device_malloc(&device, BLOCK_SIZE * sizeof(int), &A_dev));
        BSG_CUDA_CALL(hb_mc_device_malloc(&device, BLOCK_SIZE * sizeof(int), &B_dev));
        bsg_pr_test_info("A_dev = %08x, B_dev = %08x\n", A_dev, B_dev);

        hb_mc_dma_htod_t htod = { A_dev, A_host, BLOCK_SIZE * sizeof(int) };
        BSG_CUDA_CALL(hb_mc_device_dma_to_device(&device, &htod, 1));
        
        uint32_t kargv[2];
        kargv[0] = A_dev;
        kargv[1] = B_dev;
        
        uint32_t kargc = ARRAY_SIZE(kargv);
        bsg_pr_test_info("Launching\n");
        BSG_CUDA_CALL(hb_mc_kernel_enqueue(&device, gd, td,
                                           "kernel_hardware_shared_mem_load_store", kargc, kargv));

        BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));
        bsg_pr_test_info("Complete\n");
        hb_mc_dma_dtoh_t dtoh = { B_dev, B_host, BLOCK_SIZE * sizeof(int) };
        BSG_CUDA_CALL(hb_mc_device_dma_to_host(&device, &dtoh, 1));

        if (memcmp(A_host, B_host, BLOCK_SIZE * sizeof(int)) != 0) {
            bsg_pr_test_info("%s: FAILED: A and B do not match\n", test_name);
            return HB_MC_FAIL;
        }
        
        BSG_CUDA_CALL(hb_mc_device_finish(&device));
        
        return HB_MC_SUCCESS;
}

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif
        bsg_pr_test_info("test_shared_mem_load_store Regression Test \n");
        int rc = kernel_shared_mem_load_store(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}


