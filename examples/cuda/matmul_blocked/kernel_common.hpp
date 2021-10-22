#ifndef _KERNEL_COMMON_H
#define _KERNEL_COMMON_H

#include <cstring>
#include <cstdint>
#include <math.h>

// BSG_TILE_GROUP_X_DIM and BSG_TILE_GROUP_Y_DIM must be defined
// before bsg_manycore.h and bsg_tile_group_barrier.h are
// included.
#define BSG_TILE_GROUP_X_DIM bsg_tiles_X
#define BSG_TILE_GROUP_Y_DIM bsg_tiles_Y
#include <bsg_manycore.h>
#include <bsg_cuda_lite_barrier.h>
#include <hb_tensor.hpp>
#include <hb_assert.hpp>
#include <hb_common.hpp>

bsg_attr_remote void* hb_memcpy(bsg_attr_remote void* bsg_attr_noalias dest,
                         const bsg_attr_remote void* bsg_attr_noalias src,
                         size_t n);


#endif // _KERNEL_COMMON_H
