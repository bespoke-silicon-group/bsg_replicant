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

#ifndef BSG_MANYCORE_REQUEST_PACKET_H
#define BSG_MANYCORE_REQUEST_PACKET_H

#include <bsg_manycore_features.h>
#include <bsg_manycore_bits.h>
#include <bsg_manycore_errno.h>
#include <bsg_manycore_epa.h>
#include <endian.h>
#ifdef __cplusplus
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#else
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#endif

#define HB_MC_PACKET_PAYLOAD_REMOTE_LOAD 0

#ifdef __cplusplus
extern "C" {
#endif

        /**
         * The raw request packets that are used to initiate communications between the manycore and the host.
         * You should not read from any of the fields directly, instead use the accessor functions.
         */
        typedef struct request_packet {
                uint8_t  x_dst; //!< x coordinate of the responder
                uint8_t  y_dst; //!< y coordinate of the responder
                uint8_t  x_src; //!< x coordinate of the requester
                uint8_t  y_src; //!< y coordinate of the requester
                uint32_t data;  //!< packet's payload data
                uint8_t  reg_id; //!< 5-bit id for load or amo
                uint8_t  op_ex;  //!< 4-bit byte mask
                uint8_t  op;    //!< opcode
                uint32_t addr;  //!< address field (EPA)
                uint8_t  reserved[1];
        }  __attribute__((packed)) hb_mc_request_packet_t;


        typedef enum __hb_mc_packet_op_t {
                HB_MC_PACKET_OP_REMOTE_LOAD  = 0,
                HB_MC_PACKET_OP_REMOTE_STORE = 1,
                HB_MC_PACKET_OP_CACHE_OP     = 3, // AFL, AINV, AFLINV
        } hb_mc_packet_op_t;

        typedef enum __hb_mc_packet_cache_op {
                HB_MC_PACKET_CACHE_OP_AFL    = 0, //!< flush address
                HB_MC_PACKET_CACHE_OP_AINV   = 1, //!< invalidate address
                HB_MC_PACKET_CACHE_OP_AFLINV = 2, //!< flush address and invalidate
		HB_MC_PACKET_CACHE_OP_TAGFL  = 3, //!< flush tag
        } hb_mc_packet_cache_op_t;

        typedef enum __hb_mc_packet_mask_t {
                HB_MC_PACKET_REQUEST_MASK_BYTE  = 0x1,
                HB_MC_PACKET_REQUEST_MASK_SHORT = 0x3,
                HB_MC_PACKET_REQUEST_MASK_WORD  = 0xF,

                HB_MC_PACKET_REQUEST_MASK_BYTE_0 = HB_MC_PACKET_REQUEST_MASK_BYTE << 0,
                HB_MC_PACKET_REQUEST_MASK_BYTE_1 = HB_MC_PACKET_REQUEST_MASK_BYTE << 1,
                HB_MC_PACKET_REQUEST_MASK_BYTE_2 = HB_MC_PACKET_REQUEST_MASK_BYTE << 2,
                HB_MC_PACKET_REQUEST_MASK_BYTE_3 = HB_MC_PACKET_REQUEST_MASK_BYTE << 3,

                HB_MC_PACKET_REQUEST_MASK_SHORT_0 = HB_MC_PACKET_REQUEST_MASK_SHORT << 0,
                HB_MC_PACKET_REQUEST_MASK_SHORT_1 = HB_MC_PACKET_REQUEST_MASK_SHORT << 2,
        } hb_mc_packet_mask_t;

        /**
         * Get the X coordinate of the requester from a request packet
         * @param[in] packet a request packet
         * @return the X coordinate of the requester for packet
         */
        static inline uint8_t hb_mc_request_packet_get_x_dst(const hb_mc_request_packet_t *packet)
        {
                return packet->x_dst;
        }

        /**
         * Get the Y coordinate of the requester from a request packet
         * @param[in] packet a request packet
         * @return the Y coordinate of the requester for packet
         */
        static inline uint8_t hb_mc_request_packet_get_y_dst(const hb_mc_request_packet_t *packet)
        {
                return packet->y_dst;
        }

        /**
         * Get the X coordinate of the responder from a request packet
         * @param[in] packet a request packet
         * @return the X coordinate of the responder for packet
         */
        static inline uint8_t hb_mc_request_packet_get_x_src(const hb_mc_request_packet_t *packet)
        {
                return packet->x_src;
        }

        /**
         * Get the X coordinate of the responder from a request packet
         * @param[in] packet a request packet
         * @return the X coordinate of the responder for packet
         */
        static inline uint8_t hb_mc_request_packet_get_y_src(const hb_mc_request_packet_t *packet)
        {
                return packet->y_src;
        }

        /**
         * Get the extend opcode of a request packet
         * @param[in] packet a request packet
         * @return the extended opcode of packet
         */
        static inline uint8_t hb_mc_request_packet_get_mask(const hb_mc_request_packet_t *packet)
        {
                return packet->op_ex;
        }

        /**
         * Get the extended opcode of a request packet (a cache opcode if this is a cache op)
         * @param[in] packet a request packet
         * @return the extended cache opcode
         */
        static inline uint8_t hb_mc_request_packet_get_cache_op(const hb_mc_request_packet_t *packet)
        {
	    return packet->op_ex;
        }

        /**
         * Get the opcode of a request packet
         * @param[in] packet a request packet
         * @return the opcode of packet
         */
        static inline uint8_t hb_mc_request_packet_get_op(const hb_mc_request_packet_t *packet)
        {
                return packet->op;
        }

        /**
         * Get the address field of a request packet
         * @param[in] packet a request packet
         * @return the address field of packet
         */
        static inline uint32_t hb_mc_request_packet_get_addr(const hb_mc_request_packet_t *packet)
        {
                return le32toh(packet->addr);
        }

        /**
         * Get the data field of a request packet
         * @param[in] packet a request packet
         * @return the data field of packet
         */
        static inline uint32_t hb_mc_request_packet_get_data(const hb_mc_request_packet_t *packet)
        {
                return le32toh(packet->data);
        }

        /**
         * Get the valid data of a request packet
         * @param[in] packet a request packet
         * @return the valid data field of packet
         */
        static inline uint32_t hb_mc_request_packet_get_data_valid(const hb_mc_request_packet_t *packet)
        {
                uint32_t valid = 0;
                for (int i = 0; i < 4; i++) { /* TODO: hardcoded */
                        if (hb_mc_get_bits(packet->op_ex, i, 1) == 1)
                                valid |=  hb_mc_get_bits(packet->data, i*8, 8);
                }
                return le32toh(valid);

        }
        /**
         * Set the X coordinate of the requester in a request packet
         * @param[in] packet a request packet
         * @param[in] x an X coordinate
         */
        static inline void hb_mc_request_packet_set_x_dst(hb_mc_request_packet_t *packet, uint8_t x)
        {
                packet->x_dst = x;
        }

        /**
         * Set the Y coordinate of the requester in a request packet
         * @param[in] packet a request packet
         * @param[in] y an Y coordinate
         */
        static inline void hb_mc_request_packet_set_y_dst(hb_mc_request_packet_t *packet, uint8_t y)
        {
                packet->y_dst = y;
        }

        /**
         * Set the X coordinate of the responder in a request packet
         * @param[in] packet a request packet
         * @param[in] x an X coordinate
         */
        static inline void hb_mc_request_packet_set_x_src(hb_mc_request_packet_t *packet, uint8_t x)
        {
                packet->x_src = x;
        }

        /**
         * Set the Y coordinate of the responder in a request packet
         * @param[in] packet a request packet
         * @param[in] y a Y coordinate
         */
        static inline void hb_mc_request_packet_set_y_src(hb_mc_request_packet_t *packet, uint8_t y)
        {
                packet->y_src = y;
        }

        /**
         * Set the id in a load request packet
         * @param[in] packet a request packet
         * @param[in] id for int/float load
         */
        static inline void hb_mc_request_packet_set_load_id(hb_mc_request_packet_t *packet, uint8_t load_id)
        {
                packet->reg_id = load_id;
        }

        /**
         * Set the data mask in a request packet
         * @param[in] packet a request packet
         * @param[in] mask a byte-mask value for remote store
         */
        static inline void hb_mc_request_packet_set_mask(hb_mc_request_packet_t *packet, hb_mc_packet_mask_t mask)
        {
                packet->op_ex = mask;
        }

        /**
         * Set the cache opcode in a request packet
         * @param[in] packet a request packet
         * @param[in] opcode a cache opcode
         */
        static inline void hb_mc_request_packet_set_cache_op(hb_mc_request_packet_t *packet,
                                                             hb_mc_packet_cache_op_t op)
        {
                packet->op_ex = op;
        }

        /**
         * Set the opcode in a request packet
         * @param[in] packet a request packet
         * @param[in] op an opcode
         */
        static inline void hb_mc_request_packet_set_op(hb_mc_request_packet_t *packet, hb_mc_packet_op_t op)
        {
                packet->op = op;
        }

        /**
         * Set the addess in a request packet
         * @param[in] packet a request packet
         * @param[in] addr a valid manycore end point word address
         */
        static inline void hb_mc_request_packet_set_addr(hb_mc_request_packet_t *packet, uint32_t addr)
        {
                packet->addr = htole32(addr);
        }

        /**
         * Set the data in a request packet
         * @param[in] packet a request packet
         * @param[in] data packet data
         */
        static inline void hb_mc_request_packet_set_data(hb_mc_request_packet_t *packet, uint32_t data)
        {
                packet->data = htole32(data); // TODO: byte mask?
        }


        /**
         * Get the EPA of a request packet.
         * This function differs from hb_mc_request_packet_get_addr() in that it performs translation
         * of the address field.
         * @param[in] packet a request packet
         * @return the EPA of the packet
         */
        static inline hb_mc_epa_t hb_mc_request_packet_get_epa(const hb_mc_request_packet_t *packet)
        {
                return hb_mc_request_packet_get_addr(packet) << 2;
        }

        /**
         * Set the EPA of a request packet.
         * @param[in] packet a request packet
         * @param[in] addr a valid manycore end point address (EPA)
         */
        static inline void hb_mc_request_packet_set_epa(hb_mc_request_packet_t *packet,
                                                        hb_mc_epa_t epa)
        {
                hb_mc_request_packet_set_addr(packet, epa >> 2);
        }

        /**
         * Checks if 2 request packets are the same.
         * @param[in] a a request packet
         * @param[in] b a request packet
         * @return HB_MC_SUCCESS if packets match and HB_MC_FAIL if packets do not match. In order to match,
         * all of the non-data fields of a an b must be the same and the valid data must be the same.
         */
        static int hb_mc_request_packet_equals(const hb_mc_request_packet_t *a, const hb_mc_request_packet_t *b) {
                if (!a || !b) {
                        return HB_MC_FAIL;
                }
                else if (hb_mc_request_packet_get_x_dst(a) != hb_mc_request_packet_get_x_dst(b)) {
                        return HB_MC_FAIL;
                }
                else if (hb_mc_request_packet_get_y_dst(a) != hb_mc_request_packet_get_y_dst(b)) {
                        return HB_MC_FAIL;
                }
                else if (hb_mc_request_packet_get_x_src(a) != hb_mc_request_packet_get_x_src(b)) {
                        return HB_MC_FAIL;
                }
                else if (hb_mc_request_packet_get_y_src(a) != hb_mc_request_packet_get_y_src(b)) {
                        return HB_MC_FAIL;
                }
                else if (hb_mc_request_packet_get_data_valid(a) != hb_mc_request_packet_get_data_valid(b)) {
                        return HB_MC_FAIL;
                }
                else if (hb_mc_request_packet_get_addr(a) != hb_mc_request_packet_get_addr(b)) {
                        return HB_MC_FAIL;
                }
                return HB_MC_SUCCESS;
        }


        /**
         * Format a request packet as a string.
         * @param[in] packet  A request packet to format as a string.
         * @param[in] buffer  A buffer in which to format the string.
         * @param[in] sz      Size of #buffer.
         * @return A string formatted with human readable packet data.
         */
        static inline const char *hb_mc_request_packet_to_string(const hb_mc_request_packet_t *packet,
                                                                 char *buffer,
                                                                 size_t sz)
        {
                snprintf(buffer, sz,
                         "request_pkt{"
                         "dst=(%" PRIu8 ",%" PRIu8 "), "
                         "addr=0x%08" PRIx32 ", "
                         "data=0x%08" PRIx32 ", "
                         "op=0x%02" PRIx8 ""
                         "}",
                         hb_mc_request_packet_get_x_dst(packet),
                         hb_mc_request_packet_get_y_dst(packet),
                         hb_mc_request_packet_get_addr(packet),
                         hb_mc_request_packet_get_data(packet),
                         hb_mc_request_packet_get_op(packet)
                         );

                return buffer;
        }
#ifdef __cplusplus
}
#endif
#endif
