#pragma once
#include "bsg_manycore.h"
#include "bsg_cuda_lite_barrier.h"

namespace barrier {
    extern "C" void bsg_barrier_amoadd(int *lock, int *sense);

#ifdef CHECK_BARRIER
    extern int checkpoint;
#endif
    static inline void spmm_barrier()
    {
#ifdef CHECK_BARRIER
        bsg_print_int(checkpoint++);
#endif
        bsg_fence();
        bsg_barrier_hw_tile_group_sync();
        bsg_fence();
#ifdef CHECK_BARRIER
        bsg_print_int(checkpoint++);
#endif        
    }
    static inline void spmm_barrier_init()
    {
        bsg_barrier_hw_tile_group_init();
    }
}
