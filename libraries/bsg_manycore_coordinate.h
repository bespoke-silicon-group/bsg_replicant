// Copyright (c) 2019, University of Washington All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list
// of conditions and the following disclaimer.
// 
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
// 
// Neither the name of the copyright holder nor the names of its contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

#define HB_MC_COORDINATE(xv, yv)                \
        {.x = xv, .y = yv}

#define HB_MC_DIMENSION(xv, yv)                 \
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

        /**
         * Calculates and returns the relative coordinates based on absolute coordinates and origin coordinates 
         * @param[in]  origin        Origin coordinates 
         * @parma[in]  coord         Absolute coordinates 
         * @return     relative_coord
         */
        __attribute__((warn_unused_result))
        static hb_mc_coordinate_t hb_mc_coordinate_get_relative (hb_mc_coordinate_t origin, hb_mc_coordinate_t coord) {
                hb_mc_coordinate_t relative_coord = hb_mc_coordinate (  hb_mc_coordinate_get_x (coord) - hb_mc_coordinate_get_x (origin) , 
                                                                        hb_mc_coordinate_get_y (coord) - hb_mc_coordinate_get_y (origin) );
                return relative_coord;
        }

        /**
         * Calculates and returns the absolute coordinates based on relative coordinates and origin coordinates 
         * @param[in]  origin        Origin coordinates 
         * @parma[in]  coord         Relative coordinates 
         * @return     absolute_coord
         */
        __attribute__((warn_unused_result))
        static hb_mc_coordinate_t hb_mc_coordinate_get_absolute (hb_mc_coordinate_t origin, hb_mc_coordinate_t coord) {
                hb_mc_coordinate_t absolute_coord = hb_mc_coordinate (  hb_mc_coordinate_get_x (coord) + hb_mc_coordinate_get_x (origin) , 
                                                                        hb_mc_coordinate_get_y (coord) + hb_mc_coordinate_get_y (origin) );
                return absolute_coord;
        }

        /**
         * Calculates and returns a 1D flat index based on 2D coordinates and 2D dimensions 
         * @param[in]  coord         2D coordinates  
         * @parma[in]  dim           Dimensions 
         * @return     idx
         */
        __attribute__((warn_unused_result))
        static inline hb_mc_idx_t hb_mc_coordinate_to_index (hb_mc_coordinate_t coord, hb_mc_dimension_t dim) {
                hb_mc_idx_t idx = hb_mc_coordinate_get_y(coord) * hb_mc_dimension_get_x(dim) + hb_mc_coordinate_get_x(coord); 
                return idx;
        } 

        /**
         * Calculates and returns a 1D length based on 2D dimensions 
         * @parma[in]  dim           Dimensions 
         * @return     1D flat length
         */
        __attribute__((warn_unused_result))
        static inline int  hb_mc_dimension_to_length (hb_mc_dimension_t dim) { 
                return (hb_mc_dimension_get_x(dim) * hb_mc_dimension_get_y(dim)); 
        } 









#ifdef __cplusplus
}
#endif
#endif
