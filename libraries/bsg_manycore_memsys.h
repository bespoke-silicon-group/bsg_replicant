#pragma once
#include <bsg_manycore_features.h>
#include <stdint.h>

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

int hb_mc_memsys_get_id_from_rom_value(
        uint32_t rom_id,
        hb_mc_memsys_id_t *id);

#ifdef __cplusplus
}
#endif
