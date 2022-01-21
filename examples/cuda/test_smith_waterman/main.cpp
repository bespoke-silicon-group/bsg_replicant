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

                ifstream f_ref, f_query;
                string ref_str, query_str, num;
                map<char, int> dna_map = {
                  {'A', 0}, {'C', 1}, {'G', 2}, {'T', 3}
                };
                const int N = 1024;
                int *n1 = new int[N];
                int *n2 = new int[N];
                vector<int> ref_vec, query_vec;

                // read sequences from file
                f_ref.open("data/dna-reference.fasta", ios::in);
                f_query.open("data/dna-query.fasta", ios::in);

                const int SEQ_LEN = 256;
                int *ref = new int[N*SEQ_LEN];
                int *query = new int[N*SEQ_LEN];

                for (int n = 0; n < N; n++) {
                  f_ref >> num >> ref_str;
                  f_query >> num >> query_str;

                  n1[n] = ref_str.length();
                  n2[n] = query_str.length();

                  for (int i = 0; i < n1[n]; i++) {
                    int base_pair = dna_map[ref_str[i]];
                    ref[n*SEQ_LEN+i] = base_pair;
                  }

                  for (int i = 0; i < n2[n]; i++) {
                    int base_pair = dna_map[query_str[i]];
                    query[n*SEQ_LEN+i] = base_pair;
                  }

                }

                f_ref.close();
                f_query.close();

                /* Allocate memory on the device for A, B and C. */
                int sm_size = (256 + 1) * (256 + 1);
                size_t vsize0 = N * 256 * sizeof(int);
                size_t vsize1 = N * 256 * sizeof(int);
                size_t vsize2 = N * sm_size * sizeof(int);
                size_t vsize3 = N * sizeof(int);

                eva_t ref_device, query_device, score_matrix_device, n1_device, n2_device;
                /* allocate A[N] on the device */
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, vsize0, &ref_device));
                 /* allocate B[N] on the device */
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, vsize1, &query_device));
                 /* allocate C[N] on the device */
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, vsize2, &score_matrix_device));

                BSG_CUDA_CALL(hb_mc_device_malloc(&device, vsize3, &n1_device));

                BSG_CUDA_CALL(hb_mc_device_malloc(&device, vsize3, &n2_device));

                /* Copy A & B from host onto device DRAM. */
                hb_mc_dma_htod_t htod_jobs [] = {
                        {
                                .d_addr = ref_device,
                                .h_addr = ref,
                                .size   = vsize0
                        },
                        {
                                .d_addr = query_device,
                                .h_addr = query,
                                .size   = vsize1
                        },
                        {
                                .d_addr = n1_device,
                                .h_addr = n1,
                                .size   = vsize3
                        },
                        {
                                .d_addr = n2_device,
                                .h_addr = n2,
                                .size   =  vsize3
                        }
                };

                bsg_pr_test_info("Writing A and B to device\n");

                BSG_CUDA_CALL(hb_mc_device_dma_to_device(&device, htod_jobs, 4));

                /* Define block_size_x/y: amount of work for each tile group */
                /* Define tg_dim_x/y: number of tiles in each tile group */
                /* Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y */
                hb_mc_dimension_t grid_dim = { .x = 1, .y = 1};


                /* Prepare list of input arguments for kernel. */
                // N1 is the number of alignments per tile while N is the numeber of total alignments
                int N1 = 8;
                uint32_t cuda_argv[6] = {ref_device, query_device, score_matrix_device, n1_device, n2_device, N1};

                /* Enqqueue grid of tile groups, pass in grid and tile group dimensions,
                   kernel name, number and list of input arguments */
                BSG_CUDA_CALL(hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, "kernel_smith_waterman", 6, cuda_argv));

                /* Launch and execute all tile groups on device and wait for all to finish.  */
                BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));

                /* Copy result matrix back from device DRAM into host memory.  */
                int* score_matrix_host = new int[N*sm_size]();

                hb_mc_dma_dtoh_t dtoh_job = {
                        .d_addr = score_matrix_device,
                        .h_addr = score_matrix_host,
                        .size   = vsize2
                };

                bsg_pr_test_info("Reading C to host\n");

                BSG_CUDA_CALL(hb_mc_device_dma_to_host(&device, &dtoh_job, 1));

                /* Calculate the expected result using host code and compare the results.  */
                //if (mismatch)
                        //return HB_MC_FAIL;

                /* Freeze the tiles and memory manager cleanup.  */
                BSG_CUDA_CALL(hb_mc_device_program_finish(&device));

#ifdef PRINT_MATRIX
                for (int i = 0; i < *n1_host + 1; i++) {
                  for (int j = 0; j < *n2_host + 1; j++) {
                    printf("%d\t", score_matrix_host[(*n2_host+1)*i+j]);
                  }
                  printf("\n");
                }
#endif

#ifdef PRINT_SCORE
  int score;
  ofstream fout;
  fout.open("output", ios::out);
  for (int n = 0; n < N; n++) {
    score = 0;
    for (int i = 0; i < n1[n]; i++) {
      for (int j = 0; j < n2[n]; j++) {
        if (score_matrix_host[sm_size*n+256*i+j] > score)
          score = score_matrix_host[sm_size*n+256*i+j];
      }
    }
    fout << score << endl;
  }
  fout.close();
#endif

//#ifdef PRINT_SCORE
                //int score = -1;
                //for (int i = 0; i < *n1_host + 1; i++) {
                  //for (int j = 0; j < *n2_host + 1; j++) {
                    //if (score_matrix_host[(*n2_host+1)*i+j] > score)
                      //score = score_matrix_host[(*n2_host+1)*i+j];
                  //}
                //}
                //printf("Score = %d\n", score);
//#endif

                delete [] score_matrix_host;
        }

        BSG_CUDA_CALL(hb_mc_device_finish(&device));

        return HB_MC_SUCCESS;
}

declare_program_main("test_smith_waterman", kernel_smith_waterman);

