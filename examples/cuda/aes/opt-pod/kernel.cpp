#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_tile_group_barrier.hpp>
#include <aes.hpp>


extern "C" void aes_multigrid(struct AES_ctx *ctx, uint8_t* buf, size_t length){
	int grid_idx = (__bsg_tile_group_id_y * __bsg_grid_dim_x + __bsg_tile_group_id_x); 
        AES_CBC_encrypt_buffer(&ctx[grid_idx], &buf[grid_idx * length], length);
}
