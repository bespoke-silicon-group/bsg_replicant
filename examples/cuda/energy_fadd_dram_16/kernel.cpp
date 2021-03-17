//This kernel adds 2 vectors 

#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_tile_group_barrier.hpp>

bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;

extern "C" __attribute__ ((noinline))
int kernel_energy_fadd_dram(float *A, float *B, float *C, int N, int block_size) {

  int start_id = __bsg_id * block_size;
  bsg_saif_start();
	for (int iter_x = start_id; iter_x < start_id + block_size; iter_x+=16) {
		C[iter_x + 0]  = A[iter_x + 0]  + B[iter_x + 0];
		C[iter_x + 1]  = A[iter_x + 1]  + B[iter_x + 1];
		C[iter_x + 2]  = A[iter_x + 2]  + B[iter_x + 2];
		C[iter_x + 3]  = A[iter_x + 3]  + B[iter_x + 3];
		C[iter_x + 4]  = A[iter_x + 4]  + B[iter_x + 4];
		C[iter_x + 5]  = A[iter_x + 5]  + B[iter_x + 5];
		C[iter_x + 6]  = A[iter_x + 6]  + B[iter_x + 6];
		C[iter_x + 7]  = A[iter_x + 7]  + B[iter_x + 7];
		C[iter_x + 8]  = A[iter_x + 8]  + B[iter_x + 8];
		C[iter_x + 9]  = A[iter_x + 9]  + B[iter_x + 9];
		C[iter_x + 10] = A[iter_x + 10] + B[iter_x + 10];
		C[iter_x + 11] = A[iter_x + 11] + B[iter_x + 11];
		C[iter_x + 12] = A[iter_x + 12] + B[iter_x + 12];
		C[iter_x + 13] = A[iter_x + 13] + B[iter_x + 13];
		C[iter_x + 14] = A[iter_x + 14] + B[iter_x + 14];
		C[iter_x + 15] = A[iter_x + 15] + B[iter_x + 15];
	}
  bsg_saif_end();

	barrier.sync();

	return 0;
}
