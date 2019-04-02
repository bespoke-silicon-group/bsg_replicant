#ifndef BSG_MANYCORE_BITS_H
#define BSG_MANYCORE_BITS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t hb_mc_get_bits (uint32_t val, uint32_t start, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
