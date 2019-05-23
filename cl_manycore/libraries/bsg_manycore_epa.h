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

#ifdef __cplusplus
};
#endif
#endif
