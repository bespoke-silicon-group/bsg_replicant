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

#ifndef BSG_MANYCORE_RESPONSE_PACKET_H
#define BSG_MANYCORE_RESPONSE_PACKET_H

#include <bsg_manycore_features.h>
#include <bsg_manycore_bits.h>
#include <bsg_manycore_errno.h>
#include <endian.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

        /**
         * The raw response packets that are used to respond to requests.
         * You should not read from any of the fields directly, instead use the accessor functions.
         */
        typedef struct response_packet {
                uint8_t   x_dst; //!< x coordinate of the requester
                uint8_t   y_dst; //!< y coordinate of the requester
                uint8_t   load_id; //!< read response id
                uint32_t  data; //!< packet's payload data
                uint8_t   op;    //!< opcode
                uint8_t   reserved[8];
        } __attribute__((packed)) hb_mc_response_packet_t;


        /**
         * Get the X coordinate of the requester from a response packet
         * @param[in] packet a response packet
         * @return the X coordinate of the requester for packet
         */
        static inline uint8_t hb_mc_response_packet_get_x_dst(const hb_mc_response_packet_t *packet)
        {
                return packet->x_dst;
        }

        /**
         * Get the Y coordinate of the requester from a response packet
         * @param[in] packet a response packet
         * @return the Y coordinate of the requester for packet
         */
        static inline uint8_t hb_mc_response_packet_get_y_dst(const hb_mc_response_packet_t *packet)
        {
                return packet->y_dst;
        }

        /**
         * Get the load ID of the requester from a response packet
         * @param[in] packet a response packet
         * @return the load ID of the packet
         */
        static inline uint8_t hb_mc_response_packet_get_load_id(const hb_mc_response_packet_t *packet)
        {
                return packet->load_id;
        }

        /**
         * Get the payload data of the requester from a response packet
         * @param[in] packet a response packet
         * @return the packet's payload data
         */
        static inline uint32_t hb_mc_response_packet_get_data(const hb_mc_response_packet_t *packet)
        {
                return packet->data;
        }

        /**
         * Get the opcode of the requester from a response packet
         * @param[in] packet a response packet
         * @return the packet's opcode
         */
        static inline uint8_t hb_mc_response_packet_get_op(const hb_mc_response_packet_t *packet)
        {
                return packet->op;
        }

        /**
         * Set the x destination of the response packet
         * @param packet a response packet
         * @param x x destination
         */
        static inline void hb_mc_response_packet_set_x_dst(hb_mc_response_packet_t *packet, uint8_t x)
        {
                packet->x_dst = x;
        }

        /**
         * Set the y destination of the response packet
         * @param packet a response packet
         * @param y y destination
         */
        static inline void hb_mc_response_packet_set_y_dst(hb_mc_response_packet_t *packet, uint8_t y)
        {
                packet->y_dst = y;
        }

        /**
         * Set the data of the response packet
         * @param packet a response packet
         * @param data the data to send
         */
        static inline void hb_mc_response_packet_set_data(hb_mc_response_packet_t *packet, uint32_t data)
        {
                packet->data = data;
        }

        /**
         * Set the opcode of the response packet
         * @param packet a response packet
         * @param op the opcode to set
         */
        static inline void hb_mc_response_packet_set_op(hb_mc_response_packet_t *packet, uint8_t op)
        {
                packet->op = op;
        }

        /**
         * Format a response packet as a string.
         * @param[in] packet  A response packet to format as a string.
         * @param[in] buffer  A buffer in which to format the string.
         * @param[in] sz      Size of #buffer.
         * @return A string formatted with human readable packet data.
         */
        static inline const char *hb_mc_response_packet_to_string(const hb_mc_response_packet_t *packet,
                                                                  char *buffer,
                                                                  size_t sz)
        {
                snprintf(buffer, sz,
                         "response_pkt{"
                         "dst=(%" PRIu8 ",%" PRIu8 "), "
                         "data=0x%08" PRIx32 ","
                         "id=0x%08" PRIx32 ","
                         "op=0x%02" PRIx8 ""
                         "}",
                         hb_mc_response_packet_get_x_dst(packet),
                         hb_mc_response_packet_get_y_dst(packet),
                         hb_mc_response_packet_get_data(packet),
                         hb_mc_response_packet_get_load_id(packet),
                         hb_mc_response_packet_get_op(packet));

                return buffer;
        }

#ifdef __cplusplus
}
#endif
#endif
