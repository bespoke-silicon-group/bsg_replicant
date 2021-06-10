//This kernel adds 2 vectors

#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_tile_group_barrier.hpp>

extern "C" __attribute__ ((noinline))
int kernel_vec_add_parallel(float bsg_attr_remote * bsg_attr_noalias A, float bsg_attr_remote * bsg_attr_noalias B, float bsg_attr_remote * bsg_attr_noalias C,
                            int nblocks, int limit) {

        asm volatile ("csrw 0xfc0, %[limit]" : : [limit] "r" (limit));
        for (int blk_i = 0; blk_i < nblocks; blk_i += 1) {
                register float a_temp[16], b_temp[16];
                bsg_unroll(16)
                for (int i = 0; i < 16; i += 1) {
                        a_temp[i] = A[i + (blk_i * 16)];
                        b_temp[i] = B[i + (blk_i * 16)];
                }
                __asm__ __volatile__ ("" : : : "memory");
                bsg_unroll(16)
                for (int i = 0; i < 16; i += 1) {
                        C[i + (blk_i * 16)] =  a_temp[i] + b_temp[i];
                }
        }

	return 0;
}
