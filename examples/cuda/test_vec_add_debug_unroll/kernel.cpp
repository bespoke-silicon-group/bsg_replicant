//This kernel adds 2 vectors

#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_tile_group_barrier.hpp>

//bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;
extern "C" __attribute__ ((noinline))
int kernel_vec_add_parallel(bsg_attr_remote const int *A, bsg_attr_remote const int *B, bsg_attr_remote int *C,
                            int N, int block_size_x, int curr_limit) {

    asm volatile ("csrw 0xfc0, %[curr_limit]" : : [curr_limit] "r" (curr_limit));
    #pragma bsg_unroll(32)
	for (int iter_x = 0; iter_x < block_size_x; iter_x += 1) {
		C[iter_x] = A[iter_x] + B[iter_x];
	}

	// barrier.sync();

	return 0;
}
