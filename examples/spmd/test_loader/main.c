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

static int read_program_file(const char *file_name, unsigned char **file_data, size_t *file_size)
{
        struct stat st;
        FILE *f;
        int r;
        unsigned char *data;

        if ((r = stat(file_name, &st)) != 0) {
                bsg_pr_err("could not stat '%s': %m\n", file_name);
                return HB_MC_FAIL;
        }

        if (!(f = fopen(file_name, "rb"))) {
                bsg_pr_err("failed to open '%s': %m\n", file_name);
                return HB_MC_FAIL;
        }

        if (!(data = (unsigned char *) malloc(st.st_size))) {
                bsg_pr_err("failed to read '%s': %m\n", file_name);
                fclose(f);
                return HB_MC_FAIL;
        }

        if ((r = fread(data, st.st_size, 1, f)) != 1) {
                bsg_pr_err("failed to read '%s': %m\n", file_name);
                fclose(f);
                free(data);
                return HB_MC_FAIL;
        }

        fclose(f);
        *file_data = data;
        *file_size = st.st_size;
        return HB_MC_SUCCESS;
}

int test_loader(int argc, char **argv) {
        unsigned char *program_data;
        size_t program_size;
        hb_mc_manycore_t manycore = {0}, *mc = &manycore;
        int err, r = HB_MC_FAIL;

        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Reading from file: %s\n", bin_path);

        // read in the program data from the file system
        err = read_program_file(bin_path, &program_data, &program_size);
        if (err != HB_MC_SUCCESS)
                return err;

        // initialize the manycore
        err = hb_mc_manycore_init(&manycore, test_name, 0);
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
                bsg_pr_test_info("Loading to pod %s\n", pod_str);

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

                /* load the program */
                err = hb_mc_loader_load(program_data, program_size,
                                        mc, &default_map,
                                        &target, 1);
                if (err != HB_MC_SUCCESS) {
                        bsg_pr_err("failed to load binary '%s': %s\n",
                                   bin_path, hb_mc_strerror(err));
                        return err;
                }

                err = hb_mc_tile_unfreeze(mc, &target);
                if (err != HB_MC_SUCCESS) {
                        bsg_pr_err("failed to unfreeze tile (%" PRId32", %" PRId32 "): %s\n",
                                   hb_mc_coordinate_get_x(target),
                                   hb_mc_coordinate_get_y(target),
                                   hb_mc_strerror(err));
                        goto cleanup;
                }

                usleep(100);

                int done = 0;
                while (!done) {
                        hb_mc_packet_t pkt;
                        bsg_pr_dbg("Waiting for finish packet\n");

                        err = hb_mc_manycore_packet_rx(mc, &pkt, HB_MC_FIFO_RX_REQ, -1);
                        if (err != HB_MC_SUCCESS) {
                                bsg_pr_err("failed to read response packet: %s\n",
                                           hb_mc_strerror(err));

                                return HB_MC_FAIL;
                        }

                        char pkt_str[128];
                        hb_mc_request_packet_to_string(&pkt.request, pkt_str, sizeof(pkt_str));

                        bsg_pr_dbg("received packet %s\n", pkt_str);
                
                        switch (hb_mc_request_packet_get_epa(&pkt.request)) {
                        case 0xEAD0:
                                bsg_pr_dbg("received finish packet\n");
                                err = HB_MC_SUCCESS;
                                done = 1;
                                break;
                        case 0xEAD8:
                                bsg_pr_dbg("received fail packet\n");
                                err = HB_MC_FAIL;
                                return err;
                        default: break;
                        }
                }
        } // foreach pod...
cleanup:
        hb_mc_manycore_exit(mc);
        return err;
        
}

declare_program_main("SPMD loader", test_loader);
