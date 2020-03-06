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

#ifndef BSG_MANYCORE_NPA_H
#define BSG_MANYCORE_NPA_H

#include <bsg_manycore_features.h>
#include <bsg_manycore_coordinate.h>
#include <bsg_manycore_epa.h>

#ifdef __cplusplus
#include <cstdio>
#include <cstdint>
#include <cinttypes>
#else
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

        /**
         * A Network Physical Address.
         * This type uniquely identifies a physical memory address within the entire manycore fabric.
         * Never access the fields of this struct directly. Instead use the accessor functions.
         * The implementation of this type may change -- that's why you should use the accessors.
         */
        typedef struct {
                hb_mc_idx_t x;
                hb_mc_idx_t y;
                hb_mc_epa_t epa;
        } hb_mc_npa_t;

        /**
         * Get the X coordinate from #npa.
         * @param[in] npa   A Network Physical Address. Behavior is undefined if #npa is NULL.
         * @return the X coordinate of #npa.
         */
        static inline hb_mc_idx_t hb_mc_npa_get_x(const hb_mc_npa_t *npa)
        {
                return npa->x;
        }


        /**
         * Get the Y coordinate from #npa.
         * @param[in] npa   A Network Physical Address. Behavior is undefined if #npa is NULL.
         * @return the Y coordinate of #npa.
         */
        static inline hb_mc_idx_t hb_mc_npa_get_y(const hb_mc_npa_t *npa)
        {
                return npa->y;
        }

        /**
         * Get the XY coordinate from #npa.
         * @param[in] npa   A Network Physical Address. Behavior is undefined if #npa is NULL.
         * @return the XY coordinate of #npa.
         */
        static inline hb_mc_coordinate_t hb_mc_npa_get_xy(const hb_mc_npa_t *npa)
        {
                return hb_mc_coordinate(hb_mc_npa_get_x(npa),
                                        hb_mc_npa_get_y(npa));
        }

        /**
         * Get the Endpoint Physical Address from #npa.
         * @param[in] npa   A Network Physical Address. Behavior is undefined if #npa is NULL.
         * @return EPA of the NPA.
         */
        static inline hb_mc_epa_t hb_mc_npa_get_epa(const hb_mc_npa_t *npa)
        {
                return npa->epa;
        }


        /**
         * Set the X coordinate of #npa.
         * @param[in] npa   A Network Physical Address. Behavior is undefined if #npa is NULL.
         * @param[in] X     The new X coordinate.
         * @return A pointer to the modified #npa.
         */
        static inline hb_mc_npa_t* hb_mc_npa_set_x(hb_mc_npa_t *npa, hb_mc_idx_t x)
        {
                npa->x = x;
                return npa;
        }

        /**
         * Set the Y coordinate of #npa.
         * @param[in] npa   A Network Physical Address. Behavior is undefined if #npa is NULL.
         * @param[in] y     The new Y coordinate.
         * @return A pointer to the modified #npa.
         */
        static inline hb_mc_npa_t* hb_mc_npa_set_y(hb_mc_npa_t *npa, hb_mc_idx_t y)
        {
                npa->y = y;
                return npa;
        }


        /**
         * Set the Endpoint Physical Address of #npa.
         * @param[in] npa   A Network Physical Address. Behavior is undefined if #npa is NULL.
         * @param[in] epa   An Endpoint Physical Address.
         * @return A pointer to the modified #npa.
         */
        static inline hb_mc_npa_t* hb_mc_npa_set_epa(hb_mc_npa_t *npa, hb_mc_epa_t epa)
        {
                npa->epa = epa;
                return npa;
        }

        /**
         * Create a Network Physical Address from an X and Y coordinate and an EPA.
         * @param[in] x    X coordinate.
         * @param[in] y    Y coordinate.
         * @param[in] epa  An Endpoint Physical Address.
         * @return A Network Physical Address.
         */
        static inline hb_mc_npa_t hb_mc_npa_from_x_y(hb_mc_idx_t x, hb_mc_idx_t y, hb_mc_epa_t epa)
        {
                hb_mc_npa_t npa;

                hb_mc_npa_set_x(&npa, x);
                hb_mc_npa_set_y(&npa, y);
                hb_mc_npa_set_epa(&npa, epa);
    
                return npa;
        }

        /**
         * Create a Network Physical Address from a coordinate and an EPA.
         * @param[in] c    A coordinate.
         * @param[in] epa  An Endpoint Physical Address.
         * @return A Network Physical Address.
         */
        static inline hb_mc_npa_t hb_mc_npa(hb_mc_coordinate_t c, hb_mc_epa_t epa)
        {
                return hb_mc_npa_from_x_y(hb_mc_coordinate_get_x(c),
                                          hb_mc_coordinate_get_y(c),
                                          epa);    
        }

#define HB_MC_NPA(x_val, y_val, epa_val)                \
        { .x = x_val, .y = y_val, .epa = epa_val }

        /**
         * Create a Network Physical Address from a coordinate and an EPA.
         * @param[in] c    A coordinate.
         * @param[in] epa  An Endpoint Physical Address.
         * @return A Network Physical Address.
         */
#define hb_mc_epa_to_npa(coordinate, epa)       \
        hb_mc_npa(coordinate, epa)

        /**
         * Format an NPA as a human readable string.
         * @param[in] npa An NPA to format into a string.
         * @param[in] buf A string buffer.
         * @param[in] sz  The size of #buf in bytes.
         * @return A formatted string.
         */
        static inline const char *hb_mc_npa_to_string(const hb_mc_npa_t *npa, char *buf, size_t sz)
        {
                snprintf(buf, sz,
                         "npa { "
                         ".x = %" PRId8 ", "
                         ".y = %" PRId8 ", "
                         ".epa = 0x%08" PRIx32 ""
                         " }",
                         hb_mc_npa_get_x(npa),
                         hb_mc_npa_get_y(npa),
                         hb_mc_npa_get_epa(npa));
                return buf;
        }

#ifdef __cplusplus
};
#endif
#endif
