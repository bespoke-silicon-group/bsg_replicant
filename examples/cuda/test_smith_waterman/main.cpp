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
#include <math.h>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>

using namespace std;

#define PRINT_SCORE
//#define PRINT_MATRIX
#define ALLOC_NAME "default_allocator"
#define CUDA_CALL(expr)                                                 \
        {                                                               \
                int __err;                                              \
                __err = expr;                                           \
                if (__err != HB_MC_SUCCESS) {                           \
                        bsg_pr_err("'%s' failed: %s\n", #expr, hb_mc_strerror(__err)); \
                        return __err;                                   \
                }                                                       \
        }

void read_seq(const string file_name, const int N,
              const int SIZE_SEQ, unsigned* seq, unsigned* size) {
  map<char, int> dna_char2int = {{'A', 0}, {'C', 1}, {'G', 2}, {'T', 3}, {'N', 0}};

  ifstream fin;
  fin.open(file_name, ios::in);
  for (int i = 0; i < N; i++) {
    string str, num;
    fin >> num >> str;
    for (int j = 0; j < str.size(); j++) {
      seq[i*SIZE_SEQ+j] = dna_char2int[str[j]];
    }
    size[i] = str.size();
  };
  fin.close();
}

int kernel_smith_waterman (int argc, char **argv) {
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Vector Addition Kernel on one 2x2 tile groups.\n");

        srand(static_cast<unsigned>(time(0)));

        /* Define path to binary. */
        /* Initialize device, load binary and unfreeze tiles. */
        hb_mc_dimension_t tg_dim = { .x = 1, .y = 1};
        hb_mc_device_t device;
        BSG_CUDA_CALL(hb_mc_device_init_custom_dimensions(&device, test_name, 0, tg_dim));

        /* if DMA is not supported just return SUCCESS */
        if (!hb_mc_manycore_supports_dma_write(device.mc)
            || !hb_mc_manycore_supports_dma_read(device.mc)) {
                bsg_pr_test_info("DMA not supported for this machine: returning success\n");
                BSG_CUDA_CALL(hb_mc_device_finish(&device));
                return HB_MC_SUCCESS;
        }

        hb_mc_pod_id_t pod;
        hb_mc_device_foreach_pod_id(&device, pod)
        {
                BSG_CUDA_CALL(hb_mc_device_set_default_pod(&device, pod));
                BSG_CUDA_CALL(hb_mc_device_program_init(&device, bin_path, ALLOC_NAME, 0));

                // == Reading data
                const unsigned N = 4;
                // read N queries
                const int SIZEA_MAX = 32;
                unsigned* seqa = new unsigned[N*SIZEA_MAX]();
                unsigned* sizea = new unsigned[N];
                read_seq("../data/dna-query.fasta", N, SIZEA_MAX, seqa, sizea);

                // read N references
                const int SIZEB_MAX = 32;
                unsigned* seqb = new unsigned[N*SIZEB_MAX]();
                unsigned* sizeb = new unsigned[N];
                read_seq("../data/dna-reference.fasta", N, SIZEB_MAX, seqb, sizeb);

                // == Sending data to device
                // Define the sizes of the I/O arrays
                size_t seqa_bytes = N * SIZEA_MAX * sizeof(unsigned);
                size_t seqb_bytes = N * SIZEB_MAX * sizeof(unsigned);
                size_t sizea_bytes = N * sizeof(unsigned);
                size_t sizeb_bytes = N * sizeof(unsigned);
                size_t score_bytes = N * sizeof(unsigned);

                // Allocate device memory for the I/O arrays
                eva_t seqa_d, seqb_d, sizea_d, sizeb_d, score_d;
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, seqa_bytes, &seqa_d));
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, seqb_bytes, &seqb_d));
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, sizea_bytes, &sizea_d));
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, sizeb_bytes, &sizeb_d));
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, score_bytes, &score_d));

                // Transfer data host -> device
                hb_mc_dma_htod_t htod_jobs [] = {
                        {
                                .d_addr = seqa_d,
                                .h_addr = seqa,
                                .size   = seqa_bytes
                        },
                        {
                                .d_addr = seqb_d,
                                .h_addr = seqb,
                                .size   = seqb_bytes
                        },
                        {
                                .d_addr = sizea_d,
                                .h_addr = sizea,
                                .size   = sizea_bytes
                        },
                        {
                                .d_addr = sizeb_d,
                                .h_addr = sizeb,
                                .size   = sizeb_bytes
                        }
                };

                bsg_pr_test_info("Writing A and B to device\n");

                BSG_CUDA_CALL(hb_mc_device_dma_to_device(&device, htod_jobs, 4));

                // == Launching kernel ==
                // Define amount of work for each tile group
                /* Define tg_dim_x/y: number of tiles in each tile group */
                /* Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y */
                hb_mc_dimension_t grid_dim = { .x = 1, .y = 1};

                /* Prepare list of input arguments for kernel. */
                int num_tiles = tg_dim.x * tg_dim.y;
                uint32_t cuda_argv[6] = {seqa_d, seqb_d, sizea_d, sizeb_d, N/num_tiles, score_d};

                /* Enque grid of tile groups, pass in grid and tile group dimensions,
                   kernel name, number and list of input arguments */
                BSG_CUDA_CALL(hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_smith_waterman", 6, cuda_argv));

                /* Launch and execute all tile groups on device and wait for all to finish.  */
                BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));

                // Transfer data device -> host
                unsigned* score = new unsigned[N];
                hb_mc_dma_dtoh_t dtoh_job = {
                        .d_addr = score_d,
                        .h_addr = score,
                        .size   = score_bytes
                };

                bsg_pr_test_info("Reading C to host\n");

                BSG_CUDA_CALL(hb_mc_device_dma_to_host(&device, &dtoh_job, 1));

                /* Calculate the expected result using host code and compare the results.  */
                //if (mismatch)
                        //return HB_MC_FAIL;

                /* Freeze the tiles and memory manager cleanup.  */
                BSG_CUDA_CALL(hb_mc_device_program_finish(&device));

                // write N scores to file
                ofstream fout;
                fout.open("output", ios::out);
                for (int i = 0; i < N; i++) {
                  fout << (int)score[i] << endl;
                }
                fout.close();

        }

        BSG_CUDA_CALL(hb_mc_device_finish(&device));

        return HB_MC_SUCCESS;
}

declare_program_main("test_smith_waterman", kernel_smith_waterman);

