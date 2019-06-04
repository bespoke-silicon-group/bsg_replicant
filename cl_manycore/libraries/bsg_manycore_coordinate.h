#ifndef BSG_MANYCORE_COORDINATE_H
#define BSG_MANYCORE_COORDINATE_H
#include <bsg_manycore_features.h>

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum __hb_mc_coordinate_limit_t {
        HB_MC_COORDINATE_MIN = 0,
        HB_MC_COORDINATE_MAX = 32
} hb_mc_coordinate_limit_t;

typedef int32_t hb_mc_idx_t;

typedef struct __hb_mc_coordinate_t{
        hb_mc_idx_t x;
        hb_mc_idx_t y;
} hb_mc_coordinate_t;

typedef hb_mc_coordinate_t hb_mc_dimension_t;

static inline hb_mc_idx_t hb_mc_coordinate_get_x(hb_mc_coordinate_t coordinate)
{
        return coordinate.x;
}

static inline hb_mc_coordinate_t hb_mc_coordinate_set_x(hb_mc_coordinate_t coordinate, hb_mc_idx_t x)
{
        coordinate.x = x;
        return coordinate;
}

static inline hb_mc_idx_t hb_mc_coordinate_get_y(hb_mc_coordinate_t coordinate)
{
        return coordinate.y;
}

static inline hb_mc_coordinate_t hb_mc_coordinate_set_y(hb_mc_coordinate_t coordinate, hb_mc_idx_t y)
{
        coordinate.y = y;
        return coordinate;
}

static inline hb_mc_idx_t hb_mc_dimension_get_x(hb_mc_dimension_t dim)
{
        return dim.x;
}

static inline hb_mc_idx_t hb_mc_dimension_get_y(hb_mc_dimension_t dim)
{
        return dim.y;
}

#define HB_MC_COORDINATE(xv, yv)					\
	{.x = xv, .y = yv}

static inline hb_mc_coordinate_t hb_mc_coordinate(hb_mc_idx_t x, hb_mc_idx_t y)
{
	hb_mc_coordinate_t ret = HB_MC_COORDINATE(x,y);
	return ret;
}

#ifdef __cplusplus
}
#endif
#endif
