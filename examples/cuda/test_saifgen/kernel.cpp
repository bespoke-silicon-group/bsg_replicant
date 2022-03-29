/*
 * This kernel performs nothing. It is intended to test the saif generator.
 */

#include <bsg_manycore.h>
#include <bsg_cuda_lite_barrier.h>

extern "C" int  __attribute__ ((noinline)) kernel_saifgen(int arc) {
        int rc = 0;

        bsg_barrier_hw_tile_group_init();
        // Start profiling
        bsg_fence();
        bsg_barrier_hw_tile_group_sync();

        bsg_nonsynth_saif_start();
        for(volatile int i = 0; i <= 1000; ++i);
        bsg_nonsynth_saif_end();

        bsg_barrier_hw_tile_group_sync();

        return rc;
}
