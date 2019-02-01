#ifndef BSG_MANYCORE_PKT_H
#define BSG_MANYCORE_PKT_H

#include <stdint.h>
      
/*
 * packet format: {addr, op, op_ex, data, src_y_cord, src_x_cord, y_cord, x_cord)
 * */

uint32_t AXI_STREAM_BIT = 128;
uint32_t MANYCORE_BIT = 80;
uint32_t ADDR_BIT = 26;
uint32_t DATA_BIT = 32;
uint8_t NUM_X = 4; 
uint8_t NUM_Y = 4;  
uint8_t MY_X = 3;
uint8_t MY_Y = 4;
uint32_t X_BIT = 2;
uint32_t Y_BIT = 3;
uint32_t OP_REMOTE_STORE = 1; 
uint8_t OP_EX = 0xF;
uint8_t OP_EX_BIT = 4;
uint8_t OP_BIT = 2;

uint8_t TEXT = 1;
uint8_t DATA = 0;

uint32_t NUM_ICACHE = 1024;

uint8_t NUM_VCACHE = 2;
uint8_t NUM_VCACHE_ENTRY = 32;
uint8_t VCACHE_WAYS = 2;
#endif
