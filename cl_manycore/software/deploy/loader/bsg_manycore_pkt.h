#ifndef BSG_MANYCORE_PKT_H
#define BSG_MANYCORE_PKT_H

/*! \file bsg_manycore_pkt.h
* Constants that describe the Manycore's architecture.
*/

#include <stdint.h>
      
/*
 * packet format: {addr, op, op_ex, data, src_y_cord, src_x_cord, y_cord, x_cord)
 * */

uint32_t AXI_STREAM_BIT = 128; /*! The number of bits that the AXI shell considers as one Manycore packet.*/
uint32_t MANYCORE_BIT = 80; /*! The number of bits in a Manycore packet. */
uint32_t ADDR_BIT = 26; /*! Destination address of packet. */
uint32_t DATA_BIT = 32; /*! Data of packet. */
/* -------------------------- */
// depend on the Manycore dimensions
uint8_t NUM_X = 4; /*! Number of columns of tiles. */
uint8_t NUM_Y = 4; /*! Number of rows of tiles */
uint8_t MY_X = 3; /*! X coordinate of the host */
uint8_t MY_Y = 4; /*! Y coordinate of the host */
uint32_t X_BIT = 2; /*! Number of bits for encoding x coordinates. */
uint32_t Y_BIT = 3; /* Number of bits for encoding y coordinates. */
/* -------------------------- */
uint32_t OP_REMOTE_STORE = 1; 
uint8_t OP_EX = 0xF; 
uint8_t OP_EX_BIT = 4; /*! Number of bits for the OP EX field. */
uint8_t OP_BIT = 2; /*! Number of bits for the OP field. */

uint8_t TEXT = 1; 
uint8_t DATA = 0;

uint32_t NUM_ICACHE = 1024; /*! Number of icache entries. */

uint8_t NUM_VCACHE = 2; /*! Number of victim caches. */
uint8_t NUM_VCACHE_ENTRY = 32; /*! Number of victim cache entries per cache. */
uint8_t VCACHE_WAYS = 2; /* The set-associativity of the victim caches. */
#endif
