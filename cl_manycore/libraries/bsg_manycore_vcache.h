/* Defines Victim Cache Macros and Constants */
#ifndef BSG_MANYCORE_VCACHE_H
#define BSG_MANYCORE_VCACHE_H
#include <bsg_manycore_features.h>
#include <bsg_manycore_epa.h>

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Offsets in bytes */
#define HB_MC_VCACHE_EPA_BASE          0x00000000
#define HB_MC_VCACHE_EPA_OFFSET_DRAM   0x00000000
#define HB_MC_VCACHE_EPA_OFFSET_TAG    0x20000000
#define HB_MC_VCACHE_EPA_RESERVED_BITS 1

/* EPA Macros */
#define EPA_VCACHE_FROM_BYTE_OFFSET(offset)			\
	EPA_FROM_BASE_AND_OFFSET(HB_MC_VCACHE_EPA_BASE, offset)

#define HB_MC_VCACHE_EPA_DRAM						\
	EPA_VCACHE_FROM_BYTE_OFFSET(HB_MC_VCACHE_EPA_OFFSET_DRAM)

#define HB_MC_VCACHE_EPA_TAG						\
	EPA_VCACHE_FROM_BYTE_OFFSET(HB_MC_VCACHE_EPA_OFFSET_TAG)

/* Victim Cache Data Bits */
#define HB_MC_VCACHE_VALID_BITIDX 31
#define HB_MC_VCACHE_VALID (1 << HB_MC_VCACHE_VALID_BITIDX)

#ifdef __cplusplus
};
#endif

#endif
