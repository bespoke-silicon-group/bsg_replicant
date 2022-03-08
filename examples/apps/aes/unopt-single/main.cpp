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
#include <cstdlib>
#include <time.h>
#include <cstring>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstdio>
#include <bsg_manycore_regression.h>
#include <bsg_manycore.h>
#include <cstdint>
#include <aes.hpp>

#define ALLOC_NAME "default_allocator"
#define MSG_LEN 1024

int kernel_aes (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};
        uint64_t cycle_start, cycle_end;

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA AES Kernel on a grid of 1x1 tile groups.\n\n");

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

                uint8_t buf[MSG_LEN];
                uint8_t host_buf[MSG_LEN];
                for(int i = 0; i < MSG_LEN; ++i){
                        buf[i] = host_buf[i] = (uint8_t)(i);
                }

                struct AES_ctx ctx, host_ctx;
                printf("Ctx (sz) %d\n", sizeof(ctx));
                printf("Ctx: %p\n", &ctx);
                printf("Iv: %p\n", &ctx.Iv);
                printf("RoundKey: %p\n", &ctx.RoundKey);
                // Memset structs to 0 (this is critical!)
                memset(&ctx, 0, sizeof(ctx));
                memset(&host_ctx, 0, sizeof(host_ctx));
                uint8_t key[AES_KEYLEN] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF};
                AES_init_ctx(&ctx, key);
                AES_init_ctx(&host_ctx, key);
                
                AES_CBC_encrypt_buffer(&host_ctx, host_buf, MSG_LEN);

                // Allocate memory on device
                eva_t ctx_device, buf_device;
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, sizeof(ctx), &ctx_device));
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, sizeof(buf), &buf_device));
                

                // Copy data host onto device DRAM.
                void *dst = (void *) ((intptr_t) ctx_device);
                void *src = (void *) &ctx;
                BSG_CUDA_CALL(hb_mc_device_memcpy (&device, dst, src, sizeof(ctx), HB_MC_MEMCPY_TO_DEVICE));

                dst = (void *) ((intptr_t) buf_device);
                src = (void *) &buf[0];
                BSG_CUDA_CALL(hb_mc_device_memcpy (&device, dst, src, sizeof(buf), HB_MC_MEMCPY_TO_DEVICE));

                // Define tg_dim_x/y: number of tiles in each tile group
                // Calculate grid_dim_x/y: number of tile groups needed
                hb_mc_dimension_t tg_dim = { .x = 1, .y = 1 };
                hb_mc_dimension_t grid_dim = { .x = 1, .y = 1};

                uint32_t cuda_argv[3] = {ctx_device, buf_device, MSG_LEN};

                BSG_CUDA_CALL(hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "AES_CBC_encrypt_buffer", 3, cuda_argv));

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
                src = (void *) ((intptr_t) buf_device);
                dst = (void *) &buf[0];
                BSG_CUDA_CALL(hb_mc_device_memcpy (&device, (void *) dst, src, sizeof(buf), HB_MC_MEMCPY_TO_HOST));

                BSG_CUDA_CALL(hb_mc_device_program_finish(&device));

                // Compare the results.
                if(memcmp(host_buf, buf, MSG_LEN) != 0){
                        bsg_pr_err(BSG_RED("Ciphertext mismatch!\n"));
                        for (int i = 0; i < MSG_LEN;){
                                int j;
                                for(j = 0; j < 32; ++j){
                                        printf("%02x ", buf[i + j]);
                                }
                                printf("\n");
                                for(j = 0; j < 32; ++j){
                                        printf("%02x ", host_buf[i + j]);
                                }
                                printf("\n");
                                i += j;
                        }
                        return HB_MC_FAIL;
                } 

        }

        BSG_CUDA_CALL(hb_mc_device_finish(&device));

        return HB_MC_SUCCESS;

}

declare_program_main("test_aes", kernel_aes);
