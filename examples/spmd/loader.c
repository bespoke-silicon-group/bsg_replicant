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
        hb_mc_dimension_t tg;
        char *bin_path, *test_name;
        struct arguments_spmd args = {NULL, NULL, 0, 0};

        argp_parse (&argp_spmd, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;
        tg.x = args.tg_x;
        tg.y = args.tg_y;

        int fail = 0;


        bsg_pr_test_info("Reading from file: %s\n", bin_path);
        bsg_pr_test_info("Tile group dimension: %u %u\n", tg.x, tg.y);


        // read in the program data from the file system
        err = read_program_file(bin_path, &program_data, &program_size);
        if (err != HB_MC_SUCCESS)
                return err;

        // initialize the manycore
        err = hb_mc_manycore_init(&manycore, test_name, HB_MC_DEVICE_ID);
        if (err != HB_MC_SUCCESS) {
                bsg_pr_err("failed to initialize manycore instance: %s\n",
                           hb_mc_strerror(err));
                return err;
        }

        /* initialize the tile */
        hb_mc_coordinate_t pod;
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);

        // Specify number of pods to launch, default all available pods (-1)
        int pod_launch_x = HB_MC_POD_GROUP_X;
        int pod_launch_y = HB_MC_POD_GROUP_Y;
        if (pod_launch_x == -1 || pod_launch_x > (int)(cfg->pods.x)) pod_launch_x = cfg->pods.x;
        if (pod_launch_y == -1 || pod_launch_y > (int)(cfg->pods.y)) pod_launch_y = cfg->pods.y;
        bsg_pr_test_info("Pod launch dim desired: %d %d, actual: %d %d\n", 
                            HB_MC_POD_GROUP_X, HB_MC_POD_GROUP_Y, pod_launch_x, pod_launch_y);

        hb_mc_config_foreach_pod(pod, cfg)
        {
                // Skip pods;
                if (((int)(pod.x) >= pod_launch_x) || ((int)(pod.y) >= pod_launch_y)) {
                    continue;
                }

                hb_mc_coordinate_t origin = hb_mc_config_pod_vcore_origin(cfg, pod);
                hb_mc_coordinate_t target = origin;

                char pod_str[256];
                hb_mc_coordinate_to_string(pod, pod_str, sizeof(pod_str));
                bsg_pr_test_info("Loading to pod %s\n", pod_str);

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

                        /* load the program */
                        err = hb_mc_loader_load(program_data, program_size,
                                                mc, &default_map,
                                                &target, 1);
                        if (err != HB_MC_SUCCESS) {
                                bsg_pr_err("failed to load binary '%s': %s\n",
                                           bin_path, hb_mc_strerror(err));
                                return err;
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
#if HB_MC_LAUNCH_PODS_IN_SERIES == 0
        }

        /* unfreeze tile */
        hb_mc_config_foreach_pod(pod, cfg)
        {
                // Skip pods;
                if (((int)(pod.x) >= pod_launch_x) || ((int)(pod.y) >= pod_launch_y)) {
                    continue;
                }

                hb_mc_coordinate_t origin = hb_mc_config_pod_vcore_origin(cfg, pod);
                hb_mc_coordinate_t target = origin;
#endif
                bsg_pr_test_info("Unfreezing pod (%u %u)\n", pod.x, pod.y);

                foreach_coordinate(target, origin, tg){
                        err = hb_mc_tile_unfreeze(mc, &target);
                        if (err != HB_MC_SUCCESS) {
                                bsg_pr_err("failed to unfreeze tile (%" PRId32", %" PRId32 "): %s\n",
                                           hb_mc_coordinate_get_x(target),
                                           hb_mc_coordinate_get_y(target),
                                           hb_mc_strerror(err));
                                goto cleanup;
                        }
                }
#if HB_MC_LAUNCH_PODS_IN_SERIES == 0
        }
#endif

        /* wait until all pods have completed */
        bsg_pr_test_info("Waiting for pods to finish...\n");
        int num_packet_per_pod = (HB_MC_WAIT_ALL_TILES_DONE == 0)? 1 : tg.x * tg.y;
        int num_pod_launched = (HB_MC_LAUNCH_PODS_IN_SERIES != 0)? 1 : pod_launch_x * pod_launch_y;
        int done = 0;
        while (done < num_packet_per_pod * num_pod_launched) {
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

                hb_mc_coordinate_t src;
                src.x = hb_mc_request_packet_get_x_src(&pkt.request);
                src.y = hb_mc_request_packet_get_y_src(&pkt.request);
                pod = hb_mc_config_pod(cfg, src);

                switch (hb_mc_request_packet_get_epa(&pkt.request)) {
                case 0xEAD0:
                        bsg_pr_test_info("Received finish packet from (%3u,%3u)\n", src.x, src.y);
                        bsg_pr_dbg("received finish packet\n");
                        err = (err == HB_MC_FAIL ? HB_MC_FAIL : HB_MC_SUCCESS);
                        done += 1;
                        break;
                case 0xEAD8:
                        bsg_pr_test_info("Received failed packet from (%3u,%3u)\n", src.x, src.y);
                        bsg_pr_dbg("received fail packet\n");
                        err = HB_MC_FAIL;
                        done += 1;
                        fail += 1;
                        break;
                default: break;
                }
        }
#if HB_MC_LAUNCH_PODS_IN_SERIES != 0
        }
#endif
        if (fail > 0) err = HB_MC_FAIL;

cleanup:
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-result"
        hb_mc_manycore_exit(mc);
    #pragma GCC diagnostic pop
        return err;
        
}

declare_program_main("SPMD loader", test_loader);
