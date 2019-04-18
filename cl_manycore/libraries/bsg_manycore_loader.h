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

#ifdef __cplusplus
extern "C" {
#endif

static uint32_t OP_REMOTE_LOAD = 0;
static uint32_t OP_REMOTE_STORE = 1; 
static uint8_t OP_EX = 0xF; 

static uint8_t TEXT = 1; 
static uint8_t DATA = 0;

static uint32_t EPA_BYTE_ADDR_WIDTH = 18;
static uint32_t SRC_FREEZE = 0;
static uint32_t SRC_UNFREEZE = 1;
static uint32_t CSR_TGO_X = 1;
static uint32_t CSR_TGO_Y = 2;

static uint32_t EPA_TAG_ADDR_WIDTH = 30;

#define  DMEM_BASE 0x1000

int hb_mc_load_binary (uint8_t fd, char *filename, uint8_t *x, uint8_t *y, uint8_t size);
static uint8_t *hb_mc_get_unfreeze_pkt (uint8_t x, uint8_t y); 
int hb_mc_freeze (uint8_t fd, uint8_t x, uint8_t y);
int hb_mc_unfreeze (uint8_t fd, uint8_t x, uint8_t y);
int hb_mc_set_tile_group_origin(uint8_t fd, uint8_t x, uint8_t y, uint8_t x_cord, uint8_t y_cord);
int hb_mc_init_cache_tag(uint8_t fd, uint8_t x, uint8_t y);
#ifdef __cplusplus
}
#endif

#endif 

