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
#include <assert.h>

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

class Sequence {
  private:
  // Read DNA Sequence from file
  static void read_seq(const string file_name, const int N,
                const int SIZE_SEQ, unsigned* seq, unsigned* size) {
    map<char, int> dna_char2int = {{'A', 0}, {'C', 1}, {'G', 2}, {'T', 3}, {'N', 0}};

    ifstream fin;
    fin.open(file_name, ios::in);
    if(fin.fail()){
            bsg_pr_info("Hey! File does not exist!\n");
            exit(1);
    }
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

  // Pack DNA sequence
  static void pack(const unsigned* unpacked, const int num_unpacked, const int num_packed, unsigned* packed) {
    for (int i = 0; i < num_packed; i++) {
      for (int j = 0; j < 16 && i * 16 + j < num_unpacked; j++) {
        int unpacked_val = unpacked[j] << (30 - 2 * j);
        packed[i] |= unpacked_val;
      }
      unpacked += 16;
    }
  }

  public:
  // Get packed data
  static void get_data_packed(const int N, const int SIZEA_MAX, const int SIZEB_MAX,
                              unsigned* seqa, unsigned* seqb,
                              unsigned* sizea, unsigned* sizeb) {
      // read N queries
      unsigned* seqa_unpacked = new unsigned[N*SIZEA_MAX]();
      read_seq("data/dna-query32.fasta", N, SIZEA_MAX, seqa_unpacked, sizea);

      // read N references
      unsigned* seqb_unpacked = new unsigned[N*SIZEB_MAX]();
      read_seq("data/dna-reference32.fasta", N, SIZEB_MAX, seqb_unpacked, sizeb);

      // pack
      int num_unpacked = N * SIZEA_MAX;
      const int SIZEA_MAX_PACKED = (SIZEA_MAX + 15) / 16;
      int num_packed = N * SIZEA_MAX_PACKED;
      unsigned* unpacked = seqa_unpacked;
      pack(unpacked, num_unpacked, num_packed, seqa);

      const int SIZEB_MAX_PACKED = (SIZEB_MAX + 15) / 16;
      num_unpacked = N * SIZEB_MAX;
      num_packed = N * SIZEB_MAX_PACKED;
      unpacked = seqb_unpacked;
      pack(unpacked, num_unpacked, num_packed, seqb);
      delete[] seqa_unpacked;
      delete[] seqb_unpacked;
  }
  };

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
        hb_mc_dimension_t tg_dim = { .x = 16, .y = 8};
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

                // == Get data
                int num_tiles = tg_dim.x * tg_dim.y;
                const int N_TILE = 4;
                const int N = N_TILE * num_tiles;
                const int SIZEA_MAX = 64;
                const int SIZEB_MAX = 64;
                const int SIZEA_MAX_PACKED = (SIZEA_MAX + 15) / 16;
                const int SIZEB_MAX_PACKED = (SIZEB_MAX + 15) / 16;
                unsigned* seqa = new unsigned[N * SIZEA_MAX_PACKED]();
                unsigned* seqb = new unsigned[N * SIZEB_MAX_PACKED]();
                unsigned* sizea = new unsigned[N];
                unsigned* sizeb = new unsigned[N];
                Sequence::get_data_packed(N, SIZEA_MAX, SIZEB_MAX, seqa, seqb, sizea, sizeb);

                // == Sending data to device
                // Define the sizes of the I/O arrays
                size_t seqa_bytes = N * SIZEA_MAX_PACKED * sizeof(unsigned);
                size_t seqb_bytes = N * SIZEB_MAX_PACKED * sizeof(unsigned);
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
                delete[] seqa;
                delete[] seqb;
                delete[] sizea;
                delete[] sizeb;

                // == Launching kernel ==
                // Define amount of work for each tile group
                /* Define tg_dim_x/y: number of tiles in each tile group */
                /* Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y */
                hb_mc_dimension_t grid_dim = { .x = 1, .y = 1};

                /* Prepare list of input arguments for kernel. */
                uint32_t cuda_argv[8] = {N_TILE, SIZEA_MAX, SIZEB_MAX,
                                         seqa_d, seqb_d, sizea_d,
                                         sizeb_d, score_d};

                /* Enque grid of tile groups, pass in grid and tile group dimensions,
                   kernel name, number and list of input arguments */
                BSG_CUDA_CALL(hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_smith_waterman", 8, cuda_argv));

                /* Launch and execute all tile groups on device and wait for all to finish.  */
                BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));

                // Transfer data device -> host
                int* score = new int[N];
                hb_mc_dma_dtoh_t dtoh_job = {
                        .d_addr = score_d,
                        .h_addr = score,
                        .size   = score_bytes
                };

                bsg_pr_test_info("Reading C to host\n");

                BSG_CUDA_CALL(hb_mc_device_dma_to_host(&device, &dtoh_job, 1));

                /* Freeze the tiles and memory manager cleanup.  */
                BSG_CUDA_CALL(hb_mc_device_program_finish(&device));

                // == Check output
                // check N scores against golden
                unsigned score_golden[N];
                ifstream fin;
                fin.open("data/output32", ios::in);
                for (int i = 0; i < N; i++) {
                  fin >> score_golden[i];
                }
                fin.close();

                // Write to file
                ofstream fout;
                fout.open("output", ios::out);
                for (int i = 0; i < N; i++) {
                  fout << score[i] << endl;
                }
                fout.close();

                // Check
                for (int i = 0; i < N; i++) {
                  if (score[i] != score_golden[i]) {
                    cout << "ERROR : mismatch for score " << i << endl;
                    return HB_MC_FAIL;
                  }
                }
                delete[] score;
        }

        BSG_CUDA_CALL(hb_mc_device_finish(&device));

        return HB_MC_SUCCESS;
}

declare_program_main("test_smith_waterman", kernel_smith_waterman);

