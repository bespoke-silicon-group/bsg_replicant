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

/*******************************************************************/
/* This kernel is designed to overload the memory system.          */
/* It forces cache evictions by striding to the line in the        */
/* same set of the same cache and doing a single store.            */
/* It then stores a word to an address maping to the evicted line. */
/*******************************************************************/

#include "test_hammer_cache.hpp"
#include <sys/stat.h>

#define ALLOC_NAME "default_allocator"

#define CUDA_CALL(expr)                                                 \
        {                                                               \
                int __r;                                                \
                __r = expr;                                             \
                if (__r != HB_MC_SUCCESS) {                             \
                        bsg_pr_err("'%s' failed: %s\n", #expr, hb_mc_strerror(__r)); \
                        return __r;                                     \
                }                                                       \
        }

#define array_size(x)                           \
        (sizeof(x)/sizeof(x[0]))

int test_loader (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running %s "
                         "on a grid of 2x2 tile groups\n\n", test_name);

        // init
        hb_mc_device_t device;
        CUDA_CALL(hb_mc_device_init(&device, test_name, 0));
        CUDA_CALL(hb_mc_device_program_init(&device, bin_path, ALLOC_NAME, 0));

        // config tg
        hb_mc_dimension_t tg_dim = { .x = 2, .y = 2 }; 
        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1 };

        // args
        hb_mc_eva_t mem;
        CUDA_CALL(hb_mc_device_malloc(&device, (1<<20), &mem));

        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(device.mc);

        uint32_t block_words, n_caches, n_ways, n_hammers, n_sets;
        block_words = hb_mc_config_get_vcache_block_words(cfg);
        n_caches    = hb_mc_config_get_num_dram_coordinates(cfg);        
        n_hammers   = 16;
        n_ways      = hb_mc_config_get_vcache_ways(cfg);
        n_sets      = hb_mc_config_get_vcache_sets(cfg);
                
        uint32_t kernel_argv[] = {mem, block_words, n_caches, n_ways, n_sets, n_hammers};

        char kernel_name[256];
        snprintf(kernel_name, sizeof(kernel_name), "kernel_%s", test_name + sizeof("test_") - 1);

        // call kernel
        CUDA_CALL(hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, kernel_name,
                                        array_size(kernel_argv), kernel_argv));
        
        CUDA_CALL(hb_mc_device_tile_groups_execute(&device));
        
        // done
        CUDA_CALL(hb_mc_device_finish(&device));

        return HB_MC_SUCCESS;
}

#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif
        bsg_pr_test_info("Unified Main CUDA Regression Test \n");
        int rc = test_loader(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
