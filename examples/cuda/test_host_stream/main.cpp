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
#include <bsg_manycore_responder.h>
#include <algorithm>
#include <vector>
#include <bsg_manycore_spsc_queue.hpp>

#define ALLOC_NAME "default_allocator"
#define TEST_BYTE 0xcd

#define BUFFER_ELS  10
#define CHAIN_LEN    4
#define NUM_PACKETS 100

/*!
 * Runs a host_stream kernel on a 2x2 tile group.
 * This test streams data through circular buffers on the host through the manycore
 * in a chain, then streams back to the host. Validation is that the data received
 * matches the data pattern sent.
 * 
 * This test demonstrates how host can be run concurrently with manycore code in a
 * streaming or cooperative manner.
 */

int kernel_host_stream(int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Device Memset Kernel on a grid of one 2x2 tile group.\n\n");

        /*****************************************************************************************************************
        * Define path to binary.
        * Initialize device, load binary and unfreeze tiles.
        ******************************************************************************************************************/
        hb_mc_device_t *device = (hb_mc_device_t *) malloc(sizeof(hb_mc_device_t));
        BSG_CUDA_CALL(hb_mc_device_init(device, test_name, 0));
        BSG_CUDA_CALL(hb_mc_device_program_init(device, bin_path, ALLOC_NAME, 0));
        hb_mc_manycore_t *mc = device->mc;
        hb_mc_pod_id_t pod_id = device->default_pod_id;
        hb_mc_pod_t *pod = &device->pods[pod_id];

        /*****************************************************************************************************************
        * 
        ******************************************************************************************************************/
        eva_t buffer_device;
        eva_t count_device;
        BSG_CUDA_CALL(hb_mc_device_malloc(device, BUFFER_ELS * (CHAIN_LEN+1) * sizeof(int), &buffer_device));
        BSG_CUDA_CALL(hb_mc_device_malloc(device, (CHAIN_LEN+1) * sizeof(int), &count_device));

        BSG_CUDA_CALL(hb_mc_device_memset(device, &count_device, 0, (CHAIN_LEN+1) * sizeof(int)));

        /*****************************************************************************************************************
        * Define block_size_x/y: amount of work for each tile group
        * Define tg_dim_x/y: number of tiles in each tile group
        * Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
        ******************************************************************************************************************/
        hb_mc_dimension_t tg_dim = { .x = CHAIN_LEN, .y = 1 }; 

        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1 };


        /*****************************************************************************************************************
        * Prepare list of input arguments for kernel.
        ******************************************************************************************************************/
        uint32_t cuda_argv[2] = {buffer_device, count_device};

        /*****************************************************************************************************************
        * Enquque grid of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
        ******************************************************************************************************************/
        BSG_CUDA_CALL(hb_mc_kernel_enqueue (device, grid_dim, tg_dim, "kernel_host_stream", 2, cuda_argv));

        /*****************************************************************************************************************
        * Launch and execute all tile groups on device and wait for all to finish. 
        ******************************************************************************************************************/

        int packets_sent = 0;
        int packets_recv = 0;
        int mismatch = 0;
        void *src, *dst;

        eva_t send_count_eva = count_device;
        eva_t send_buffer_eva = buffer_device;
        bsg_manycore_spsc_queue_send<int, BUFFER_ELS> send_spsc(device, send_buffer_eva, send_count_eva);

        eva_t recv_count_eva = count_device + CHAIN_LEN * sizeof(int);
        eva_t recv_buffer_eva = buffer_device + (CHAIN_LEN * BUFFER_ELS * sizeof(int));
        bsg_manycore_spsc_queue_recv<int, BUFFER_ELS> recv_spsc(device, recv_buffer_eva, recv_count_eva);
        BSG_CUDA_CALL(hb_mc_manycore_host_request_fence(mc, -1));
        BSG_CUDA_CALL(hb_mc_device_pod_try_launch_tile_groups(device, pod));
        do
        {
            int send_data = packets_sent;
            if (send_spsc.try_send(send_data))
            {
                packets_sent++;
            }

            int recv_data;
            if (recv_spsc.try_recv(&recv_data))
            {
                if (recv_data != packets_recv++)
                {
                    mismatch = 1;
                }
            }

            // Check for finish
            hb_mc_device_pod_wait_for_tile_group_finish_any(device, pod, 1);
        } while (packets_recv < NUM_PACKETS);
        
        /*****************************************************************************************************************
        * Freeze the tiles and memory manager cleanup. 
        ******************************************************************************************************************/
        BSG_CUDA_CALL(hb_mc_device_finish(device)); 

        // Fail if data is not expected
        if (mismatch) { 
                return HB_MC_FAIL;
        }
        return HB_MC_SUCCESS;
}

declare_program_main("test_host_stream", kernel_host_stream);
