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

#define ARR_LOG2_NUM_ELEMENTS 15
#define ALLOC_NAME "default_allocator"

#include <bit>
template<unsigned long long BITS>
class LFSR{

        unsigned long long htaps[25] = {
                0x0,// Error
                0x1,
                0x3,
                0x6,
                0xC,
                0x14,
                0x30,
                0x60,
                0xB4,
                0x110,
                0x240,
                0x500,
                0xE08,
                0x1C80,
                0x3802,
                0x6000,
                0xD008,
                0x12000,
                0x20400,
                0x72000,
                0x90000,
                0x140000,
                0x300000,
                0x420000,
                0xE10000};

        unsigned long long taps = htaps[BITS];

public:
        unsigned long long next(unsigned long long input){
                unsigned long long bit = std::popcount(input & taps) & 1u;
                return ((input << 1) | (bit)) & ((1 << BITS)-1) ;
        }
};

template<unsigned long long BITS>
int build_dram_ptrs(eva_t buf, eva_t * ps, size_t sz){
        int nels = sz / sizeof(eva_t);
        if((1<< BITS) > nels){
                bsg_pr_err("Number of elements is not sufficient to encode all LFSR states\n");
                return HB_MC_FAIL;
        }
        LFSR<BITS> rng;
        unsigned long long cur = 1, next;
        
        do {
                next = rng.next(cur);
                ps[cur] = buf + next * sizeof(eva_t);
                cur = next;
        } while(cur != 1);
        return HB_MC_SUCCESS;
}

int kernel_ptr_chase (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        // Define path to binary.
        // Initialize device, load binary and unfreeze tiles.
        hb_mc_device_t device;
        BSG_CUDA_CALL(hb_mc_device_init(&device, test_name, 0));
        BSG_CUDA_CALL(hb_mc_device_program_init(&device, bin_path, ALLOC_NAME, 0));

        //************************************************************
        // Define tg_dim_x/y: number of tiles in each tile group
        // Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
        //************************************************************
        hb_mc_dimension_t dev_dim = hb_mc_config_get_dimension_vcore(hb_mc_manycore_get_config(device.mc));

        hb_mc_dimension_t tg_dim = { .x = dev_dim.x, .y = dev_dim.y };

        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1};

        bsg_pr_info("Tile Group Dimensions: %d x %d \n", tg_dim.x, tg_dim.y);

        //************************************************************
        // Allocate memory on the device for pointers
        //************************************************************
        eva_t *ps = new eva_t[1 << ARR_LOG2_NUM_ELEMENTS];

        eva_t _ps;
        BSG_CUDA_CALL(hb_mc_device_malloc(&device, (1 << ARR_LOG2_NUM_ELEMENTS) * sizeof(eva_t), &_ps));

        BSG_CUDA_CALL(build_dram_ptrs<ARR_LOG2_NUM_ELEMENTS>(_ps, ps, (1 << ARR_LOG2_NUM_ELEMENTS) * sizeof(eva_t)));

        //************************************************************
        // Copy ps, from host onto device
        //************************************************************
        void *dst, *src;

        bsg_pr_info("Copying ps\n");
        dst = (void *) ((intptr_t) _ps);
        src = (void *) ps;

        // First use DMA to get most of the data there.
        hb_mc_dma_htod_t htod;
        htod.d_addr = ((intptr_t) _ps);;
        htod.h_addr = src;
        htod.size   = (1 << ARR_LOG2_NUM_ELEMENTS) * sizeof(eva_t);

        BSG_CUDA_CALL(hb_mc_device_dma_to_device(&device, &htod, 1));

        // If the dataset fits in cache, warm the cache
        size_t tot_cache_size = dev_dim.x * 2 * hb_mc_config_get_vcache_size(hb_mc_manycore_get_config(device.mc));
        if(((1 << ARR_LOG2_NUM_ELEMENTS) * sizeof(eva_t)) <= tot_cache_size){
                BSG_CUDA_CALL(hb_mc_device_memcpy (&device, dst, src, (1 << ARR_LOG2_NUM_ELEMENTS) * sizeof(eva_t), HB_MC_MEMCPY_TO_DEVICE));
        }

        //************************************************************
        // Prepare list of arguments for kernel.
        //************************************************************
        uint32_t cuda_argv[3] = {_ps, 1, (1 << ARR_LOG2_NUM_ELEMENTS)};

        //************************************************************
        // Enquque grid of tile groups, pass in grid and tile group
        // dimensions, kernel name, number and list of arguments
        //************************************************************
        bsg_pr_info("Enqueue Kernel\n");
        BSG_CUDA_CALL(hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_dram_pointer_chase", sizeof(cuda_argv)/sizeof(cuda_argv[0]), cuda_argv));

        //************************************************************
        // Launch and execute all tile groups on device and wait for all to finish.
        //************************************************************
        bsg_pr_info("Execute Kernel\n");

        uint64_t cycle_start, cycle_end;
        hb_mc_manycore_get_cycle((&device)->mc, &cycle_start);

        BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));

        hb_mc_manycore_get_cycle((&device)->mc, &cycle_end);

        //************************************************************
        // Copy result back from device DRAM into host memory.
        //************************************************************

        //************************************************************
        // Freeze the tiles and memory manager cleanup.
        //************************************************************
        BSG_CUDA_CALL(hb_mc_device_finish(&device));

        bsg_pr_test_info(BSG_GREEN("Pass\n"));

        bsg_pr_info("\n\n====== EXECUTION STATISTICS ====== \n");
        bsg_pr_info("Cycles: %lu\n", cycle_end-cycle_start);
        bsg_pr_info("====== END EXECUTION STATISTICS ====== \n\n\n");

        return HB_MC_SUCCESS;
}

declare_program_main("Group Stride", kernel_ptr_chase);
