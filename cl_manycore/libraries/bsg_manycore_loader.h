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
	#include <bsg_manycore_tile.h>
	#include <bsg_manycore_errno.h>
	#include <bsg_manycore_mmio.h>
	#include <bsg_manycore_mem.h>
#else
	#include <utils/sh_dpi_tasks.h>
        #include "bsg_manycore_features.h"
	#include "bsg_manycore_driver.h"
	#include "bsg_manycore_tile.h"
	#include "bsg_manycore_errno.h"
	#include "bsg_manycore_mmio.h"
	#include "bsg_manycore_mem.h"
#endif
#include "elf.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

int hb_mc_load_binary (uint8_t fd, char *filename, uint8_t *x, uint8_t *y, uint8_t size);
static uint8_t *hb_mc_get_unfreeze_pkt (uint8_t x, uint8_t y); 
#ifdef __cplusplus
}
#endif

#endif 

