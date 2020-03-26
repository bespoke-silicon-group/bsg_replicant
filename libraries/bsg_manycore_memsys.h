#pragma once
#include <bsg_manycore_features.h>
#include <bsg_manycore_rom.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Memory system IDs as they appear in the ROM */
#define HB_MC_ROM_MEMSYS_ID_NONE     "NONE"
#define HB_MC_ROM_MEMSYS_ID_AXI4     "AXI4"
#define HB_MC_ROM_MEMSYS_ID_INFMEM   "INFM"
#define HB_MC_ROM_MEMSYS_ID_DRAMSIM3 "DRS3"

/* Memory system IDs */
typedef enum __hb_mc_memsys_id_t {
        HB_MC_MEMSYS_ID_NONE,
        HB_MC_MEMSYS_ID_AXI4,
        HB_MC_MEMSYS_ID_INFMEM,
        HB_MC_MEMSYS_ID_DRAMSIM3,
} hb_mc_memsys_id_t;

const char *hb_mc_memsys_id_to_string(
        hb_mc_memsys_id_t id);

typedef struct __hb_mc_dram_pa_bitfield {
        uint32_t bits;
        uint32_t bitidx;
} hb_mc_dram_pa_bitfield;

static inline
unsigned long long
hb_mc_dram_pa_bitfield_get(const hb_mc_dram_pa_bitfield *bf, unsigned long long address)
{
        unsigned long long mask  = (1<<bf->bits)-1;
        return (address >> bf->bitidx) & mask;
}

typedef struct __hb_mc_memsys_t {
        // memory system parameters
        hb_mc_memsys_id_t id;
        // memory system features
        uint32_t feature_dma;
        uint32_t feature_cache;
        // dram address bitfields
        hb_mc_dram_pa_bitfield dram_ro;
        hb_mc_dram_pa_bitfield dram_bg;
        hb_mc_dram_pa_bitfield dram_ba;
        hb_mc_dram_pa_bitfield dram_co;
        hb_mc_dram_pa_bitfield dram_byte_offset;
} hb_mc_memsys_t;

typedef enum {
        HB_MC_MEMSYS_ROM_IDX_ID,
        HB_MC_MEMSYS_ROM_IDX_FEATURE_DMA,
        HB_MC_MEMSYS_ROM_IDX_FEATURE_CACHE,
        HB_MC_MEMSYS_ROM_IDX_DRAM_RO_BITS,
        HB_MC_MEMSYS_ROM_IDX_DRAM_BG_BITS,
        HB_MC_MEMSYS_ROM_IDX_DRAM_BA_BITS,
        HB_MC_MEMSYS_ROM_IDX_DRAM_CO_BITS,
        HB_MC_MEMSYS_ROM_IDX_DRAM_BYTE_OFF_BITS,
        HB_MC_MEMSYS_ROM_IDX_DRAM_RO_BITIDX,
        HB_MC_MEMSYS_ROM_IDX_DRAM_BG_BITIDX,
        HB_MC_MEMSYS_ROM_IDX_DRAM_BA_BITIDX,
        HB_MC_MEMSYS_ROM_IDX_DRAM_CO_BITIDX,
        HB_MC_MEMSYS_ROM_IDX_DRAM_BYTE_OFF_BITIDX,
        HB_MC_MEMSYS_ROM_IDX_MAX,
} hb_mc_memsys_rom_idx_t;

int hb_mc_memsys_init(const hb_mc_rom_word_t *rom_data, hb_mc_memsys_t *memsys);


unsigned long long
hb_mc_memsys_map_to_physical_channel_address(const hb_mc_memsys_t *memsys, unsigned long long address);

#ifdef __cplusplus
}
#endif
