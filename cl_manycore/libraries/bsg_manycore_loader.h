#ifndef BSG_MANYCORE_LOADER_H
#define BSG_MANYCORE_LOADER_H

#ifndef _BSD_SOURCE
	#define _BSD_SOURCE
#endif
#ifndef _XOPEN_SOURCE
	#define _XOPEN_SOURCE 500
#endif

#ifndef COSIM
        #include <bsg_manycore_features.h>
	#include <bsg_manycore_driver.h>
	#include <bsg_manycore.h>
	#include <bsg_manycore_tile.h>
	#include <bsg_manycore_errno.h>
	#include <bsg_manycore_mmio.h>
	#include <bsg_manycore_mem.h>
#else
	#include <utils/sh_dpi_tasks.h>
        #include "bsg_manycore_features.h"
	#include "bsg_manycore_driver.h"
	#include "bsg_manycore.h"
	#include "bsg_manycore_tile.h"
	#include "bsg_manycore_errno.h"
	#include "bsg_manycore_mmio.h"
	#include "bsg_manycore_mem.h"
#endif
#include "elf.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef __cplusplus
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <climits>
#include <cstdbool>
#else
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

int hb_mc_load_binary (uint8_t fd, char *filename, uint8_t *x, uint8_t *y, uint8_t size);
static uint8_t *hb_mc_get_unfreeze_pkt (uint8_t x, uint8_t y); 

// Loads an ELF into each tile in tiles, and all DRAMs
int hb_mc_loader_load(const void *bin, size_t sz, const hb_mc_manycore_t *mc, eva_id_t id, const hb_mc_coordinate_t *tiles, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif 

