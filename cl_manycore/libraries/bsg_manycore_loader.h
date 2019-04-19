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

static uint8_t WORD = 4;
static uint8_t SHORT = 2;

static uint32_t ADDR_BYTE = 4; /*! Number of bytes for encoding the address. */
static uint32_t DATA_BYTE = 4; /*! Number of bytes for encoding the data. */
/* -------------------------- */
// depend on the Manycore dimensions
extern uint8_t MY_X; /*! X coordinate of the host */
extern uint8_t MY_Y; /*! Y coordinate of the host */
static uint32_t X_BYTE = 1; /*! Number of bytes for encoding x coordinates. */
static uint32_t Y_BYTE = 1; /* Number of bytes for encoding y coordinates. */
/* -------------------------- */
static uint32_t OP_REMOTE_LOAD = 0;
static uint32_t OP_REMOTE_STORE = 1; 
static uint8_t OP_EX = 0xF; 
static uint8_t OP_EX_BYTE = 1; /*! Number of bytes for the OP EX field. */
static uint8_t OP_BYTE = 1; /*! Number of bytes for the OP field. */

static uint8_t TEXT = 1; 
static uint8_t DATA = 0;

static uint32_t EPA_BYTE_ADDR_WIDTH = 18;
static uint32_t SRC_FREEZE = 0;
static uint32_t SRC_UNFREEZE = 1;
static uint32_t CSR_TGO_X = 1;
static uint32_t CSR_TGO_Y = 2;

static uint32_t EPA_TAG_ADDR_WIDTH = 30;
static uint32_t NUM_CACHE = 4;

static uint32_t NUM_ICACHE = 1024; /*! Number of icache entries. */

static uint8_t NUM_VCACHE = 2; /*! Number of victim caches. */
static uint8_t NUM_VCACHE_ENTRY = 32; /*! Number of victim cache entries per cache. */
static uint8_t VCACHE_WAYS = 2; /* The set-associativity of the victim caches. */

#define  DMEM_BASE 0x1000

int hb_mc_load_binary (uint8_t fd, char *filename, uint8_t *x, uint8_t *y, uint8_t size);
static uint8_t *hb_mc_get_unfreeze_pkt (uint8_t x, uint8_t y); 
int hb_mc_freeze (uint8_t fd, uint8_t x, uint8_t y);
int hb_mc_unfreeze (uint8_t fd, uint8_t x, uint8_t y);
int hb_mc_set_tile_group_origin(uint8_t fd, uint8_t x, uint8_t y, uint8_t x_cord, uint8_t y_cord);
int hb_mc_init_cache_tag(uint8_t fd, uint8_t x, uint8_t y);
#endif 

