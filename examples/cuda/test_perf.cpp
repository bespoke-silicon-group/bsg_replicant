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

// This test is designed to measure the performance of the runtime.

#include "test_perf.hpp"
#define ALLOC_NAME "default_allocator"
#define NUM_ELEMENTS 32768

static inline uint64_t get_sysclk_ms(uint64_t &ms){
        struct timeval tv;
        gettimeofday(&tv, NULL);

        ms = (uint64_t)(tv.tv_sec) * 1000 +
                (uint64_t)(tv.tv_usec) / 1000;

        return ms;
}

int test_perf (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};
        int instr_start, fops_start;
        int instr_end, fops_end;
        uint64_t cycle_start, cycle_end, cycles_elapsed;
        uint64_t ms_start, ms_end, ms_elapsed;
        float seconds_elapsed;

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        /***********************************************************************
        * Define path to binary.
        * Initialize device, load binary and unfreeze tiles.
        ************************************************************************/
        hb_mc_device_t device;
        rc = hb_mc_device_init(&device, test_name, 0);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to initialize device.\n");
                return rc;
        }


        /***********************************************************************
        * Measure initialization performance
        ************************************************************************/
        hb_mc_manycore_get_cycle((&device)->mc, &cycle_start);
        get_sysclk_ms(ms_start);

        rc = hb_mc_device_program_init(&device, bin_path, ALLOC_NAME, 0);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to initialize program.\n");
                return rc;
        }

        hb_mc_manycore_get_cycle((&device)->mc, &cycle_end);
        get_sysclk_ms(ms_end);

        cycles_elapsed = cycle_end-cycle_start;
        ms_elapsed = ms_end - ms_start;
        seconds_elapsed = static_cast<float>(ms_elapsed)/1000.0f;
        bsg_pr_info("Initialization Complete: %llu cycles / %f seconds = %f cycles/second\n",
                    cycles_elapsed, seconds_elapsed, 
                    static_cast<float>(cycles_elapsed)/seconds_elapsed);

        /************************************************************************
        * Allocate memory on the device for A, B and C.
        *************************************************************************/
        eva_t A_device, B_device, C_device; 
        rc = hb_mc_device_malloc(&device, NUM_ELEMENTS * sizeof(float), &A_device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }

        rc = hb_mc_device_malloc(&device, NUM_ELEMENTS * sizeof(float), &B_device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }

        rc = hb_mc_device_malloc(&device, NUM_ELEMENTS * sizeof(float), &C_device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to allocate memory on device.\n");
                return rc;
        }


        /*************************************************************************
        * Allocate memory on the host for A & B and initialize with random values.
        **************************************************************************/
        float A_host[NUM_ELEMENTS];
        float B_host[NUM_ELEMENTS];
        float C_host[NUM_ELEMENTS], C_expected[NUM_ELEMENTS];

        // Initialize the random number generators
        std::numeric_limits<int32_t> lim; // Used to get INT_MIN and INT_MAX in C++
        std::default_random_engine generator;
        generator.seed(42);
        std::uniform_real_distribution<float> distribution(lim.min(),lim.max());
        // Generate random numbers. Since the Manycore can't handle infinities,
        // subnormal numbers, or NANs, filter those out.
        auto res = distribution(generator);

        for (uint64_t i = 0; i < NUM_ELEMENTS; ++i) {
                A_host[i] = distribution(generator);
                B_host[i] = distribution(generator);
        }

        /**************************************************************************
        * Copy A & B from host onto device DRAM.
        * Measure transfer performance for packet-based transfer and DMA (if available)
        ************************************************************************/
        hb_mc_manycore_get_cycle((&device)->mc, &cycle_start);
        get_sysclk_ms(ms_start);

        void *dst = (void *) ((intptr_t) A_device);
        void *src = (void *) &A_host[0];
        rc = hb_mc_device_memcpy (&device, dst, src, NUM_ELEMENTS * sizeof(float), HB_MC_MEMCPY_TO_DEVICE);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy memory to device.\n");
                return rc;
        }

        hb_mc_manycore_get_cycle((&device)->mc, &cycle_end);
        get_sysclk_ms(ms_end);

        cycles_elapsed = cycle_end-cycle_start;
        ms_elapsed = ms_end - ms_start;
        seconds_elapsed = static_cast<float>(ms_elapsed)/1000.0f;
        bsg_pr_info("Data Transfer Complete: %llu cycles / %f seconds = %f cycles/second\n",
                    cycles_elapsed, seconds_elapsed, 
                    static_cast<float>(cycles_elapsed)/seconds_elapsed);

        bsg_pr_info("Data Transfer Complete: %llu bytes / %f seconds = %f bytes/second\n",
                    sizeof(A_host) + sizeof(B_host), seconds_elapsed,
                    static_cast<float>(sizeof(A_host) + sizeof(B_host))/seconds_elapsed);


        hb_mc_manycore_get_cycle((&device)->mc, &cycle_start);
        get_sysclk_ms(ms_start);

        dst = (void *) ((intptr_t) B_device);
        src = (void *) &B_host[0];
        if(hb_mc_manycore_supports_dma_write((&device)->mc)){
                hb_mc_dma_htod_t htod_jobs [] = {
                        {
                                .d_addr = B_device,
                                .h_addr = B_host,
                                .size   = sizeof(B_host)
                        }
                };
                rc = hb_mc_device_dma_to_device(&device, htod_jobs, 1);
        } else {
                rc = hb_mc_device_memcpy (&device, dst, src, NUM_ELEMENTS * sizeof(float), HB_MC_MEMCPY_TO_DEVICE);
        }
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy  memory to device.\n");
                return rc;
        }

        hb_mc_manycore_get_cycle((&device)->mc, &cycle_end);
        get_sysclk_ms(ms_end);

        cycles_elapsed = cycle_end-cycle_start;
        ms_elapsed = ms_end - ms_start;
        seconds_elapsed = static_cast<float>(ms_elapsed)/1000.0f;
        bsg_pr_info("Data Transfer Complete: %llu cycles / %f seconds = %f cycles/second\n",
                    cycles_elapsed, seconds_elapsed, 
                    static_cast<float>(cycles_elapsed)/seconds_elapsed);

        bsg_pr_info("Data Transfer Complete: %llu bytes / %f seconds = %f bytes/second\n",
                    sizeof(A_host) + sizeof(B_host), seconds_elapsed,
                    static_cast<float>(sizeof(A_host) + sizeof(B_host))/seconds_elapsed);

        /***************************************************************************
        * Define block_size_x/y: amount of work for each tile group
        * Define tg_dim_x/y: number of tiles in each tile group
        ****************************************************************************/
        hb_mc_dimension_t tg_dim = { .x = 1, .y = 1}; 

        hb_mc_dimension_t dev_dim;
        dev_dim = hb_mc_config_get_dimension_vcore(hb_mc_manycore_get_config((&device)->mc));
        hb_mc_dimension_t grid_dim = dev_dim;

        uint32_t work = NUM_ELEMENTS / (grid_dim.x * grid_dim.y);

        /****************************************************************************
        * Prepare list of input arguments for kernel.
        *****************************************************************************/
        uint32_t cuda_argv[4] = {A_device, B_device, C_device, work};

        /*****************************************************************************
        * Enquque grid of tile groups, pass in grid and tile group
        * dimensions, kernel name, number and list of input arguments
        ******************************************************************************/
        rc = hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_vec_add_parallel", 4, cuda_argv);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to initialize grid.\n");
                return rc;
        }

        /******************************************************************************
        * Launch and execute all tile groups on device and wait for all to finish. 
        *******************************************************************************/
        hb_mc_manycore_get_cycle((&device)->mc, &cycle_start);
        get_sysclk_ms(ms_start);

        rc = hb_mc_device_tile_groups_execute(&device);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to execute tile groups.\n");
                return rc;
        }

        hb_mc_manycore_get_cycle((&device)->mc, &cycle_end);
        get_sysclk_ms(ms_end);

        cycles_elapsed = cycle_end-cycle_start;
        ms_elapsed = ms_end - ms_start;
        seconds_elapsed = static_cast<float>(ms_elapsed)/1000.0f;
        bsg_pr_info("Computation Complete: %llu cycles / %f seconds = %f cycles/second\n",
                    cycles_elapsed, seconds_elapsed, 
                    static_cast<float>(cycles_elapsed)/seconds_elapsed);

        /********************************************************************************
        * Copy result matrix back from device DRAM into host memory. 
        *********************************************************************************/
        hb_mc_manycore_get_cycle((&device)->mc, &cycle_start);
        get_sysclk_ms(ms_start);

        src = (void *) ((intptr_t) C_device);
        dst = (void *) &C_host[0];
        rc = hb_mc_device_memcpy (&device, (void *) dst, src, NUM_ELEMENTS * sizeof(float), HB_MC_MEMCPY_TO_HOST);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to copy  memory from device.\n");
                return rc;
        }

        hb_mc_manycore_get_cycle((&device)->mc, &cycle_end);
        get_sysclk_ms(ms_end);

        cycles_elapsed = cycle_end-cycle_start;
        ms_elapsed = ms_end - ms_start;
        seconds_elapsed = static_cast<float>(ms_elapsed)/1000.0f;
        bsg_pr_info("Data Transfer Complete: %llu cycles / %f seconds = %f cycles/second\n",
                    cycles_elapsed, seconds_elapsed, 
                    static_cast<float>(cycles_elapsed)/seconds_elapsed);

        bsg_pr_info("Data Transfer Complete: %llu bytes / %f seconds = %f bytes/second\n",
                    sizeof(A_host) + sizeof(B_host), seconds_elapsed,
                    static_cast<float>(sizeof(A_host) + sizeof(B_host))/seconds_elapsed);

        /*********************************************************************************
        * Freeze the tiles and memory manager cleanup. 
        **********************************************************************************/
        rc = hb_mc_device_finish(&device); 
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("failed to de-initialize device.\n");
                return rc;
        }


        /**********************************************************************************
        * Calculate the expected result using host code and compare the results. 
        ***********************************************************************************/
        int mismatch = 0; 
        for (int i = 0; i < NUM_ELEMENTS; i++) {
                if (A_host[i] + B_host[i] != C_host[i]) {
                        bsg_pr_err(BSG_RED("Mismatch: ") "C[%d]:  0x%08" PRIx32 " + 0x%08" PRIx32 " = 0x%08" PRIx32 "\t Expected: 0x%08" PRIx32 "\n", i , A_host[i], B_host[i], C_host[i], A_host[i] + B_host[i]);
                        mismatch = 1;
                }
        } 

        if (mismatch) { 
                return HB_MC_FAIL;
        }
        return HB_MC_SUCCESS;
}


#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif
        bsg_pr_test_info("test_perf Regression Test\n");
        int rc = test_perf(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}
