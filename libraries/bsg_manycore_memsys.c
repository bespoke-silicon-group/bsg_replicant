#include <bsg_manycore_memsys.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_errno.h>
#include <string.h>
#include <inttypes.h>

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
