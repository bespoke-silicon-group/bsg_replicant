#ifndef BSG_MANYCORE_EPA_H
#define BSG_MANYCORE_EPA_H
#include <bsg_manycore_features.h>

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An Endpoint Physical Address.
 * This type uniquely identifies a physical memory address within a manycore endpoint.
 */
typedef uint32_t hb_mc_epa_t;


#define EPA_FROM_BYTE_BASE_AND_OFFSET(base, offset)	\
    (((base)+(offset))>>2)

/*************/
/* Tile EPAs */
/*************/

/* Offsets in bytes */
#define HB_MC_TILE_EPA_DMEM_BASE   0x00001000
#define HB_MC_TILE_EPA_ICACHE_BASE 0x01000000
#define HB_MC_TILE_EPA_CSR_BASE                       0x20000
#define HB_MC_TILE_EPA_CSR_FREEZE_OFFSET              0x00
#define HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_X_OFFSET 0x04
#define HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_Y_OFFSET 0x08

#define EPA_TILE_CSR_FROM_BYTE_OFFSET(offset)				\
    EPA_FROM_BYTE_BASE_AND_OFFSET(HB_MC_TILE_EPA_CSR_BASE, offset)

/* EPAs */
#define HB_MC_TILE_EPA_ICACHE			\
    EPA_FROM_BYTE_BASE_AND_OFFSET(HB_MC_TILE_EPA_ICACHE_BASE, 0)

#define HB_MC_TILE_EPA_CSR_FREEZE		\
    EPA_TILE_CSR_FROM_BYTE_OFFSET(HB_MC_TILE_EPA_CSR_FREEZE_OFFSET)

#define HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_X				\
    EPA_TILE_CSR_FROM_BYTE_OFFSET(HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_X_OFFSET)

#define HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_Y				\
    EPA_TILE_CSR_FROM_BYTE_OFFSET(HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_Y_OFFSET)



/***********/
/* V$ EPAs */
/***********/

/* Offsets in bytes */
#define HB_MC_VCACHE_EPA_BASE        0x00000000
#define HB_MC_VCACHE_EPA_DRAM_OFFSET 0x00000000
#define HB_MC_VCACHE_EPA_TAG_OFFSET  0x20000000

/* EPAs */
#define EPA_VCACHE_FROM_BYTE_OFFSET(offset)				\
    EPA_FROM_BYTE_BASE_AND_OFFSET(HB_MC_VCACHE_EPA_BASE, offset)

#define HB_MC_VCACHE_EPA_DRAM					\
    EPA_VCACHE_FROM_BYTE_OFFSET(HB_MC_VCACHE_EPA_DRAM_OFFSET)

#define HB_MC_VCACHE_EPA_TAG					\
    EPA_VCACHE_FROM_BYTE_OFFSET(HB_MC_VCACHE_EPA_TAG_OFFSET)


#ifdef __cplusplus
};
#endif
#endif
