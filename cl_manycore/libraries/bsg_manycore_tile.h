#ifndef BSG_MANYCORE_TILE_H
#define BSG_MANYCORE_TILE_H

#ifndef COSIM
#include <bsg_manycore_features.h>
#include <bsg_manycore_driver.h>
#include <bsg_manycore_epa.h>
#else
#include "bsg_manycore_features.h"
#include "bsg_manycore_driver.h"
#include "bsg_manycore_epa.h"
#endif

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef hb_mc_epa_t epa_t;

typedef enum __hb_mc_csr_freeze_t{
        HB_MC_CSR_FREEZE = 1,
        HB_MC_CSR_UNFREEZE = 0
} hb_mc_csr_freeze_t;

#define hb_mc_tile_epa_get_byte_addr(base, offset) (base + offset)

#define hb_mc_tile_epa_get_word_addr(base, offset) \
	(hb_mc_tile_epa_get_byte_addr(base, offset) >> 2)

int hb_mc_tile_freeze (uint8_t fd, uint8_t x, uint8_t y);
int hb_mc_tile_unfreeze (uint8_t fd, uint8_t x, uint8_t y);
int hb_mc_tile_set_group_origin(uint8_t fd, uint8_t x, uint8_t y, uint8_t x_cord, uint8_t y_cord);

#ifdef __cplusplus
}
#endif

#endif
