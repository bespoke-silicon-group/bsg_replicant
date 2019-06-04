#ifndef BSG_MANYCORE_COORDINATE_H
#define BSG_MANYCORE_COORDINATE_H
#include <bsg_manycore_features.h>

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
#include <cinttypes>
#include <cstdio>
#else
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum __hb_mc_coordinate_limit_t {
        HB_MC_COORDINATE_MIN = 0,
        HB_MC_COORDINATE_MAX = 32
} hb_mc_coordinate_limit_t;

typedef uint32_t hb_mc_idx_t;

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

#define HB_MC_DIMENSION(xv, yv)					\
	{.x = xv, .y = yv}

static inline hb_mc_coordinate_t hb_mc_coordinate(hb_mc_idx_t x, hb_mc_idx_t y)
{
	hb_mc_coordinate_t ret = HB_MC_COORDINATE(x,y);
	return ret;
}

static inline hb_mc_dimension_t hb_mc_dimension(hb_mc_idx_t x, hb_mc_idx_t y)
{
	hb_mc_dimension_t ret = HB_MC_DIMENSION(x,y);
	return ret;
}

/**
 * Format a coordinate as a human readable string.
 * @param[in] coordinate  A coordinate to format as a string.
 * @param[in] buf         A buffer in which to format the string.
 * @param[in] sz          The size of buf.
 * @return Returns a pointer to #buf as a human readable string for #coordinate.
 */
static inline const char *hb_mc_coordinate_to_string(hb_mc_coordinate_t coordinate,
						     char *buf, size_t sz)
{
	if (!buf)
		return buf;

	snprintf(buf, sz, "(%" PRId8 ",%" PRId8 ")",
		 hb_mc_coordinate_get_x(coordinate),
		 hb_mc_coordinate_get_y(coordinate));

	return buf;
}

#ifdef __cplusplus
}
#endif
#endif
