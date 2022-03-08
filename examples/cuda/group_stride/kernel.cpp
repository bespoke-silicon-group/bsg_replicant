#include <bsg_set_tile_x_y.h>
#include <bsg_cuda_lite_barrier.h>
#include <bsg_group_strider.hpp>
#include <math.h>

// Runs a simple test of the BSG Tile group strider. Each tile creates
// a strider object for the horizontal and vertical direction, and
// then strides once to read the neighboring __bsg_x/y variable.

// Results are written to DRAM and verified by the host.

extern "C" int kernel_group_stride(int *nx, int *ny){

        bsg_barrier_hw_tile_group_init();
        // Start profiling
        bsg_fence();
        bsg_barrier_hw_tile_group_sync();

        bsg_tile_group_strider<BSG_TILE_GROUP_X_DIM, 1, BSG_TILE_GROUP_Y_DIM, 0, int> stride_x(&__bsg_x, __bsg_x, __bsg_y);
        bsg_tile_group_strider<BSG_TILE_GROUP_X_DIM, 0, BSG_TILE_GROUP_Y_DIM, 1, int> stride_y(&__bsg_y, __bsg_x, __bsg_y);
        nx[__bsg_y * BSG_TILE_GROUP_X_DIM + __bsg_x] = *stride_x.stride();
        ny[__bsg_y * BSG_TILE_GROUP_X_DIM + __bsg_x] = *stride_y.stride();
        bsg_barrier_hw_tile_group_sync();

        return 0;
}

