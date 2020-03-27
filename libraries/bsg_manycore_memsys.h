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
        HB_MC_MEMSYS_ID_NONE,     //!< No memory system
        HB_MC_MEMSYS_ID_AXI4,     //!< AXI4 memory controller
        HB_MC_MEMSYS_ID_INFMEM,   //!< Ideal single-cycle memory
        HB_MC_MEMSYS_ID_DRAMSIM3, //!< DRAMSim3 memory controller
} hb_mc_memsys_id_t;

/**
 * Get a human readable string from memory system ID.
 * @param[in] id - A memory system ID.
 * @return A human readable string that describes the memory system.
 */
const char *hb_mc_memsys_id_to_string(
        hb_mc_memsys_id_t id);

typedef struct __hb_mc_dram_pa_bitfield {
        uint32_t bits;   //!< How many bits wide is this field?
        uint32_t bitidx; //!< What is the LSB of this bitfield?
} hb_mc_dram_pa_bitfield;

/**
 * Get a bitfield from a DRAM address.
 * @param[in] bf - A bitfield structure.
 * @param[in] address - A DRAM address.
 * @return The value of the bitfield of the DRAM address.
 */
static inline
unsigned long long
hb_mc_dram_pa_bitfield_get(const hb_mc_dram_pa_bitfield *bf, unsigned long long address)
{
        unsigned long long mask  = (1<<bf->bits)-1;
        return (address >> bf->bitidx) & mask;
}

typedef struct __hb_mc_memsys_t {
        // memory system parameters
        hb_mc_memsys_id_t id; //!< What am I?
        uint32_t dram_channels; //!< How many DRAM channels?
        // here bank means 'region owned by a single manycore victim cache'
        // this doest NOT mean a phsyical DRAM bank
        uint32_t dram_bank_size; //!< How much DRAM does a single MC-V$ own?
        // memory system features
        uint32_t feature_dma; //!< Can I do DMA?
        uint32_t feature_cache; //!< Do I have DMA?
        // dram address bitfields
        hb_mc_dram_pa_bitfield dram_ro; //!< DRAM row bits info
        hb_mc_dram_pa_bitfield dram_bg; //!< DRAM bankgroup bits info
        hb_mc_dram_pa_bitfield dram_ba; //!< DRAM bank bits info
        hb_mc_dram_pa_bitfield dram_co; //!< DRAM columns bits info
        hb_mc_dram_pa_bitfield dram_byte_offset; //!< DRAM byte offset bits info
} hb_mc_memsys_t;

/* Word address of memory system values read from the ROM */
typedef enum {
        HB_MC_MEMSYS_ROM_IDX_ID,
        HB_MC_MEMSYS_ROM_IDX_DRAM_CHANNELS,
        HB_MC_MEMSYS_ROM_IDX_DRAM_BANK_SIZE,
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

/**
 * Initialize a memory system from ROM data.
 * @param[in] rom_data - A buffer HB_MC_MEMSYS_ROM_IDX_MAX in length
 * @param[in] memsys - A memsys structure to initialize
 * @return HB_MC_INVALID if there's an error and HB_MC_SUCCESS otherwise.
 */
int hb_mc_memsys_init(const hb_mc_rom_word_t *rom_data, hb_mc_memsys_t *memsys);

/**
 * Map a channel's DRAM address to it's physical address within the channel.
 * @param[in] memsys - A memory system
 * @param[in] address - A channel's DRAM address
 * @return A physical address within a DRAM channel
 *
 * This function never returns an error. Make sure memsys is initialized.
 */
unsigned long long
hb_mc_memsys_map_to_physical_channel_address(const hb_mc_memsys_t *memsys, unsigned long long address);

#ifdef __cplusplus
}
#endif
