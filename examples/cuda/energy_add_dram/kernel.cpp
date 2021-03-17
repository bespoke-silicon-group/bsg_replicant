//This kernel adds 2 vectors 

#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_tile_group_barrier.hpp>

#define bsg_saif_start() asm volatile ("addi zero,zero,1")
#define bsg_saif_end() asm volatile ("addi zero,zero,2")

bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;

extern "C" __attribute__ ((noinline))
int kernel_energy_add_dram(int *A, int *B, int *C, int N, int block_size) {

  int start_id = __bsg_id * block_size;
  bsg_saif_start();
	for (int iter_x = start_id; iter_x < start_id + block_size; iter_x++) {
		C[iter_x] = A[iter_x] + B[iter_x];
	}
  bsg_saif_end();

	barrier.sync();

	return 0;
}
