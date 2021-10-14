#include "spmm_barrier.hpp"

namespace barrier {
    __attribute__((section(".dram")))
    int lock = 0;
    int sense = 1;
    int locksel = 0;
#ifdef CHECK_BARRIER
    int checkpoint = 0;
#endif
}
