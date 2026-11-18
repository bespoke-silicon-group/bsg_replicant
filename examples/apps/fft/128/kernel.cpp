// 128x128 FFT using four-step method
// In this implementation all tiles communicate through scratchpad stores

#include <bsg_manycore.h>
#include <bsg_cuda_lite_barrier.h>

/* #include "bsg_tile_group_barrier.hpp" */

#define FFT128
#include <fft.hpp>

/* bsg_barrier<bsg_tiles_X, bsg_tiles_Y> tg_barrier; */

FP32Complex fft_workset[NUM_POINTS];

extern "C" __attribute__ ((noinline))
int
kernel_fft(FP32Complex *in, FP32Complex *out, float bsg_attr_remote * bsg_attr_noalias tw, int N) {


    bsg_barrier_hw_tile_group_init();

    bsg_cuda_print_stat_kernel_start();
    // TODO: Prefetch Source
    bsg_cuda_print_stat_start(1);
    load_fft_scale_no_twiddle(in, fft_workset, tw, __bsg_id, 128, 128, N);
    bsg_cuda_print_stat_end(1);

    asm volatile("": : :"memory");
    bsg_barrier_hw_tile_group_sync();


    bsg_cuda_print_stat_start(2);
    tg_mem_square_transpose_inp(fft_workset, 128);
    bsg_cuda_print_stat_end(2);

    // TODO: Prefetch Destination
    asm volatile("": : :"memory");
    bsg_barrier_hw_tile_group_sync();


    bsg_cuda_print_stat_start(3);
    fft_store(fft_workset, out, __bsg_id, 128, 128, N);
    bsg_cuda_print_stat_end(3);

    bsg_fence();

    asm volatile("": : :"memory");
    bsg_barrier_hw_tile_group_sync();


    bsg_cuda_print_stat_kernel_end();


    asm volatile("": : :"memory");
    bsg_barrier_hw_tile_group_sync();


    return 0;
}
