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
#include <bsg_manycore.h>
#include <bsg_manycore_tile.h>
#include <bsg_manycore_loader.h>
#include <bsg_manycore_errno.h>
#include <bsg_manycore_config_pod.h>

#include <bsg_manycore_regression.h>

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>

#define NUM_CMD 4
#define DUMMY_KERNEL_NAME "NO_KERNEL"

int test_loader(int argc, char **argv) {
        unsigned char *program_data;
        size_t program_size;
        hb_mc_manycore_t manycore = {0}, *mc = &manycore;
        int err, r = HB_MC_FAIL;
        hb_mc_dimension_t tg;
        char *bin_path, *test_name;
        struct arguments_spmd args = {NULL, NULL, 0, 0};
        int augmented_argc;
        char *augmented_argv[NUM_CMD + 2];

        if(argc != NUM_CMD) {
            fprintf(stderr, "Incorrect number of CMD arguments\n");
            return HB_MC_FAIL;
        }
        // Add dummy kernel name
        augmented_argc = argc + 1;
        memcpy(&augmented_argv[2], &argv[1], sizeof(argv[0]) * (NUM_CMD + 1));
        augmented_argv[1] = DUMMY_KERNEL_NAME;

        argp_parse (&argp_spmd, augmented_argc, augmented_argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;
        tg.x = args.tg_x;
        tg.y = args.tg_y;

        bsg_pr_test_info("bin name: %s; test name: %s\n", bin_path, test_name);
        bsg_pr_test_info("Tile group dimension: %d %d\n", tg.x, tg.y);

        // initialize the manycore
        err = hb_mc_manycore_init(mc, test_name, 0);
        if (err != HB_MC_SUCCESS) {
                bsg_pr_err("failed to initialize manycore instance: %s\n",
                           hb_mc_strerror(err));
                return err;
        }

        /* initialize the tile */
        hb_mc_coordinate_t pod;
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        hb_mc_config_foreach_pod(pod, cfg)
        {
                hb_mc_coordinate_t origin = hb_mc_config_pod_vcore_origin(cfg, pod);
                hb_mc_coordinate_t target = origin;

                char pod_str[256];
                hb_mc_coordinate_to_string(pod, pod_str, sizeof(pod_str));
                bsg_pr_test_info("Initializing pod %s\n", pod_str);

                foreach_coordinate(target, origin, tg){
                        // freeze the tile
                        err = hb_mc_tile_freeze(mc, &target);
                        if (err != HB_MC_SUCCESS) {
                                bsg_pr_err("failed to freeze tile (%" PRId32 ", %" PRId32 "): %s\n",
                                           hb_mc_coordinate_get_x(target),
                                           hb_mc_coordinate_get_y(target),
                                           hb_mc_strerror(err));
                                goto cleanup;
                        }
                        // set its origin
                        err = hb_mc_tile_set_origin(mc, &target, &origin);
                        if (err != HB_MC_SUCCESS) {
                                bsg_pr_err("failed to set origin of (%" PRId32 ", %" PRId32 ") "
                                           "to (%" PRId32 ", %" PRId32 "): %s\n",
                                           hb_mc_coordinate_get_x(target),
                                           hb_mc_coordinate_get_y(target),
                                           hb_mc_coordinate_get_x(origin),
                                           hb_mc_coordinate_get_y(origin),
                                           hb_mc_strerror(err));
                                goto cleanup;
                        }

                        // set its tg origin
                        err = hb_mc_tile_set_origin_registers(mc, &target, &origin);
                        if (err != HB_MC_SUCCESS) {
                                bsg_pr_err("failed to set origin of (%" PRId32 ", %" PRId32 ") "
                                           "to (%" PRId32 ", %" PRId32 "): %s\n",
                                           hb_mc_coordinate_get_x(target),
                                           hb_mc_coordinate_get_y(target),
                                           hb_mc_coordinate_get_x(origin),
                                           hb_mc_coordinate_get_y(origin),
                                           hb_mc_strerror(err));
                                goto cleanup;
                        }

                }
        }


        // Read/Write access to DMEM (address 0x0) in pod row 0
        hb_mc_npa_t npa = {.epa = 0};
        for(int x = 0;x < 16*4;x++) {
          for(int y = 0;y < 8;y++) {
            #define EXPECTED_VAL 123
            uint32_t val = 0;
            npa.x = 16 + x;
            npa.y = 8 + y;
            hb_mc_manycore_write32(mc, &npa, EXPECTED_VAL);
            hb_mc_manycore_read32(mc, &npa, &val);
            if(val != EXPECTED_VAL)
              fprintf(stderr, "Wrong read data\n");
            else
              printf("Correct read data\n");
          }
        }

cleanup:
        hb_mc_manycore_exit(mc);
        return err;
        
}
// Although this host code is called test_loader, it is a NoC tester instead of a loader.
// We do this to minimize the changes to the existing flow.
declare_program_main("SPMD ASIC NOC TESTER", test_loader);
