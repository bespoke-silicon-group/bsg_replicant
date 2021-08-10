#include "bsg_manycore.h"
#include "bsg_tile_group_barrier.hpp"

bsg_barrier<bsg_tiles_X,bsg_tiles_Y> barrier;

extern "C" int reset_barrier()
{
    barrier.reset();
    return 0;
}

__attribute__((noinline)) static int stride_internal(int **A, int x, int y, int loads_per_core)
{
    int k = 0;
    int *p = reinterpret_cast<int*>(A);
    do {
        p = *reinterpret_cast<int**>(p);
        k++;
    } while (k < loads_per_core);
    return reinterpret_cast<int>(p);
}

extern "C" int stride(int **A, int x, int y, int loads_per_core)
{
    int r = 0;
    bsg_cuda_print_stat_kernel_start();
    if (bsg_x == x
        && bsg_y == y) {
        r = stride_internal(A, x, y, loads_per_core);        
    }
    barrier.sync();
    bsg_cuda_print_stat_kernel_end();    
    return r;
}
