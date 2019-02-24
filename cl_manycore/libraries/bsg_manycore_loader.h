#ifndef BSG_MANYCORE_LOADER_H
#define BSG_MANYCORE_LOADER_H

#ifndef _BSD_SOURCE
	#define _BSD_SOURCE
#endif
#ifndef _XOPEN_SOURCE
	#define _XOPEN_SOURCE 500
#endif

#include "elf.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>

static uint32_t ADDR_BIT = 27; /*! Destination address of packet. */
static uint32_t DATA_BIT = 32; /*! Data of packet. */
/* -------------------------- */
// depend on the Manycore dimensions
static uint8_t MY_X = 3; /*! X coordinate of the host */
static uint8_t MY_Y = 4; /*! Y coordinate of the host */
static uint32_t X_BIT = 2; /*! Number of bits for encoding x coordinates. */
static uint32_t Y_BIT = 3; /* Number of bits for encoding y coordinates. */
/* -------------------------- */
static uint32_t OP_REMOTE_LOAD = 0;
static uint32_t OP_REMOTE_STORE = 1; 
static uint8_t OP_EX = 0xF; 
static uint8_t OP_EX_BIT = 4; /*! Number of bits for the OP EX field. */
static uint8_t OP_BIT = 2; /*! Number of bits for the OP field. */

static uint8_t TEXT = 1; 
static uint8_t DATA = 0;

static uint32_t NUM_ICACHE = 1024; /*! Number of icache entries. */

static uint8_t NUM_VCACHE = 2; /*! Number of victim caches. */
static uint8_t NUM_VCACHE_ENTRY = 32; /*! Number of victim cache entries per cache. */
static uint8_t VCACHE_WAYS = 2; /* The set-associativity of the victim caches. */

extern uint32_t DMEM_BASE;


static bool hb_mc_load_packets(uint8_t fd, uint8_t **pkts, uint32_t num_pkts);
static void hb_mc_set_bits (uint8_t *data, uint8_t start, uint8_t val);
static void hb_mc_set_field (uint8_t *packet, uint8_t bit_start, uint8_t bit_end, uint32_t val);
static void hb_mc_parse_elf (char *filename, uint8_t x, uint8_t y, uint32_t *num_instr, uint32_t *data_size, uint8_t ***icache_pkts, uint8_t ***dram_pkts, uint8_t ***dmem_pkts, bool init_dram);	
uint8_t *hb_mc_get_pkt(uint32_t addr, uint32_t data, uint8_t x, uint8_t y, uint8_t opcode);
void hb_mc_load_binary (uint8_t fd, char *filename, uint8_t *x, uint8_t *y, uint8_t size);
static uint8_t *hb_mc_get_unfreeze_pkt (uint8_t x, uint8_t y); 
void hb_mc_unfreeze (uint8_t fd, uint8_t x, uint8_t y);
static uint32_t *hb_mc_get_byte (uint32_t *packet, uint8_t ofs);
static uint32_t hb_mc_get_bits(uint32_t data, uint8_t start,  uint8_t size);
 #endif 
