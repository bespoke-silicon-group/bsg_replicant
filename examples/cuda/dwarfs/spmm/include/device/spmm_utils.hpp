#pragma once
#include <cstdint>
#include "bsg_tile_config_vars.h"
#include "bsg_manycore.hpp"

namespace utils
{
    /**
     * Returns true if tile is south side of chip
     */
    static inline int is_south_not_north(int tile_y)
    {
        return tile_y/(bsg_global_Y/2);
    }

    static inline int x()
    {
        return __bsg_x;
    }

    static inline int y()
    {
        return __bsg_y;
    }

    static inline int is_cache_adjacent(int tile_y)
    {
        return tile_y == 0 || tile_y == bsg_global_Y-1;
    }

    static inline int is_center(int tile_y)
    {
        return tile_y == (bsg_global_Y/2)-1 || (tile_y == bsg_global_Y/2);
    }

    /**
     * Returns the neighbor closer to the center, or -1 if none exists
     */
    static inline int inner_neighbor_y_to(int tile_y)
    {
        if (is_center(tile_y)) {
            return -1;
        } else if (is_south_not_north(tile_y)) {
            return tile_y-1;
        } else {
            return tile_y+1;
        }
    }

    /**
     * check if memory is in dram
     */
    template <typename T>
    static inline int is_dram(T *tp)
    {
        intptr_t ptr = reinterpret_cast<intptr_t>(tp);
        return ptr & 0x80000000;
    }

    /**
     * check if memory is in tile-group shared
     */
    template <typename T>
    static inline int is_tile_group(T *tp)
    {
        intptr_t ptr = reinterpret_cast<intptr_t>(tp);
        return ptr & 0xe0000000 == 0x20000000;
    }

    /**
     * check if memory is in global tile memory
     */
    template <typename T>
    static inline int is_tile_global(T *tp)
    {
        intptr_t ptr = reinterpret_cast<intptr_t>(tp);
        return ptr & 0xc0000000 == 0x40000000;
    }

    /**
     * check if memory is in tile local memory
     */
    template <typename T>
    static inline int is_tile_local(T *tp)
    {
        intptr_t ptr = reinterpret_cast<intptr_t>(tp);        
        return !(ptr & 0xe0000000);
    }

    /**
     * make local pointer a tile group pointer;
     * pointing to this tile's memory
     */
    template<typename PtrType>
    static inline PtrType tile_group_pointer(PtrType ptr) {
        return bsg_tile_group_remote_pointer(utils::x(), utils::y(), ptr);
    }
}

/**
 * Number of elements in static array
 */
#define ARRAY_SIZE(x)                              \
    (sizeof(x)/sizeof(x[0]))
