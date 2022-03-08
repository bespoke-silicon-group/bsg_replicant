#include <bsg_manycore.h>
#include <bsg_cuda_lite_barrier.h>
#include <aes.hpp>

extern "C" void aes_singlegrid(struct AES_ctx *ctx, uint8_t* buf, size_t length, int niters){
	int tg_idx = (bsg_tiles_X * __bsg_y + __bsg_x);
        bsg_barrier_hw_tile_group_init();
        bsg_barrier_hw_tile_group_sync();
        bsg_cuda_print_stat_kernel_start();

        for(int i = 0 ; i < niters; ++i){
                AES_CBC_encrypt_buffer(&ctx[tg_idx * niters + i], &buf[tg_idx * niters * length + length * i], length);
        }

        bsg_cuda_print_stat_kernel_end();
        bsg_fence();
        bsg_barrier_hw_tile_group_sync();
}
