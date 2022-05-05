#include <bsg_manycore.h>
#include <bsg_cuda_lite_barrier.h>
#include <math.h>

extern "C" int kernel_dram_pointer_chase(unsigned int ** ptrs, int stride, unsigned int nels){
        unsigned int * cur;
        int start = __bsg_id * stride + 1;
        cur = ptrs[start];

        bsg_barrier_hw_tile_group_init();
        // Start profiling
        bsg_fence();
        bsg_barrier_hw_tile_group_sync();
        bsg_cuda_print_stat_kernel_start();

        do{
                cur = (unsigned int *)*cur;
        }while(cur != ptrs[start]);
        
        bsg_cuda_print_stat_kernel_end();
        bsg_barrier_hw_tile_group_sync();
        return 0;
}

