#include <bsg_manycore_memsys.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_errno.h>
#include <string.h>
#include <inttypes.h>

static inline int is_power2(uint32_t x)
{
        return __builtin_popcount(x) == 1;
}

const char * hb_mc_memsys_id_to_string(hb_mc_memsys_id_t id)
{
        static const char * strtab [] = {
                [HB_MC_MEMSYS_ID_NONE] = "No Memory System",
                [HB_MC_MEMSYS_ID_AXI4] = "AXI4 Memory Controller",
                [HB_MC_MEMSYS_ID_INFMEM] = "Ideal Memory System",
                [HB_MC_MEMSYS_ID_DRAMSIM3] = "DRAMSim3 HBM2",
        };

        return strtab[id];
}

// Decode a raw memory system ID read from the ROM and translate it to an integer ID
static
int hb_mc_memsys_get_id_from_rom_value(
        uint32_t rom_id,
        hb_mc_memsys_id_t *id)
{
        if (memcmp(HB_MC_ROM_MEMSYS_ID_NONE, &rom_id, sizeof(rom_id))==0)
                *id = HB_MC_MEMSYS_ID_NONE;
        else if (memcmp(HB_MC_ROM_MEMSYS_ID_AXI4, &rom_id, sizeof(rom_id))==0)
                *id = HB_MC_MEMSYS_ID_AXI4;
        else if (memcmp(HB_MC_ROM_MEMSYS_ID_INFMEM, &rom_id, sizeof(rom_id))==0)
                *id = HB_MC_MEMSYS_ID_INFMEM;
        else if (memcmp(HB_MC_ROM_MEMSYS_ID_DRAMSIM3, &rom_id, sizeof(rom_id))==0)
                *id = HB_MC_MEMSYS_ID_DRAMSIM3;
        else { // error - bad memory system ID
                bsg_pr_err("%s: Invalid Memory System ID 0x%" PRIx32 "\n",
                           __func__, rom_id);
                return HB_MC_INVALID;
        }

        bsg_pr_dbg("%s: Memory System: %s\n",
                   __func__, hb_mc_memsys_id_to_string(*id));

        return HB_MC_SUCCESS;
}

// Set the features supported by the memory system
static
int hb_mc_memsys_set_features(hb_mc_memsys_t *memsys)
{
        switch (memsys->id) {
        case HB_MC_MEMSYS_ID_INFMEM:
                memsys->feature_cache = 0;
                memsys->feature_dma = 1;
                break;
        case HB_MC_MEMSYS_ID_DRAMSIM3:
                memsys->feature_cache = 1;
                memsys->feature_dma = 1;
                break;
        case HB_MC_MEMSYS_ID_NONE: // TODO - throw this error somewhere else?
                return HB_MC_INVALID;
        default: // by default we support cache but not DMA
                memsys->feature_cache = 1;
                memsys->feature_dma = 0;
        }

        return HB_MC_SUCCESS;
}

/*
  check if a condition is true and print a helpful error message if it fails
*/
#define CHECK(value, condition)                                         \
        do {                                                            \
                if (!(condition)) {                                     \
                        bsg_pr_err("%s: expected '" #condition "' failed: bad value %" PRIu32 "\n", \
                                   __func__, value);                    \
                        return HB_MC_INVALID;                           \
                }                                                       \
        } while (0)

/*
  check that a bit index is in [0,30] and print a helpful error message if not
 */
#define DRAMSIM3_CHECK_BITIDX(value)                    \
        CHECK(value, (value >= 0 && value <= 30))

// dramsim3 verification function
static
int hb_mc_memsys_check_dram_address_map_info_dramsim3(const hb_mc_memsys_t *memsys)
{
        bsg_pr_dbg("%s: checking row bits\n",
                   __func__);

        CHECK(memsys->dram_ro.bits,
              memsys->dram_ro.bits == 14
              || memsys->dram_ro.bits == 15);

        bsg_pr_dbg("%s: checking column bits\n",
                   __func__);

        CHECK(memsys->dram_co.bits,
              memsys->dram_co.bits == 6);

        bsg_pr_dbg("%s: checking bank bits\n",
                   __func__);

        CHECK(memsys->dram_ba.bits,
              memsys->dram_ba.bits == 2);

        bsg_pr_dbg("%s: checking bank group bits\n",
                   __func__);

        CHECK(memsys->dram_bg.bits,
              memsys->dram_bg.bits == 2);

        bsg_pr_dbg("%s: checking byte offset bits\n",
                   __func__);

        CHECK(memsys->dram_byte_offset.bits,
              memsys->dram_byte_offset.bits == 5);

        bsg_pr_dbg("%s: checking row bit index\n",
                   __func__);

        DRAMSIM3_CHECK_BITIDX(memsys->dram_ro.bitidx);

        bsg_pr_dbg("%s: checking column bit index\n",
                   __func__);

        DRAMSIM3_CHECK_BITIDX(memsys->dram_co.bitidx);

        bsg_pr_dbg("%s: checking bank bit index\n",
                   __func__);

        DRAMSIM3_CHECK_BITIDX(memsys->dram_ba.bitidx);

        bsg_pr_dbg("%s: checking bank group bit index\n",
                   __func__);

        DRAMSIM3_CHECK_BITIDX(memsys->dram_bg.bitidx);

        bsg_pr_dbg("%s: checking byte offset index\n",
                   __func__);

        DRAMSIM3_CHECK_BITIDX(memsys->dram_byte_offset.bitidx);

        return HB_MC_SUCCESS;
}

// check that value is zero and print a helpful error message if not
#define CHECK_ZERO(value) \
        CHECK(value, (value == 0))

// check that value is one and print a helpful error message if not
#define CHECK_ONE(value) \
        CHECK(value, (value == 1))

// default memory system verification function
static
int hb_mc_memsys_check_dram_address_map_info_generic(const hb_mc_memsys_t *memsys)
{
        CHECK_ZERO(memsys->dram_ro.bits);
        CHECK_ZERO(memsys->dram_co.bits);
        CHECK_ZERO(memsys->dram_bg.bits);
        CHECK_ZERO(memsys->dram_ba.bits);
        CHECK(memsys->dram_byte_offset.bits, memsys->dram_byte_offset.bits >= 28);
        return HB_MC_SUCCESS;
}

// default memory system verification function
static
int hb_mc_memsys_set_dram_address_map_info(hb_mc_memsys_t *memsys,
                                           const hb_mc_rom_word_t *rom_data)
{
        memsys->dram_ro.bits = rom_data[HB_MC_MEMSYS_ROM_IDX_DRAM_RO_BITS];
        memsys->dram_bg.bits = rom_data[HB_MC_MEMSYS_ROM_IDX_DRAM_BG_BITS];
        memsys->dram_ba.bits = rom_data[HB_MC_MEMSYS_ROM_IDX_DRAM_BA_BITS];
        memsys->dram_co.bits = rom_data[HB_MC_MEMSYS_ROM_IDX_DRAM_CO_BITS];
        memsys->dram_byte_offset.bits = rom_data[HB_MC_MEMSYS_ROM_IDX_DRAM_BYTE_OFF_BITS];

        memsys->dram_ro.bitidx = rom_data[HB_MC_MEMSYS_ROM_IDX_DRAM_RO_BITIDX];
        memsys->dram_bg.bitidx = rom_data[HB_MC_MEMSYS_ROM_IDX_DRAM_BG_BITIDX];
        memsys->dram_ba.bitidx = rom_data[HB_MC_MEMSYS_ROM_IDX_DRAM_BA_BITIDX];
        memsys->dram_co.bitidx = rom_data[HB_MC_MEMSYS_ROM_IDX_DRAM_CO_BITIDX];
        memsys->dram_byte_offset.bitidx = rom_data[HB_MC_MEMSYS_ROM_IDX_DRAM_BYTE_OFF_BITIDX];

        if (memsys->id == HB_MC_MEMSYS_ID_DRAMSIM3) {
                return hb_mc_memsys_check_dram_address_map_info_dramsim3(memsys);
        } else {
                return hb_mc_memsys_check_dram_address_map_info_generic(memsys);
        }
}

static
int hb_mc_memsys_set_dram_channels(hb_mc_memsys_t *memsys,
                                   const hb_mc_rom_word_t *rom_data)
{
        memsys->dram_channels = rom_data[HB_MC_MEMSYS_ROM_IDX_DRAM_CHANNELS];

        switch (memsys->id) {
        case HB_MC_MEMSYS_ID_DRAMSIM3:
                CHECK(memsys->dram_channels,
                      memsys->dram_channels <= 8);
                break;
        case HB_MC_MEMSYS_ID_INFMEM:
                break; // can be arbitrarily large
        default: // by default we should expect one channel
                CHECK_ONE(memsys->dram_channels);
        }

        return HB_MC_SUCCESS;
}

static
int hb_mc_memsys_set_dram_bank_size(hb_mc_memsys_t *memsys,
                                    const hb_mc_rom_word_t *rom_data)
{
        memsys->dram_bank_size = rom_data[HB_MC_MEMSYS_ROM_IDX_DRAM_BANK_SIZE];
        CHECK(memsys->dram_bank_size, is_power2(memsys->dram_bank_size));

        return HB_MC_SUCCESS;
}

int hb_mc_memsys_init(const hb_mc_rom_word_t *rom_data, hb_mc_memsys_t *memsys)
{
        int err;

        // get the id
        err = hb_mc_memsys_get_id_from_rom_value(
                rom_data[HB_MC_MEMSYS_ROM_IDX_ID],
                &memsys->id);

        if (err != HB_MC_SUCCESS)
                return HB_MC_INVALID;

        bsg_pr_dbg("%s: setting memory system features\n",
                   __func__);

        err = hb_mc_memsys_set_features(memsys);
        if (err != HB_MC_SUCCESS)
                return HB_MC_INVALID;

        bsg_pr_dbg("%s: setting DRAM channels\n", __func__);

        err = hb_mc_memsys_set_dram_channels(memsys, rom_data);
        if (err != HB_MC_SUCCESS)
                return HB_MC_INVALID;

        bsg_pr_dbg("%s: setting DRAM bank size\n", __func__);
        err = hb_mc_memsys_set_dram_bank_size(memsys, rom_data);
        if (err != HB_MC_SUCCESS)
                return HB_MC_INVALID;

        bsg_pr_dbg("%s: DRAM address map information\n",
                   __func__);

        err = hb_mc_memsys_set_dram_address_map_info(memsys, rom_data);
        if (err != HB_MC_SUCCESS)
                return HB_MC_INVALID;

        return HB_MC_SUCCESS;
}


static
unsigned long long
hb_mc_memsys_map_to_physical_channel_address_dramsim3(const hb_mc_memsys_t *memsys, unsigned long long address)
{

        unsigned long long ro = hb_mc_dram_pa_bitfield_get(&memsys->dram_ro, address);
        unsigned long long bg = hb_mc_dram_pa_bitfield_get(&memsys->dram_bg, address);
        unsigned long long ba = hb_mc_dram_pa_bitfield_get(&memsys->dram_ba, address);
        unsigned long long co = hb_mc_dram_pa_bitfield_get(&memsys->dram_co, address);
        unsigned long long byte_offset = hb_mc_dram_pa_bitfield_get(&memsys->dram_byte_offset, address);

        // dramsim3 mapping is ro,bg,ba,co,byte_offset
        unsigned long dram_address = 0;
        dram_address = (dram_address | ro);
        dram_address = (dram_address << memsys->dram_bg.bits) | bg;
        dram_address = (dram_address << memsys->dram_ba.bits) | ba;
        dram_address = (dram_address << memsys->dram_co.bits) | co;
        dram_address = (dram_address << memsys->dram_byte_offset.bits) | byte_offset;

        bsg_pr_dbg("%s: mapping %09llx to %09llx {ro: %llu, bg: %llu, ba: %llu, co: %llu, off: %llx}\n",
                   __func__, address, dram_address,
                   ro, bg, ba, co, byte_offset);

        return dram_address;
}

unsigned long long
hb_mc_memsys_map_to_physical_channel_address(const hb_mc_memsys_t *memsys, unsigned long long address)
{
        switch (memsys->id)
        {
        case HB_MC_MEMSYS_ID_DRAMSIM3:
                return hb_mc_memsys_map_to_physical_channel_address_dramsim3(memsys, address);
        default:
                return address;
        }
}
