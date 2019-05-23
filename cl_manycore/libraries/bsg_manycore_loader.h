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
#include <bsg_manycore_errno.h>
#include <bsg_manycore_mmio.h>
#include <bsg_manycore_eva.h>
#else
#include <utils/sh_dpi_tasks.h>
#include "bsg_manycore_features.h"
#include "bsg_manycore_driver.h"
#include "bsg_manycore.h"
#include "bsg_manycore_errno.h"
#include "bsg_manycore_mmio.h"
#include "bsg_manycore_eva.h"
#endif

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

/**
 * Loads a binary object into a list of tiles and DRAM
 * @param[in]  bin    A memory buffer containing a valid manycore binary
 * @param[in]  sz     Size of #bin in bytes
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  id     An eva ID for computing the eva to npa map
 * @param[in]  tiles  A list of manycore to load with #bin, with the origin at 0
 * @param[in]  len    The number of tiles in #tiles
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_loader_load(const void *bin, size_t sz, hb_mc_manycore_t *mc,
		const hb_mc_eva_id_t *id, const hb_mc_coordinate_t *tiles, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif 

