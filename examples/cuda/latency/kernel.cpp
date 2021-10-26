//This kernel adds 2 vectors

#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_tile_group_barrier.hpp>

extern "C" __attribute__ ((noinline))
int kernel_latency(int bsg_attr_remote * bsg_attr_noalias A, unsigned int stride, unsigned int limit) {

        int sum = 0;
        bsg_cuda_print_stat_kernel_start();

        bsg_unroll(1)
        for (int i = 0; i < limit; i += stride){
                sum += A[i];
                bsg_compiler_memory_barrier();
        }
        bsg_cuda_print_stat_kernel_end();
        A[0] = sum;
	return 0;
}
