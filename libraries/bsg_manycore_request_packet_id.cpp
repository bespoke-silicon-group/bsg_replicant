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

#include <bsg_manycore_request_packet.h>
#include <bsg_manycore_request_packet_id.h>
#include <bsg_manycore_coordinate.h>

static int x_is_match(const hb_mc_request_packet_t *pkt,
                      const hb_mc_request_packet_id_t *pktid)
{
        uint8_t x = hb_mc_request_packet_get_x_src(pkt);

        return (pktid->id_x_src.x_lo <= x) && (x <= pktid->id_x_src.x_hi);
}

static int y_is_match(const hb_mc_request_packet_t *pkt,
                      const hb_mc_request_packet_id_t *pktid)
{
        uint8_t y = hb_mc_request_packet_get_y_src(pkt);

        return (pktid->id_y_src.y_lo <= y) && (y <= pktid->id_y_src.y_hi);
}

static int addr_is_match(const hb_mc_request_packet_t *pkt,
                         const hb_mc_request_packet_id_t *pktid)
{
        hb_mc_epa_t epa = hb_mc_request_packet_get_epa(pkt);
        /* check for equivalency under mask */
        return (pktid->id_addr.a_mask & epa) == pktid->id_addr.a_value;
}

int hb_mc_request_packet_is_match(const hb_mc_request_packet_t    *pkt,
                                  const hb_mc_request_packet_id_t *pktid)
{
        return x_is_match(pkt, pktid) && y_is_match(pkt, pktid) && addr_is_match(pkt, pktid);
}
