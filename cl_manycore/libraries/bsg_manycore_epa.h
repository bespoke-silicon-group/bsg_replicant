#ifndef __BSG_MANYCORE_EPA
#define __BSG_MANYCORE_EPA

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

#endif
