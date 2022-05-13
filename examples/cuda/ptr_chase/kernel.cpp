#include <bsg_manycore.h>
#include <bsg_cuda_lite_barrier.h>
#include <math.h>

#ifndef LATENCY_WORK
#define LATENCY_WORK 1
#endif

extern "C" int kernel_dram_pointer_chase(unsigned int volatile ** ptrs, int stride, unsigned int niters){
        volatile unsigned int * cur[LATENCY_WORK];
        int start = (1 + __bsg_id * (stride * LATENCY_WORK)) % (1<<ARR_LOG2_NUM_ELEMENTS);

        // LATENCY_WORK is how much we will unroll the inner loop to hide latency
        // This loop sets the start point for each work item.
        bsg_unroll(100)
        for (int w = 0; w < LATENCY_WORK; ++w){
                cur[w] = ptrs[start + w];
        }

        bsg_barrier_hw_tile_group_init();
        // Start profiling
        bsg_fence();
        bsg_barrier_hw_tile_group_sync();
        bsg_cuda_print_stat_kernel_start();

        for(int iter = 0; iter < niters; ++iter){
                // Unroll to amortize loop overhead
                // Divide by LATENCY_WORK so we don't overflow the icache
                bsg_unroll(128)
                for(int unroll = 0; unroll < 128/LATENCY_WORK; ++unroll){
                        bsg_unroll(100)
                        for (int w = 0; w < LATENCY_WORK; ++w){
                                cur[w] = (unsigned int *)*cur[w];
                        }
                }
        }
        
        bsg_cuda_print_stat_kernel_end();
        bsg_barrier_hw_tile_group_sync();
        return 0;
}

