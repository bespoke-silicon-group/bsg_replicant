//This kernel adds 2 vectors

#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_tile_group_barrier.hpp>

extern "C" __attribute__ ((noinline))
int kernel_vec_add_parallel(float bsg_attr_remote * bsg_attr_noalias A, float bsg_attr_remote * bsg_attr_noalias B, float bsg_attr_remote * bsg_attr_noalias C,
                            int nblocks, int limit) {

        asm volatile ("csrw 0xfc0, %[limit]" : : [limit] "r" (limit));
        for (int blk_i = 0; blk_i < nblocks; blk_i += 1) {
                bsg_unroll(32)
                for (int i = 0; i < 32; i += 1) {
                        C[i + (blk_i * 32)] = A[i + (blk_i * 32)] + B[i + (blk_i * 32)];
                }
        }

	return 0;
}
