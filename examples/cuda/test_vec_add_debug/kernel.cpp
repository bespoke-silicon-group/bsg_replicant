//This kernel adds 2 vectors

#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_tile_group_barrier.hpp>

//bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;

int kernel_vec_add_parallel(int *A, int *B, int *C, int N, int block_size_x) {

	for (int iter_x = 0; iter_x < block_size_x; iter_x += 1) {
		C[iter_x] = A[iter_x] + B[iter_x];
	}

	// barrier.sync();

	return 0;
}
