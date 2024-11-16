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

#ifndef BSG_MANYCORE_REQUEST_PACKET_ID_H
#define BSG_MANYCORE_REQUEST_PACKET_ID_H

#include <bsg_manycore_features.h>
#include <bsg_manycore_request_packet.h>
#include <bsg_manycore_epa.h>
#include <bsg_manycore_coordinate.h>
#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

        typedef struct request_packet_id {
                int init;
                /* matching with addresses */
                struct address_value_mask_pair
                {
                        hb_mc_epa_t a_value;
                        hb_mc_epa_t a_mask;
#ifdef __cplusplus
                        /* Needed to support initializers in C++ sources */
                address_value_mask_pair(): a_value(0), a_mask(0){}
                address_value_mask_pair(hb_mc_epa_t v, hb_mc_epa_t m):
                        a_value(v), a_mask(m) {}
#endif
                } id_addr; //!< matches addresses

                /* matching with X coordinates */
                struct coordinate_x_lo_hi_pair
                {
                        uint32_t x_lo;
                        uint32_t x_hi;
#ifdef __cplusplus
                        /* Needed to support initializers in C++ sources */
                coordinate_x_lo_hi_pair(): x_lo(0), x_hi(0) {}
                coordinate_x_lo_hi_pair(uint32_t lo, uint32_t hi):
                        x_lo(lo), x_hi(hi) {}
#endif
                } id_x_src; //!< matches source X coordinates

                /* matching with Y coordinate */
                struct coordinate_y_lo_hi_pair
                {
                        uint32_t y_lo;
                        uint32_t y_hi;
#ifdef __cplusplus
                        /* Needed to support initializers in C++ sources */
                coordinate_y_lo_hi_pair(): y_lo(0), y_hi(0) {}
                coordinate_y_lo_hi_pair(uint32_t lo, uint32_t hi):
                        y_lo(lo), y_hi(hi) {}
#endif
                } id_y_src; //!< matches source Y coordinates

#ifdef __cplusplus
                /* Needed to support initializers in C++ sources */
        request_packet_id() : id_addr(), id_x_src(), id_y_src() {}
        request_packet_id(address_value_mask_pair addr,
                          coordinate_x_lo_hi_pair x,
                          coordinate_y_lo_hi_pair y,
                          int init = 0):
                init(init), id_addr(addr), id_x_src(x), id_y_src(y) {}
#endif
        } hb_mc_request_packet_id_t;

        /* this is used to match an address under a mask */
#ifdef __cplusplus
#define RQST_ID_ADDR_UNDER_MASK(addr, mask)     \
        {addr, mask}

#define RQST_ID_ADDR(addr)                      \
        {addr, UINT32_MAX}
#else
#define RQST_ID_ADDR_UNDER_MASK(addr, mask)     \
        {.a_value = addr, .a_mask = mask}

#define RQST_ID_ADDR(addr)                      \
        {.a_value = addr, .a_mask = UINT32_MAX}
#endif


        /* these are used to match INCLUSIVE ranges */
#ifdef __cplusplus
#define RQST_ID_RANGE_X(lo, hi)                 \
        {lo, hi}
#define RQST_ID_RANGE_Y(lo, hi)                 \
        {lo, hi}
#else
#define RQST_ID_RANGE_X(lo, hi)                 \
        {.x_lo = lo, .x_hi = hi}
#define RQST_ID_RANGE_Y(lo, hi)                 \
        {.y_lo = lo, .y_hi = hi}
#endif

        /* these are used to match any coordinate */
#define RQST_ID_ANY_X                           \
        RQST_ID_RANGE_X(0, UINT32_MAX)
#define RQST_ID_ANY_Y                           \
        RQST_ID_RANGE_Y(0, UINT32_MAX)

        /* these are used to match a single coordinate */
#define RQST_ID_X(xval)                         \
        RQST_ID_RANGE_X(xval, xval)
#define RQST_ID_Y(yval)                         \
        RQST_ID_RANGE_Y(yval, yval)

#ifdef __cplusplus
#define RQST_ID(x, y, addr)                     \
        request_packet_id({addr, x, y, 1})
#else
#define RQST_ID(x, y, addr)                                             \
        { .init = 1, .id_x_src = x, .id_y_src = y, .id_addr = addr }
#endif

        /**
         * Query if a response packet matches an ID.
         * @param[in] pkt   A response packet. Behavior is undefined if this is not a valid pktid.
         * @param[in] pktid A response packet ID. Behavior is undefined if this is not a valid pktid.
         * @return 0 if there's no match, 1 if there is a match.
         */
        int hb_mc_request_packet_is_match(const hb_mc_request_packet_t    *pkt,
                                          const hb_mc_request_packet_id_t *pktid);


#ifdef __cplusplus
}
#endif

#endif
