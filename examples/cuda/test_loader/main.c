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
#include <cl_manycore_regression.h>
#include <sys/stat.h>

#define ALLOC_NAME "default_allocator"

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
        BSG_CUDA_CALL(hb_mc_device_init(&device, test_name, 0));

        hb_mc_pod_id_t pod;
        hb_mc_device_foreach_pod_id(&device, pod)
        {
                bsg_pr_test_info("Loading program for test %s onto pod %d\n",
                                 test_name, pod);

                BSG_CUDA_CALL(hb_mc_device_set_default_pod(&device, pod));
                BSG_CUDA_CALL(hb_mc_device_program_init(&device, bin_path, ALLOC_NAME, 0));

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
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, sizeof(uint32_t), &raddr));
                BSG_CUDA_CALL(hb_mc_device_memset(&device, &raddr, 0, sizeof(uint32_t)));

                /**********************************************************************/
                /* Prepare list of input arguments for kernel.                        */
                /**********************************************************************/
                int kernel_argv[] = {raddr};
                char kernel_name[256];
                snprintf(kernel_name, sizeof(kernel_name), "kernel_%s",
                         test_name + sizeof("test_") - 1);

                /**********************************************************************/
                /* Enquque grid of tile groups, pass in grid and tile group dimensions*/
                /* kernel name, number and list of input arguments                    */
                /**********************************************************************/
                BSG_CUDA_CALL(hb_mc_kernel_enqueue(&device, grid_dim, tg_dim, kernel_name,
                                                   1, kernel_argv));

                /**********************************************************************/
                /* Launch and execute all tile groups on device and wait for finish.  */
                /**********************************************************************/
                BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));

                /*************************/
                /* Read the return value */
                /*************************/
                uint32_t rcode;
                BSG_CUDA_CALL(hb_mc_device_memcpy(&device, &rcode, (void*)raddr, sizeof(rcode),
                                                  HB_MC_MEMCPY_TO_HOST));

                /**********************************************************************/
                /* Freeze the tiles and memory manager cleanup.                       */
                /**********************************************************************/
                BSG_CUDA_CALL(hb_mc_device_program_finish(&device));

                /*************************/
                /* Check the return code */
                /*************************/
                if (rcode != 0) {
                        bsg_pr_err("kernel returned non-zero.\n");
                        return HB_MC_FAIL;
                }
        } // foreach pod_id

        BSG_CUDA_CALL(hb_mc_device_finish(&device));

        return HB_MC_SUCCESS;
}

#ifdef VCS
int vcs_main(int argc, char ** argv)
#else
int main(int argc, char ** argv)
#endif
{
        bsg_pr_test_info("Unified Main CUDA Regression Test\n");
        int rc = test_loader(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}


