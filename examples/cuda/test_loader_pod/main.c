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
#include <sys/stat.h>

#define ALLOC_NAME "default_allocator"

int test_loader_program_run(struct arguments_path *args, hb_mc_device_t *dev)
{
        char *bin_path = args->path;
        char *test_name = args->name;

        hb_mc_pod_id_t pod = 1;
        BSG_CUDA_CALL(hb_mc_device_pod_program_init(dev, pod, bin_path));

        /**********************************************************************/
        /* Define block_size_x/y: amount of work for each tile group          */
        /* Define tg_dim_x/y: number of tiles in each tile group              */
        /* Calculate grid_dim_x/y: number of                                  */
        /* tile groups needed based on block_size_x/y                         */
        /**********************************************************************/
        hb_mc_dimension_t tg_dim = { .x = 2, .y = 2 };
        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1 };

        /****************************************/
        /* Allocate a word for the return value */
        /****************************************/
        hb_mc_eva_t raddr;
        BSG_CUDA_CALL(hb_mc_device_pod_malloc(dev, pod, sizeof(uint32_t), &raddr));
        BSG_CUDA_CALL(hb_mc_device_pod_memset(dev, pod, raddr, 0, sizeof(uint32_t)));

        /**********************************************************************/
        /* Prepare list of input arguments for kernel.                        */
        /**********************************************************************/
        int kernel_argv[] = {raddr};
        char kernel_name[256];
        snprintf(kernel_name, sizeof(kernel_name), "kernel_%s", test_name + sizeof("test_") - 1);

        /**********************************************************************/
        /* Enquque grid of tile groups, pass in grid and tile group dimensions*/
        /* kernel name, number and list of input arguments                    */
        /**********************************************************************/
        BSG_CUDA_CALL(hb_mc_device_pod_kernel_enqueue(dev, pod, grid_dim, tg_dim, kernel_name, 1, kernel_argv));

        /**********************************************************************/
        /* Launch and execute all tile groups on device and wait for finish.  */
        /**********************************************************************/
        BSG_CUDA_CALL(hb_mc_device_pod_kernels_execute(dev, pod));

        /*************************/
        /* Read the return value */
        /*************************/
        uint32_t rcode;
        BSG_CUDA_CALL(hb_mc_device_pod_memcpy(dev, pod, &rcode, (void*)raddr, sizeof(rcode),
                                              HB_MC_MEMCPY_TO_HOST));

        /**********************************************************************/
        /* Freeze the tiles and memory manager cleanup.                       */
        /**********************************************************************/
        BSG_CUDA_CALL(hb_mc_device_pod_program_finish(dev, pod));

        /*************************/
        /* Check the return code */
        /*************************/
        if (rcode != 0) {
                bsg_pr_err("kernel returned non-zero.\n");
                return HB_MC_FAIL;
        }

        return HB_MC_SUCCESS;

}

int test_loader (int argc, char **argv) {
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Unified Main %s "
                         "on a grid of 2x2 tile groups\n\n", test_name);

        srand(time);

        /**********************************************************************/
        /* Define path to binary.                                             */
        /* Initialize device, load binary and unfreeze tiles.                 */
        /**********************************************************************/
        hb_mc_device_t device;
        BSG_CUDA_CALL(hb_mc_device_init(&device, test_name, args.device_id));

        int r = test_loader_program_run(&args, &device);

        BSG_CUDA_CALL(hb_mc_device_finish(&device));

        return r;
}

declare_program_main("Unified Main CUDA Pods", test_loader);
