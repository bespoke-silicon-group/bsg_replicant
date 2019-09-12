#ifndef BSG_MANYCORE_BITS_H
#define BSG_MANYCORE_BITS_H
#include <bsg_manycore_features.h>

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

uint32_t hb_mc_get_bits (uint32_t val, uint32_t start, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
