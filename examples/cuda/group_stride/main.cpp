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

//#define BLOCK_DIM 16 // this block dim needs to match the same marco in the riscv binary
#define ALLOC_NAME "default_allocator"

int kernel_group_stride (int argc, char **argv) {
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

        hb_mc_dimension_t tg_dim = { .x = dev_dim.x, .y = dev_dim.y };

        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1};


        //************************************************************
        // Allocate memory on the device for nx and ny
        //************************************************************
        int nx[tg_dim.x * tg_dim.y], ny[tg_dim.x * tg_dim.y];

        bsg_pr_info("Tile Group Dimensions: %d x %d \n", tg_dim.x, tg_dim.y);

        eva_t _nx, _ny;
        rc = hb_mc_device_malloc(&device, sizeof(nx), &_nx);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("Failed to allocate nx on device.\n");
                return rc;
        }

        rc = hb_mc_device_malloc(&device, sizeof(ny), &_ny);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("Failed to allocate ny on device.\n");
                return rc;
        }


        //************************************************************
        // Copy nx and ny, from host onto device
        //************************************************************
        void *dst, *src;

        bsg_pr_info("Copying nx\n");
        dst = (void *) ((intptr_t) _nx);
        src = (void *) &nx;
        rc = hb_mc_device_memcpy (&device, dst, src, sizeof(nx), HB_MC_MEMCPY_TO_DEVICE);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("Failed to copy nx to device.\n");
                return rc;
        }

        bsg_pr_info("Copying ny\n");
        dst = (void *) ((intptr_t) _ny);
        src = (void *) &ny;
        rc = hb_mc_device_memcpy (&device, dst, src, sizeof(ny), HB_MC_MEMCPY_TO_DEVICE);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("Failed to copy ny to device.\n");
                return rc;
        }


        //************************************************************
        // Prepare list of arguments for kernel.
        //************************************************************
        uint32_t cuda_argv[2] = {_nx, _ny};

        //************************************************************
        // Enquque grid of tile groups, pass in grid and tile group
        // dimensions, kernel name, number and list of arguments
        //************************************************************
        bsg_pr_info("Enqueue Kernel\n");
        rc = hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_group_stride", sizeof(cuda_argv)/sizeof(cuda_argv[0]), cuda_argv);
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
        src = (void *) ((intptr_t) _nx);
        dst = (void *) nx;
        rc = hb_mc_device_memcpy (&device, dst, src, sizeof(nx), HB_MC_MEMCPY_TO_HOST);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("failed to copy memory from device.\n");
                return rc;
        }

        bsg_pr_info("Copying result back\n");
        src = (void *) ((intptr_t) _ny);
        dst = (void *) ny;
        rc = hb_mc_device_memcpy (&device, dst, src, sizeof(ny), HB_MC_MEMCPY_TO_HOST);
        if (rc != HB_MC_SUCCESS) {
                bsg_pr_err("failed to copy memory from device.\n");
                return rc;
        }

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
        for(int ix = 0; ix < tg_dim.x; ++ix){
                for(int iy = 0; iy < tg_dim.y; ++iy){
                        if(nx[tg_dim.x * iy + ix] != ((ix + 1) % tg_dim.x)){
                                bsg_pr_err(BSG_RED("Incorrect east neighbor for tile X: %d, Y: %d. Expected: %d, Got: %d\n"), ix, iy, ((ix + 1) % tg_dim.x), nx[tg_dim.x * iy + ix]);
                                return HB_MC_FAIL;
                        }
                        if(ny[tg_dim.x * iy + ix] != ((iy + 1) % tg_dim.y)){
                                bsg_pr_err(BSG_RED("Incorrect south neighbor for tile X: %d, Y: %d. Expected: %d, Got: %d\n"), ix, iy, ((iy + 1) % tg_dim.y), ny[tg_dim.x * iy + ix]);
                                return HB_MC_FAIL;
                        }
                }
        }

        bsg_pr_test_info(BSG_GREEN("Pass\n"));

        bsg_pr_info("\n\n====== EXECUTION STATISTICS ====== \n");
        bsg_pr_info("Cycles: %d\n", cycle_end-cycle_start);
        bsg_pr_info("====== END EXECUTION STATISTICS ====== \n\n\n");

        return HB_MC_SUCCESS;
}

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
        int main(int argc, char ** argv) {
#endif
                bsg_pr_test_info("Tile Group Striding Regression Test\n");
                int rc = kernel_group_stride(argc, argv);
                bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
                return rc;
        }


