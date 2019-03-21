#ifndef BSG_MANYCORE_PACKET
#define BSG_MANYCORE_PACKET
#include <stdint.h>
#include <endian.h>

/**
 * The raw request packets that are used to initiate communications between the manycore and the host.
 * You should not read from any of the fields directly, instead use the accessor functions.
 */
typedef struct request_packet {
        uint8_t  x_dst; //!< x coordinate of the requester
        uint8_t  y_dst; //!< y coordinate of the requester
        uint8_t  x_src; //!< x coordinate of the responder
        uint8_t  y_src; //!< y coordinate of the responder
        uint32_t data;  //!< packet's payload data
        uint8_t  op_ex; //!< 4-bit byte mask
        uint8_t  op;    //!< opcode
        uint32_t addr;  //!< address field (EPA)       
        uint8_t  reserved[2];
}  __attribute__((packed)) request_packet_t;

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
static inline uint8_t request_packet_get_x_dst(request_packet_t *packet)
{
        return packet->x_dst;
}

/**
 * Get the Y coordinate of the requester from a request packet
 * @param[in] packet a request packet
 * @return the Y coordinate of the requester for packet
 */
static inline uint8_t request_packet_get_y_dst(request_packet_t *packet)
{
        return packet->y_dst;
}

/**
 * Get the X coordinate of the responder from a request packet
 * @param[in] packet a request packet
 * @return the X coordinate of the responder for packet
 */
static inline uint8_t request_packet_get_x_src(request_packet_t *packet)
{
        return packet->x_src;
}

/**
 * Get the X coordinate of the responder from a request packet
 * @param[in] packet a request packet
 * @return the X coordinate of the responder for packet
 */
static inline uint8_t request_packet_get_y_src(request_packet_t *packet)
{
        return packet->y_src;
}

/**
 * Get the extend opcode of a request packet
 * @param[in] packet a request packet
 * @return the extended opcode of packet
 */
static inline uint8_t request_packet_get_op_ex(request_packet_t *packet)
{
        return packet->op_ex;
}

/**
 * Get the opcode of a request packet
 * @param[in] packet a request packet
 * @return the opcode of packet
 */
static inline uint8_t request_packet_get_op(request_packet_t *packet)
{
        return packet->op;
}

/**
 * Get the address field of a request packet
 * @param[in] packet a request packet
 * @return the address field of packet
 */
static inline uint32_t request_packet_get_addr(request_packet_t *packet)
{
        return le32toh(packet->addr);
}

/**
 * Get the data field of a request packet
 * @param[in] packet a request packet
 * @return the data field of packet
 */
static inline uint32_t request_packet_get_data(request_packet_t *packet)
{
        return le32toh(packet->data); // should this do byte masking according to opex?
}

/**
 * Set the X coordinate of the requester in a request packet
 * @param[in] packet a request packet
 * @param[in] x an X coordinate
 */
static inline void request_packet_set_x_dst(request_packet_t *packet, uint8_t x)
{
        packet->x_dst = x;
}

/**
 * Set the Y coordinate of the requester in a request packet
 * @param[in] packet a request packet
 * @param[in] y an Y coordinate
 */
static inline void request_packet_set_y_dst(request_packet_t *packet, uint8_t y)
{
        packet->y_dst = y;
}

/**
 * Set the X coordinate of the responder in a request packet
 * @param[in] packet a request packet
 * @param[in] x an X coordinate
 */
static inline void request_packet_set_x_src(request_packet_t *packet, uint8_t x)
{
        packet->x_src = x;
}

/**
 * Set the Y coordinate of the responder in a request packet
 * @param[in] packet a request packet
 * @param[in] y a Y coordinate
 */
static inline void request_packet_set_y_src(request_packet_t *packet, uint8_t y)
{
        packet->y_src = y;
}

/**
 * Set the extended opcode in a request packet
 * @param[in] packet a request packet
 * @param[in] op_ex an extend opcode
 */
static inline void request_packet_set_op_ex(request_packet_t *packet, uint8_t op_ex)
{
        packet->op_ex = op_ex;
}

/**
 * Set the opcode in a request packet
 * @param[in] packet a request packet
 * @param[in] op an opcode
 */
static inline void request_packet_set_op(request_packet_t *packet, uint8_t op)
{
        packet->op = op;
}

/**
 * Set the opcode in a request packet
 * @param[in] packet a request packet
 * @param[in] addr a valid manycore end point address (EPA)
 */
static inline void request_packet_set_addr(request_packet_t *packet, uint32_t addr)
{
        packet->addr = htole32(addr);
}

/**
 * Set the opcode in a request packet
 * @param[in] packet a request packet
 * @param[in] data packet data
 */
static inline void request_packet_set_data(request_packet_t *packet, uint32_t data)
{
        packet->data = htole32(data); // byte mask?
}

typedef struct response_packet {
        // fill this in when I know it
} __attribute__((packed)) response_packet_t;

#endif
