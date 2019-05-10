#ifndef BSG_MANYCORE_TILE_H
#define BSG_MANYCORE_TILE_H

#ifndef COSIM
	#include <bsg_manycore_features.h>
	#include <bsg_manycore_driver.h>
	#include <bsg_manycore_mem.h>
	#include <bsg_manycore_elf.h>
#else
	#include "bsg_manycore_features.h"
	#include "bsg_manycore_driver.h"
	#include "bsg_manycore_mem.h"
	#incdlue "bsg_manycore_elf.h"
#endif

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t epa_t;

typedef enum __hb_mc_csr_freeze_t{
        HB_MC_CSR_FREEZE = 1,
        HB_MC_CSR_UNFREEZE = 0
} hb_mc_csr_freeze_t;

#define HB_MC_TILE_EPA_DMEM_BASE 0x1000
#define HB_MC_TILE_EPA_ICACHE_BASE 0x1000000
#define HB_MC_TILE_EPA_CSR_BASE 0x20000
#define HB_MC_TILE_EPA_CSR_FREEZE_OFFSET 0x00
#define HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_X_OFFSET 0x04
#define HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_Y_OFFSET 0x08

#define HB_MC_VCACHE_EPA_BASE 0x00000000
#define HB_MC_VCACHE_EPA_DRAM_OFFSET 0x00000000
#define HB_MC_VCACHE_EPA_TAG_OFFSET 0x20000000

#define hb_mc_tile_epa_get_byte_addr(base, offset) (base + offset)

#define hb_mc_tile_epa_get_word_addr(base, offset) \
	(hb_mc_tile_epa_get_byte_addr(base, offset) >> 2)

int hb_mc_tile_freeze (uint8_t fd, uint8_t x, uint8_t y);
int hb_mc_tile_unfreeze (uint8_t fd, uint8_t x, uint8_t y);
int hb_mc_tile_set_origin_registers(uint8_t fd, uint32_t x, uint32_t y, uint32_t origin_x, uint32_t origin_y);
int hb_mc_tile_set_origin_symbols(uint8_t fd, eva_id_t eva_id, char* elf, uint32_t x, uint32_t y, uint32_t origin_x, uint32_t origin_y);
int hb_mc_tile_set_coord_symbols(uint8_t fd, eva_id_t eva_id, char* elf, uint32_t x, uint32_t y, uint32_t coord_x, uint32_t coord_y);
int hb_mc_tile_set_id_symbol(uint8_t fd, eva_id_t eva_id, char* elf, uint32_t x, uint32_t y, uint32_t cord_x, uint32_t cord_y, uint32_t dim_x, uint32_t dim_y);



#ifdef __cplusplus
}
#endif

#endif
