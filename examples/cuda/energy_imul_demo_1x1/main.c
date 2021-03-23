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

#define ALLOC_NAME "default_allocator"

void host_imul (uint32_t *A, uint32_t *B, uint32_t *C, int N) {
        for (int i = 0; i < N; i ++) {
                C[0] = A[i] * B[i];
        }
        return;
}

int kernel_energy_ubmark (int argc, char **argv) {
        int rc;
        char *bin_path, *test_name;
        struct arguments_path args = {NULL, NULL};

        argp_parse (&argp_path, argc, argv, 0, 0, &args);
        bin_path = args.path;
        test_name = args.name;

        bsg_pr_test_info("Running the CUDA Energy ubmark\n\n");

        srand(time);

        /*********************/
        /* Initialize device */
        /*********************/
        hb_mc_device_t device;
        BSG_CUDA_CALL(hb_mc_device_init(&device, test_name, 0));

        hb_mc_pod_id_t pod;
        hb_mc_device_foreach_pod_id(&device, pod)
        {
                /**********************************************************************/
                /* Define path to binary.                                             */
                /* Initialize device, load binary and unfreeze tiles.                 */
                /**********************************************************************/
                bsg_pr_test_info("Loading program for %s onto pod %d\n",
                                 test_name, pod);

                BSG_CUDA_CALL(hb_mc_device_set_default_pod(&device, pod));
                BSG_CUDA_CALL(hb_mc_device_program_init(&device, bin_path, ALLOC_NAME, 0));

                /*****************************************************************************************************************
                 * Allocate memory on the device for A, B and C.
                 ******************************************************************************************************************/
                const uint32_t N = 255;

                eva_t A_device, B_device, C_device;
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, N * sizeof(uint32_t), &A_device)); /* allocate A[N] on the device */
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, N * sizeof(uint32_t), &B_device)); /* allocate B[N] on the device */
                BSG_CUDA_CALL(hb_mc_device_malloc(&device, 1 * sizeof(uint32_t), &C_device)); /* allocate C[1] on the device */

                /*****************************************************************************************************************
                 * Allocate memory on the host for A & B and initialize with random values.
                 ******************************************************************************************************************/
                uint32_t A_host[] = {
                                    890.0,
                                    10700000.0,
                                    7000.0,
                                    52000.0,
                                    7.5,
                                    10800000000.0,
                                    26000000.0,
                                    40.0,
                                    5200000000.0,
                                    3100.0,
                                    3800000000.0,
                                    81000000.0,
                                    8500000.0,
                                    70000000000.0,
                                    86000000.0,
                                    4600000000.0,
                                    2200.0,
                                    380000.0,
                                    100000000000.0,
                                    33000000.0,
                                    9100000000.0,
                                    2800000000.0,
                                    7000000000.0,
                                    100000000.0,
                                    980000000.0,
                                    480.0,
                                    490000.0,
                                    610.0,
                                    56000000.0,
                                    24000000.0,
                                    810.0,
                                    71000.0,
                                    3700000000.0,
                                    89000000.0,
                                    9100.0,
                                    41.0,
                                    3600000000.0,
                                    7200000.0,
                                    46000.0,
                                    42.0,
                                    3.9,
                                    60000.0,
                                    7100000000.0,
                                    7900000000.0,
                                    61.0,
                                    100000.0,
                                    650000.0,
                                    9600000000.0,
                                    5800.0,
                                    640000000.0,
                                    2.9,
                                    10000000000.0,
                                    10000000.0,
                                    68.0,
                                    3100.0,
                                    106000000000.0,
                                    530.0,
                                    190.0,
                                    3800000.0,
                                    12000000000.0,
                                    70.0,
                                    260.0,
                                    220000.0,
                                    95000000000.0,
                                    690000000.0,
                                    1800000.0,
                                    29000.0,
                                    4.2,
                                    61000000000.0,
                                    500000.0,
                                    2100.0,
                                    3800000.0,
                                    780000000.0,
                                    61000.0,
                                    74.0,
                                    3200000000.0,
                                    760000000.0,
                                    4600.0,
                                    34.0,
                                    310000000.0,
                                    160.0,
                                    3100.0,
                                    82.0,
                                    26000000000.0,
                                    400000000.0,
                                    30000.0,
                                    9.1,
                                    68000000000.0,
                                    1200.0,
                                    1020000000.0,
                                    5600000.0,
                                    60.0,
                                    80.0,
                                    40000000000.0,
                                    8000000000.0,
                                    24000000.0,
                                    32000.0,
                                    43000.0,
                                    320000.0,
                                    2800000.0,
                                    103000000000.0,
                                    5400000000.0,
                                    100.0,
                                    71.0,
                                    970000.0,
                                    580000.0,
                                    670000000.0,
                                    9300000000.0,
                                    48000000000.0,
                                    600.0,
                                    910.0,
                                    32000000000.0,
                                    77000.0,
                                    10300000.0,
                                    320000.0,
                                    840000.0,
                                    5700000.0,
                                    70.0,
                                    130.0,
                                    170000000.0,
                                    10400000.0,
                                    630.0,
                                    7400000000.0,
                                    270.0,
                                    66000000000.0,
                                    2000000.0,
                                    10900000.0,
                                    700000000.0,
                                    1030.0,
                                    84000000000.0,
                                    53000000000.0,
                                    8700000000.0,
                                    56000.0,
                                    3100.0,
                                    1030.0,
                                    1010.0,
                                    2900000000.0,
                                    350000.0,
                                    83000000000.0,
                                    180000.0,
                                    10.0,
                                    810000000.0,
                                    4500000.0,
                                    93.0,
                                    8800000.0,
                                    1.3,
                                    770000000.0,
                                    4700000000.0,
                                    850.0,
                                    32000.0,
                                    109.0,
                                    460000.0,
                                    110000.0,
                                    500000.0,
                                    4900000.0,
                                    16.0,
                                    7200.0,
                                    29000000.0,
                                    60000000.0,
                                    59000000.0,
                                    6700000.0,
                                    5700000000.0,
                                    6100000000.0,
                                    92.0,
                                    41.0,
                                    89000000.0,
                                    910.0,
                                    180000000.0,
                                    10000.0,
                                    0.0,
                                    290000000.0,
                                    9.8,
                                    1000000.0,
                                    5800000000.0,
                                    2900.0,
                                    8600.0,
                                    420000.0,
                                    750000.0,
                                    104000000.0,
                                    91.0,
                                    19.0,
                                    1700.0,
                                    3200.0,
                                    7600000000.0,
                                    310.0,
                                    10.0,
                                    13000.0,
                                    710000000.0,
                                    4900000.0,
                                    1000000000.0,
                                    5100.0,
                                    80000.0,
                                    9.4,
                                    9400000.0,
                                    10400000000.0,
                                    8500000.0,
                                    6000.0,
                                    1040000.0,
                                    180000.0,
                                    81000000.0,
                                    820.0,
                                    2000000.0,
                                    39.0,
                                    260000.0,
                                    910000000.0,
                                    84000.0,
                                    50000.0,
                                    450000000.0,
                                    33000.0,
                                    610000000.0,
                                    96000000.0,
                                    1040000000.0,
                                    67000000000.0,
                                    10700000.0,
                                    0.0,
                                    3000000.0,
                                    8.1,
                                    7000000.0,
                                    104000000.0,
                                    50000000000.0,
                                    52000000.0,
                                    8800000.0,
                                    4.8,
                                    8800000.0,
                                    6000000.0,
                                    91.0,
                                    82000000000.0,
                                    3900000000.0,
                                    300000000.0,
                                    1600.0,
                                    11000.0,
                                    1020000.0,
                                    230.0,
                                    10100000000.0,
                                    8000000.0,
                                    190000.0,
                                    62000000.0,
                                    101000000.0,
                                    35.0,
                                    104.0,
                                    10100.0,
                                    71.0,
                                    15000.0,
                                    16000.0,
                                    690000000.0,
                                    17000000.0,
                                    800000.0,
                                    91000000.0,
                                    28000.0,
                                    150000000.0,
                                    0.0,
                                    91000.0,
                                    7600000.0,
                                    11.0,
                                    1500000.0
                }; /* allocate A[N] on the host */
                uint32_t B_host[] = {
                                    7800.0,
                                    91000000.0,
                                    7100.0,
                                    60000000.0,
                                    15000.0,
                                    54000.0,
                                    370000000.0,
                                    5.8,
                                    80000.0,
                                    8600000.0,
                                    8100.0,
                                    105000.0,
                                    4500.0,
                                    3700.0,
                                    82000000.0,
                                    170000.0,
                                    9300000.0,
                                    400.0,
                                    3400000.0,
                                    380.0,
                                    890000.0,
                                    910.0,
                                    51000000000.0,
                                    9900000000.0,
                                    6600000.0,
                                    5.7,
                                    65000000000.0,
                                    8600000.0,
                                    8800000.0,
                                    5000000.0,
                                    41000.0,
                                    2300.0,
                                    760000.0,
                                    1100000000.0,
                                    6000.0,
                                    4.5,
                                    22000000.0,
                                    10400000000.0,
                                    990000.0,
                                    770000.0,
                                    700.0,
                                    9000000000.0,
                                    980.0,
                                    30000.0,
                                    52.0,
                                    104000.0,
                                    9.8,
                                    45000000000.0,
                                    110.0,
                                    2.1,
                                    10200000000.0,
                                    80.0,
                                    4800.0,
                                    60000000.0,
                                    9100.0,
                                    56.0,
                                    5000000.0,
                                    270000.0,
                                    700000000.0,
                                    50000000000.0,
                                    7200000000.0,
                                    8100000.0,
                                    2100000000.0,
                                    8900.0,
                                    100000000.0,
                                    3900000000.0,
                                    40000000.0,
                                    4100000.0,
                                    4900.0,
                                    7100.0,
                                    200000000.0,
                                    41000000.0,
                                    9100000.0,
                                    9.0,
                                    8300.0,
                                    62000000000.0,
                                    89000000000.0,
                                    810000.0,
                                    95.0,
                                    710.0,
                                    540000000.0,
                                    28.0,
                                    10100.0,
                                    370000.0,
                                    40000000.0,
                                    52.0,
                                    360.0,
                                    66000.0,
                                    7000000.0,
                                    96000000000.0,
                                    83.0,
                                    7400000000.0,
                                    64000000.0,
                                    420000.0,
                                    10500000.0,
                                    22.0,
                                    17000000000.0,
                                    7800000000.0,
                                    7600000.0,
                                    1000.0,
                                    7.6,
                                    8000000000.0,
                                    300.0,
                                    31000.0,
                                    57000.0,
                                    610000.0,
                                    8.1,
                                    1.3,
                                    38000000.0,
                                    7000000.0,
                                    340000000.0,
                                    260000.0,
                                    690000000.0,
                                    40000000000.0,
                                    2000000000.0,
                                    44000000.0,
                                    840.0,
                                    1.8,
                                    10800000000.0,
                                    9200.0,
                                    80000000.0,
                                    28.0,
                                    0.5,
                                    79000000000.0,
                                    73000000000.0,
                                    1200000.0,
                                    3700.0,
                                    40.0,
                                    9900000000.0,
                                    2.4,
                                    23000.0,
                                    990.0,
                                    8900.0,
                                    4800000.0,
                                    7.5,
                                    310000.0,
                                    73.0,
                                    710000.0,
                                    82000.0,
                                    10.4,
                                    870000000.0,
                                    3300000000.0,
                                    13000.0,
                                    1020.0,
                                    10000000.0,
                                    70000.0,
                                    290.0,
                                    110000000.0,
                                    10.0,
                                    61000.0,
                                    770000000.0,
                                    6100.0,
                                    360000.0,
                                    78000000.0,
                                    96000.0,
                                    14000.0,
                                    3600.0,
                                    44000000000.0,
                                    7300000000.0,
                                    9600000000.0,
                                    10.7,
                                    1200000000.0,
                                    610.0,
                                    8700000.0,
                                    3.0,
                                    820000.0,
                                    4000000.0,
                                    840000000.0,
                                    650000.0,
                                    3000000.0,
                                    10000000.0,
                                    40000.0,
                                    600000000.0,
                                    1070.0,
                                    5900.0,
                                    220000.0,
                                    23000.0,
                                    57000000000.0,
                                    340000000.0,
                                    57000000000.0,
                                    66000.0,
                                    1090000000.0,
                                    8200.0,
                                    940000.0,
                                    610000000.0,
                                    200.0,
                                    1040.0,
                                    620000.0,
                                    2200.0,
                                    580000000.0,
                                    460000000.0,
                                    6900.0,
                                    700000000.0,
                                    74000000000.0,
                                    270000000.0,
                                    2000000000.0,
                                    109000.0,
                                    4100.0,
                                    2400.0,
                                    110000.0,
                                    420.0,
                                    1040000000.0,
                                    5.5,
                                    410000.0,
                                    0.1,
                                    1400.0,
                                    10.5,
                                    64000.0,
                                    1070.0,
                                    8000.0,
                                    1200000.0,
                                    57000000000.0,
                                    75000000000.0,
                                    280000.0,
                                    950000.0,
                                    240.0,
                                    410000000.0,
                                    3.9,
                                    1000000.0,
                                    920000.0,
                                    103000000000.0,
                                    4400.0,
                                    0.4,
                                    5.0,
                                    8.6,
                                    930.0,
                                    23000000000.0,
                                    10.0,
                                    10400000.0,
                                    9400000000.0,
                                    8300.0,
                                    106000000.0,
                                    78000000000.0,
                                    600.0,
                                    57000000.0,
                                    4400000.0,
                                    860.0,
                                    55000000.0,
                                    8.2,
                                    3000000000.0,
                                    1070000000.0,
                                    880000.0,
                                    270000000.0,
                                    76000000.0,
                                    650.0,
                                    48000000000.0,
                                    3200.0,
                                    7200.0,
                                    9700000000.0,
                                    81000000000.0,
                                    101000000000.0,
                                    89000.0,
                                    56000000000.0,
                                    180.0,
                                    4.5
                }; /* allocate B[N] on the host */

                /*****************************************************************************************************************
                 * Copy A from host onto device DRAM.
                 ******************************************************************************************************************/
                void *dst = (void *) ((intptr_t) A_device);
                void *src = (void *) &A_host[0];
                BSG_CUDA_CALL(hb_mc_device_memcpy (&device, dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE)); /* Copy A to the device  */

                dst = (void *) ((intptr_t) B_device);
                src = (void *) &B_host[0];
                BSG_CUDA_CALL(hb_mc_device_memcpy (&device, dst, src, N * sizeof(uint32_t), HB_MC_MEMCPY_TO_DEVICE)); /* Copy A to the device  */

                /*****************************************************************************************************************
                 * Define block_size_x/y: amount of work for each tile group
                 * Define tg_dim_x/y: number of tiles in each tile group
                 * Calculate grid_dim_x/y: number of tile groups needed based on block_size_x/y
                 ******************************************************************************************************************/
                hb_mc_dimension_t tg_dim = { .x = 1, .y = 1 };
                hb_mc_dimension_t grid_dim = { .x = 1, .y = 1 };

                /*****************************************************************************************************************
                 * Prepare list of input arguments for kernel.
                 ******************************************************************************************************************/
                int cuda_argv[4] = {A_device, B_device, C_device, N};

                /*****************************************************************************************************************
                 * Enquque grid of tile groups, pass in grid and tile group dimensions, kernel name, number and list of input arguments
                 ******************************************************************************************************************/
                BSG_CUDA_CALL(hb_mc_kernel_enqueue (&device, grid_dim, tg_dim, test_name, 4, cuda_argv));

                /*****************************************************************************************************************
                 * Launch and execute all tile groups on device and wait for all to finish.
                 ******************************************************************************************************************/
                BSG_CUDA_CALL(hb_mc_device_tile_groups_execute(&device));

                /*****************************************************************************************************************
                 * Copy result matrix back from device DRAM into host memory.
                 ******************************************************************************************************************/
                uint32_t C_host[1];
                src = (void *) ((intptr_t) C_device);
                dst = (void *) &C_host[0];
                BSG_CUDA_CALL(hb_mc_device_memcpy (&device, (void *) dst, src, 1 * sizeof(uint32_t), HB_MC_MEMCPY_TO_HOST)); /* copy C to the host */

                /*****************************************************************************************************************
                 * Freeze the tiles and memory manager cleanup.
                 ******************************************************************************************************************/
                BSG_CUDA_CALL(hb_mc_device_program_finish(&device));

                /*****************************************************************************************************************
                 * Calculate the expected result using host code and compare the results.
                 ******************************************************************************************************************/
                uint32_t C_expected[1];
                host_imul (A_host, B_host, C_expected, N);

                if (C_expected[0] != C_host[0]) {
                        bsg_pr_err(BSG_RED("Mismatch: ") "result = %d but expected %d\n", C_host[0], C_expected[0]);
                        return HB_MC_FAIL;
                }

                bsg_pr_test_info("result = %d but expected %d\n", C_host[0], C_expected[0]);
        }

        BSG_CUDA_CALL(hb_mc_device_finish(&device));

        return HB_MC_SUCCESS;
}

#ifdef VCS
int vcs_main(int argc, char ** argv)
#else
int main(int argc, char ** argv)
#endif
{
        bsg_pr_test_info("test_vec_add_parallel Regression Test \n");
        int rc = kernel_energy_ubmark(argc, argv);
        bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
        return rc;
}


