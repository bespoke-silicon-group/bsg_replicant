#ifndef BSG_MANYCORE_LOADER_DEP_H
#define BSG_MANYCORE_LOADER_DEP_H
#include <bsg_manycore_features.h>
#include <bsg_manycore_driver.h>
#include <bsg_manycore.h>
#include <bsg_manycore_eva.h>
#include <bsg_manycore_coordinate.h>

#ifdef COSIM
	#include <utils/sh_dpi_tasks.h>
#endif

#include "elf.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef __cplusplus
#include <cstdint>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <climits>
#include <cstdbool>
#else
#include <stdint.h>
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

__attribute__((deprecated))
int hb_mc_load_binary (uint8_t fd, char *filename, uint8_t *x, uint8_t *y, uint8_t size);

static uint8_t *hb_mc_get_unfreeze_pkt (uint8_t x, uint8_t y); 

#ifdef __cplusplus
}
#endif

#endif 

