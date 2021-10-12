#pragma once
#include "bsg_manycore.h"

namespace barrier {
    extern "C" void bsg_barrier_amoadd(int *lock, int *sense);

    extern int lock;
    extern int sense;
#ifdef CHECK_BARRIER
    extern int checkpoint;
#endif
    static inline void spmm_barrier()
    {
#ifdef CHECK_BARRIER
        bsg_print_int(checkpoint++);
#endif
        bsg_barrier_amoadd(&lock, &sense);
#ifdef CHECK_BARRIER
        bsg_print_int(checkpoint++);
#endif        
    }
}
