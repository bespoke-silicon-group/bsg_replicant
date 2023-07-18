#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_tile_group_barrier.hpp>

#define LUT_LENGTH 16

bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;
bsg_tile_group_shared_mem(uint16_t, LUT, LUT_LENGTH);

extern "C" __attribute__ ((noinline))
int kernel_mulh_inference() {
    for(uint8_t i=0; i<LUT_LENGTH; i++) { 
        bsg_tile_group_shared_store(uint16, LUT, i, i);
    }
    barrier.sync();

    return 0;
}
