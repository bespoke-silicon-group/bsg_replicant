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

#include <bsg_manycore_errno.h>
#include <bsg_manycore_cuda.h>
#include <cstdlib>
#include <time.h>
#include <cstring>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstdio>
#include <bsg_manycore_regression.h>
#include <bsg_manycore.h>
#include <cstdint>
#include "bs.hpp"

#define ALLOC_NAME "default_allocator"


int kernel_bs (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};
        uint64_t cycle_start, cycle_end;

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Black-Scholes Kernel on a grid of 1x1 tile groups.\n\n");

        // TODO: Set values
        char *inputFile = "inputs/in_16K.txt";

        FILE *file;
        int i;
        float * ibuffer;
        int * ibuffer2;
        int rv;

        // Global Data in PARSEC
        int numOptions;
        OptionData *data;
        float * puts, *calls;
        float * _puts, *_calls;
        int numError = 0;

        //Read input data from file
        file = fopen(inputFile, "r");
        if(file == NULL) {
                printf("ERROR: Unable to open file `%s'.\n", inputFile);
                exit(1);
        }

        // File format:
        rv = fscanf(file, "%i", &numOptions);
        if(rv != 1) {
                printf("ERROR: Unable to read from file `%s'.\n", inputFile);
                fclose(file);
                exit(1);
        }

        printf("Number of Options: %d\n", numOptions);

        data = (OptionData*)malloc(numOptions*sizeof(OptionData));
        for (int i = 0; i < numOptions; ++ i ){
                rv = fscanf(file, "%f %f %f %f %f %f %c %f %f", &data[i].s, &data[i].strike, &data[i].r, &data[i].divq, &data[i].v, &data[i].t, &data[i].OptionType, &data[i].divs, &data[i].DGrefval);
                if(rv != 9) {
                        printf("ERROR: Unable to read from file `%s'.\n", inputFile);
                        fclose(file);
                        exit(1);
                }
        }

        rv = fclose(file);

        if(rv != 0) {
                printf("ERROR: Unable to close file `%s'.\n", inputFile);
                exit(1);
        }


        // Allocate outputs
        _puts = (float*)malloc(numOptions*sizeof(float));
        _calls = (float*)malloc(numOptions*sizeof(float));
        puts = (float*)malloc(numOptions*sizeof(float));
        calls = (float*)malloc(numOptions*sizeof(float));

        // Copy in data
        for (i=0; i<numOptions; i++) {
                data[i].OptionType      = ((data[i].OptionType & 0xff) == 'P') ? 1 : 0;
        }

        // At this point the data is ready to launch.
        for (i=0; i<numOptions; i++) {
                BlkSchlsEqEuroNoDiv(data[i].s, data[i].strike,
                                    data[i].r, data[i].v, data[i].t,
                                    _puts[i], _calls[i], 0);
        }

        /*********************/
        /* Initialize device */
        /*********************/
        hb_mc_device_t device;
        BSG_CUDA_CALL(hb_mc_device_init(&device, test_name, 0));


        hb_mc_pod_id_t pod;
        hb_mc_device_foreach_pod_id(&device, pod)
        {
                // Define path to binary
                // Initialize device, load binary and unfreeze tiles
                bsg_pr_test_info("Loading program for %s onto pod %d\n", test_name, pod);

                BSG_CUDA_CALL(hb_mc_device_set_default_pod(&device, pod));
                BSG_CUDA_CALL(hb_mc_device_program_init(&device, bin_path, ALLOC_NAME, 0));

                eva_t option_buf_dev;
                eva_t put_buf_dev;
                eva_t call_buf_dev;
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, numOptions * sizeof(OptionData), &option_buf_dev));
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, numOptions * sizeof(float), &put_buf_dev));
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, numOptions * sizeof(float), &call_buf_dev));

                void *src, *dst;
                // Copy data host onto device DRAM.
                dst = (void *) ((intptr_t) option_buf_dev);
                src = (void *) data;
                BSG_CUDA_CALL(hb_mc_device_memcpy (&device, dst, src,  numOptions * sizeof(OptionData), HB_MC_MEMCPY_TO_DEVICE));

                // Define tg_dim_x/y: number of tiles in each tile group
                // Calculate grid_dim_x/y: number of tile groups needed
                hb_mc_dimension_t tg_dim = { .x = 1, .y = 1 };
                hb_mc_dimension_t grid_dim = { .x = 16, .y = 8};
                uint32_t opts_tile = numOptions / (grid_dim.x * grid_dim.y);
                if(numOptions % (grid_dim.x * grid_dim.y)){
                        bsg_pr_test_err("Number of Options does not divide evenly between tiles\n");
                        return HB_MC_FAIL;
                }

                uint32_t cuda_argv[4] = {option_buf_dev, put_buf_dev, call_buf_dev, opts_tile};


                BSG_CUDA_CALL(hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_black_scholes", 4, cuda_argv));

                rc = hb_mc_manycore_get_cycle((device.mc), &cycle_start);
                if(rc != HB_MC_SUCCESS){
                        bsg_pr_test_err("Failed to read cycle counter: %s\n",
                                        hb_mc_strerror(rc));
                        return HB_MC_FAIL;
                }
                bsg_pr_test_info("Current cycle is: %lu\n", cycle_start);

                BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));

                rc = hb_mc_manycore_get_cycle((device.mc), &cycle_end);
                if(rc != HB_MC_SUCCESS){
                        bsg_pr_test_err("Failed to read cycle counter: %s\n",
                                        hb_mc_strerror(rc));
                        return HB_MC_FAIL;
                }
                bsg_pr_test_info("Current cycle is: %lu. Difference: %lu \n", cycle_end, cycle_end-cycle_start);

                // Copy result back from device DRAM into host memory.
                src = (void *) ((intptr_t) put_buf_dev);
                dst = (void *) puts;
                BSG_CUDA_CALL(hb_mc_device_memcpy (&device, dst, src, numOptions*sizeof(float), HB_MC_MEMCPY_TO_HOST)
);

                src = (void *) ((intptr_t) call_buf_dev);
                dst = (void *) calls;
                BSG_CUDA_CALL(hb_mc_device_memcpy (&device, dst, src, numOptions*sizeof(float), HB_MC_MEMCPY_TO_HOST)
);

                BSG_CUDA_CALL(hb_mc_device_program_finish(&device));

                float err = 0.0;
                for(int i = 0 ; i < numOptions; ++i){
                        bsg_pr_info("PUT: x86: %f, RISC-V: %f\n", _puts[i], puts[i]);
                        auto diff = (_puts[i] - puts[i]);
                        err += diff * diff;
                }

                for(int i = 0 ; i < numOptions; ++i){
                        bsg_pr_info("CALL: x86: %f, RISC-V: %f\n", _calls[i], calls[i]);
                        auto diff = (_calls[i] - calls[i]);
                        err += diff * diff;
                }

                bsg_pr_info("SSE: %f\n", err);
                if(err > .01){
                        return HB_MC_FAIL;
                }
        }

        BSG_CUDA_CALL(hb_mc_device_finish(&device));

        return HB_MC_SUCCESS;

}

declare_program_main("test_bs", kernel_bs);
