#ifndef BSG_MANYCORE_PACKET
#define BSG_MANYCORE_PACKET

#ifndef __USE_BSD 
#define __USE_BSD // the little endian functions will be visible only if this flag is set.
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#include <endian.h>
#include <stdint.h>
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
        uint8_t  op_ex; //!< 4-bit byte mask
        uint8_t  op;    //!< opcode
        uint32_t addr;  //!< address field (EPA)       
        uint8_t  reserved[2];
}  __attribute__((packed)) hb_mc_request_packet_t;

/**
 * Legal opcode values for request packets
 */
#define RQST_OP_REMOTE_LOAD  0
#define RQST_OP_REMOTE_STORE 1

/**
 * Get the X coordinate of the requester from a request packet
 * @param[in] packet a request packet
 * @return the X coordinate of the requester for packet
 */
static inline uint8_t hb_mc_request_packet_get_x_dst(hb_mc_request_packet_t *packet)
{
        return packet->x_dst;
}

/**
 * Get the Y coordinate of the requester from a request packet
 * @param[in] packet a request packet
 * @return the Y coordinate of the requester for packet
 */
static inline uint8_t hb_mc_request_packet_get_y_dst(hb_mc_request_packet_t *packet)
{
        return packet->y_dst;
}

/**
 * Get the X coordinate of the responder from a request packet
 * @param[in] packet a request packet
 * @return the X coordinate of the responder for packet
 */
static inline uint8_t hb_mc_request_packet_get_x_src(hb_mc_request_packet_t *packet)
{
        return packet->x_src;
}

/**
 * Get the X coordinate of the responder from a request packet
 * @param[in] packet a request packet
 * @return the X coordinate of the responder for packet
 */
static inline uint8_t hb_mc_request_packet_get_y_src(hb_mc_request_packet_t *packet)
{
        return packet->y_src;
}

/**
 * Get the extend opcode of a request packet
 * @param[in] packet a request packet
 * @return the extended opcode of packet
 */
static inline uint8_t hb_mc_request_packet_get_op_ex(hb_mc_request_packet_t *packet)
{
        return packet->op_ex;
}

/**
 * Get the opcode of a request packet
 * @param[in] packet a request packet
 * @return the opcode of packet
 */
static inline uint8_t hb_mc_request_packet_get_op(hb_mc_request_packet_t *packet)
{
        return packet->op;
}

/**
 * Get the address field of a request packet
 * @param[in] packet a request packet
 * @return the address field of packet
 */
static inline uint32_t hb_mc_request_packet_get_addr(hb_mc_request_packet_t *packet)
{
        return le32toh(packet->addr);
}

/**
 * Get the data field of a request packet
 * @param[in] packet a request packet
 * @return the data field of packet
 */
static inline uint32_t hb_mc_request_packet_get_data(hb_mc_request_packet_t *packet)
{
        return le32toh(packet->data); // should this do byte masking according to opex?
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
 * Set the extended opcode in a request packet
 * @param[in] packet a request packet
 * @param[in] op_ex an extend opcode
 */
static inline void hb_mc_request_packet_set_op_ex(hb_mc_request_packet_t *packet, uint8_t op_ex)
{
        packet->op_ex = op_ex;
}

/**
 * Set the opcode in a request packet
 * @param[in] packet a request packet
 * @param[in] op an opcode
 */
static inline void hb_mc_request_packet_set_op(hb_mc_request_packet_t *packet, uint8_t op)
{
        packet->op = op;
}

/**
 * Set the opcode in a request packet
 * @param[in] packet a request packet
 * @param[in] addr a valid manycore end point address (EPA)
 */
static inline void hb_mc_request_packet_set_addr(hb_mc_request_packet_t *packet, uint32_t addr)
{
        packet->addr = htole32(addr);
}

/**
 * Set the opcode in a request packet
 * @param[in] packet a request packet
 * @param[in] data packet data
 */
static inline void hb_mc_request_packet_set_data(hb_mc_request_packet_t *packet, uint32_t data)
{
        packet->data = htole32(data); // byte mask?
}

/**
 * The raw response packets that are used to respond to requests.  You should not read from any of the fields directly, instead use the accessor functions.
 */
typedef struct response_packet {
        uint8_t  x_dst; //!< x coordinate of the requester
        uint8_t  y_dst; //!< y coordinate of the requester
        uint32_t  load_id; //!< unused 
        uint32_t data; //!< packet's payload data
        uint8_t  op;    //!< opcode
        uint8_t  reserved[5];
} __attribute__((packed)) hb_mc_response_packet_t;


typedef union packet {
	hb_mc_request_packet_t request; /**/
	hb_mc_response_packet_t response; /* from the Hammerblade Manycore */
	uint32_t words[4];
} hb_mc_packet_t;

/**
 * Get the X coordinate of the requester from a response packet
 * @param[in] packet a response packet
 * @return the X coordinate of the requester for packet
 */
static inline uint8_t response_packet_get_x_dst(response_packet_t *packet)
{
        return packet->x_dst;
}

/**
 * Get the Y coordinate of the requester from a response packet
 * @param[in] packet a response packet
 * @return the Y coordinate of the requester for packet
 */
static inline uint8_t response_packet_get_y_dst(response_packet_t *packet)
{
        return packet->y_dst;
}

/**
 * Get the load ID of the requester from a response packet
 * @param[in] packet a response packet
 * @return the load ID of the packet
 */
static inline uint32_t response_packet_get_load_id(response_packet_t *packet)
{
        return packet->load_id;
}

/**
 * Get the payload data of the requester from a response packet
 * @param[in] packet a response packet
 * @return the packet's payload data
 */
static inline uint32_t response_packet_get_data(response_packet_t *packet)
{
        return packet->data;
}

/**
 * Get the opcode of the requester from a response packet
 * @param[in] packet a response packet
 * @return the packet's opcode
 */
static inline uint8_t response_packet_get_op(response_packet_t *packet)
{
        return packet->op;
}

#endif
