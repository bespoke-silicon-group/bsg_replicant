#ifndef BSG_MANYCORE_CONFIG_H
#define BSG_MANYCORE_CONFIG_H

#include <bsg_manycore_features.h>
#include <bsg_manycore_coordinate.h>

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// We expect a 32-bit bus for Addresses, but the top bit is reserved for the
// victim cache tag.
#define HB_MC_CONFIG_MAX_BITWIDTH_ADDR 31
#define HB_MC_CONFIG_MAX_BITWIDTH_DATA 32

#define HB_MC_CONFIG_VCORE_BASE_X 0
#define HB_MC_CONFIG_VCORE_BASE_Y 1

typedef uint32_t hb_mc_config_raw_t;
/* Compilation Metadata */
typedef struct __hb_mc_version_t {
        uint8_t major;
        uint8_t minor;
        uint8_t revision;
} hb_mc_version_t;

typedef struct __hb_mc_date_t {
        uint8_t month;
        uint8_t day;
        uint16_t year;
} hb_mc_date_t;

typedef uint32_t hb_mc_githash_t;

typedef struct __hb_mc_config_t{
        hb_mc_version_t design_version;
        hb_mc_date_t timestamp;
        uint32_t network_bitwidth_addr;
        uint32_t network_bitwidth_data;
        hb_mc_dimension_t dimension;
        hb_mc_coordinate_t host_interface;
        hb_mc_githash_t basejump;
        hb_mc_githash_t manycore;
        hb_mc_githash_t f1;
} hb_mc_config_t;

typedef enum __hb_mc_config_id_t {
        HB_MC_CONFIG_MIN = 0,
        HB_MC_CONFIG_VERSION = 0,
        HB_MC_CONFIG_TIMESTAMP = 1,
        HB_MC_CONFIG_NETWORK_ADDR_WIDTH = 2,
        HB_MC_CONFIG_NETWORK_DATA_WIDTH = 3,
        HB_MC_CONFIG_DEVICE_DIM_X = 4,
        HB_MC_CONFIG_DEVICE_DIM_Y = 5,
        HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_X = 6,
        HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_Y = 7,
        HB_MC_CONFIG_NOT_IMPLEMENTED = 8,
        HB_MC_CONFIG_REPO_BASEJUMP_HASH = 9,
        HB_MC_CONFIG_REPO_MANYCORE_HASH = 10,
        HB_MC_CONFIG_REPO_F1_HASH = 11,
        HB_MC_CONFIG_MAX = 12
} hb_mc_config_id_t;

int hb_mc_config_init(const hb_mc_config_raw_t mc[HB_MC_CONFIG_MAX], hb_mc_config_t *config);
                

static inline uint64_t hb_mc_config_id_to_addr(uint64_t addr, hb_mc_config_id_t id)
{
        return (addr + (id << 2));
}

static inline const char *hb_mc_config_id_to_string(hb_mc_config_id_t id)
{
    static const char *strtab [] = {
        [HB_MC_CONFIG_VERSION] = "BLADERUNNER HARDWARE VERSION",
        [HB_MC_CONFIG_TIMESTAMP] = "BLADERUNNER COMPILATION DATE TIMESTAMP",
        [HB_MC_CONFIG_NETWORK_ADDR_WIDTH] = "BLADERUNNER NETWORK ADDRESS WDITH",
        [HB_MC_CONFIG_NETWORK_DATA_WIDTH] = "BLADERUNNER NETWORK DATA WIDTH",
        [HB_MC_CONFIG_DEVICE_DIM_X] = "BLADERUNNER DEVICE DIMENSION X",
        [HB_MC_CONFIG_DEVICE_DIM_Y] = "BLADERUNNER DEVICE DIMENSION Y",
        [HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_X] = "BLADERUNNER HOST INTERFACE DIMENSION X",
        [HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_Y] = "BLADERUNNER HOST INTERFACE DIMENSION Y",
        [HB_MC_CONFIG_NOT_IMPLEMENTED] = "BLADERUNNER NOT IMPLEMENTED",
        [HB_MC_CONFIG_REPO_BASEJUMP_HASH] = "BLADERUNNER REPO BASEJUMP HASH",
        [HB_MC_CONFIG_REPO_MANYCORE_HASH] = "BLADERUNNER REPO MANYCORE HASH",
        [HB_MC_CONFIG_REPO_F1_HASH] = "BLADERUNNER REPO F1 HASH",
    };
    return strtab[id];
}

static inline uint8_t hb_mc_config_get_version_major(const hb_mc_config_t *cfg){
        return cfg->design_version.major;
}

static inline uint8_t hb_mc_config_get_version_minor(const hb_mc_config_t *cfg){
        return cfg->design_version.minor;
}

static inline uint8_t hb_mc_config_get_version_revision(const hb_mc_config_t *cfg){
        return cfg->design_version.revision;
}

static inline uint8_t hb_mc_config_get_compilation_month(const hb_mc_config_t *cfg){
        return cfg->timestamp.month;
}

static inline uint8_t hb_mc_config_get_compilation_day(const hb_mc_config_t *cfg){
        return cfg->timestamp.day;
}

static inline uint16_t hb_mc_config_get_compilation_year(const hb_mc_config_t *cfg){
        return cfg->timestamp.year;
}

static inline hb_mc_githash_t hb_mc_config_get_githash_basejump(const hb_mc_config_t *cfg){
        return cfg->basejump;
}

static inline hb_mc_githash_t hb_mc_config_get_githash_manycore(const hb_mc_config_t *cfg){
        return cfg->manycore;
}

static inline hb_mc_githash_t hb_mc_config_get_githash_f1(const hb_mc_config_t *cfg){
        return cfg->f1;
}

/* Device Metadata */
static inline uint32_t hb_mc_config_get_network_bitwidth_addr(const hb_mc_config_t *cfg){
        return cfg->network_bitwidth_addr;
}

static inline uint32_t hb_mc_config_get_network_bitwidth_data(const hb_mc_config_t *cfg){
        return cfg->network_bitwidth_data;
}

static inline hb_mc_coordinate_t hb_mc_config_get_host_interface(const hb_mc_config_t *cfg){
        return cfg->host_interface;
}

static inline hb_mc_dimension_t hb_mc_config_get_dimension(const hb_mc_config_t *cfg){
        return cfg->dimension;
}

static inline hb_mc_idx_t hb_mc_config_get_vcore_base_y(const hb_mc_config_t *cfg){
        return HB_MC_CONFIG_VCORE_BASE_Y; // TODO: These should be defined in the ROM?
}

static inline hb_mc_idx_t hb_mc_config_get_vcore_base_x(const hb_mc_config_t *cfg){
        return HB_MC_CONFIG_VCORE_BASE_X; // TODO: These should be defined in the ROM?
}

static inline size_t hb_mc_config_get_dram_size(const hb_mc_config_t *cfg)
{
        return 4ul * (1ul<<30ul); // 4GB: TODO: should read out of the ROM
}

static inline size_t hb_mc_config_get_dmem_size(const hb_mc_config_t *cfg)
{
        return 4 * (1<<10); // 4K: this might be read from ROM if the value ever changes
}

#ifdef __cplusplus
}
#endif
#endif
