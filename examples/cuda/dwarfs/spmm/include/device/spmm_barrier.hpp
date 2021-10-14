#pragma once
#include "bsg_manycore.h"

namespace barrier {
    extern "C" void bsg_barrier_amoadd(int *lock, int *sense);

    extern int lock;
    extern int sense;
    extern int locksel;
#ifdef CHECK_BARRIER
    extern int checkpoint;
#endif
    static inline void spmm_barrier()
    {
#ifdef CHECK_BARRIER
        bsg_print_int(checkpoint++);
#endif
        bsg_fence();
        bsg_barrier_amoadd(&lock, &sense);
        bsg_fence();
#ifdef CHECK_BARRIER
        bsg_print_int(checkpoint++);
#endif        
    }
}
