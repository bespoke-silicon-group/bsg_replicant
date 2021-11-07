#pragma once
#include "bsg_tile_config_vars.h"

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
        return tile_y == bsg_global_Y/2 || tile_y == 1 + bsg_global_Y/2;
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
}
