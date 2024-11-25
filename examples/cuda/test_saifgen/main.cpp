// Copyright (c) 2020, University of Washington All rights reserved.
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

#include <bsg_manycore_cuda.h>
#include <bsg_manycore_regression.h>

#define ALLOC_NAME "default_allocator"

int test_saifgen (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running CUDA test_saifgen\n");

        // Initialize device, load binary and unfreeze tiles.
        hb_mc_device_t device;
        BSG_CUDA_CALL(hb_mc_device_init(&device, test_name, HB_MC_DEVICE_ID));

        // Initialize the device with a kernel file
        BSG_CUDA_CALL(hb_mc_device_program_init(&device, bin_path, "default_allocator", 0));

        hb_mc_dimension_t dev_dim = hb_mc_config_get_dimension_vcore(hb_mc_manycore_get_config(device.mc));

        hb_mc_dimension_t tg_dim = { .x = bsg_tiles_X, .y = bsg_tiles_Y };

        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1};

        // Prepare list of input arguments for kernel. See the kernel source
        // file for the argument uses.
        uint32_t cuda_argv[1] = {42};

        // Enquque grid of tile groups, pass in grid and tile group dimensions,
        // kernel name, number and list of input arguments
        BSG_CUDA_CALL(hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_saifgen", 0, cuda_argv));

        // Launch and execute all tile groups on device and wait for all to
        // finish.
        uint64_t cycle_start, cycle_end;
        hb_mc_manycore_get_cycle((&device)->mc, &cycle_start);

        BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));

        hb_mc_manycore_get_cycle((&device)->mc, &cycle_end);
        
        bsg_pr_info("\n\n====== EXECUTION STATISTICS ====== \n");
        bsg_pr_info("Cycles: %d\n", cycle_end-cycle_start);
        bsg_pr_info("====== END EXECUTION STATISTICS ====== \n\n\n");

        // Freeze the tiles and memory manager cleanup.
        BSG_CUDA_CALL(hb_mc_device_finish(&device));

        return HB_MC_SUCCESS;
}

declare_program_main("test_saifgen", test_saifgen);
