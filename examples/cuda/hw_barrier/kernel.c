#include "bsg_manycore.h"
#include "bsg_cuda_lite_hw_barrier.h"
#include "bsg_barrier_amoadd.h"

int barrier_test()
{
    bsg_barrier_hw_tile_group_init();
    for (int x = 0; x < bsg_tiles_X; x++) {
        for (int y = 0; y < bsg_tiles_Y; y++) {
            if (__bsg_x == x && __bsg_y == y) {
                int *ptr = (int*)bsg_remote_ptr_io(IO_X_INDEX, 0x8888);
                *ptr = (__bsg_x<<16)|(__bsg_y);
            }
            bsg_barrier_hw_tile_group_sync();
        }
    }

    return 0;
}
