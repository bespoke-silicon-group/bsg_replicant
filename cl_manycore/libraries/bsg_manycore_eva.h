#ifndef BSG_MANYCORE_EVA_H
#define BSG_MANYCORE_EVA_H

#ifndef COSIM
#include <bsg_manycore_features.h>
#include <bsg_manycore_coordinate.h>
#include <bsg_manycore.h>
#include <bsg_manycore_npa.h>
#include <bsg_manycore_epa.h>
#else
#include "bsg_manycore_features.h"
#include "bsg_manycore_coordinate.h"
#include "bsg_manycore.h"
#include "bsg_manycore_npa.h"
#include "bsg_manycore_epa.h"
#endif

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t hb_mc_eva_t;

typedef struct __hb_mc_eva_id_t{
	int (*eva_to_npa)(const hb_mc_config_t *cfg, const hb_mc_coordinate_t *c,
			const hb_mc_eva_t *eva, hb_mc_npa_t *npa, size_t *sz);
	int (*npa_to_eva)(const hb_mc_config_t *cfg, const hb_mc_coordinate_t *c,
			const hb_mc_npa_t *npa, hb_mc_eva_t *eva, size_t *sz);
} hb_mc_eva_id_t;

extern hb_mc_eva_id_t default_eva;

/**
 * Translates a Network Physical Address to an Endpoint Virtual Address
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  id     An eva ID for computing the eva to npa map
 * @param[in]  c      A coordinate for which #eva will be formatted
 * @param[in]  npa    A valid hb_mc_npa_t to translate
 * @param[out] eva    An eva to be set by translating #npa
 * @param[out] sz     The size in bytes of the EVA segment for the #npa
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
__attribute__((warn_unused_result))
int hb_mc_npa_to_eva(const hb_mc_config_t *cfg,
		     const hb_mc_eva_id_t *id, const hb_mc_coordinate_t *c,
		     const hb_mc_npa_t *npa, hb_mc_eva_t *eva, size_t *sz);

/**
 * Translate an Endpoint Virtual Address to a Network Physical Address
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  id     An eva ID for computing the eva to npa map
 * @param[in]  c      A target tile to compute #npa
 * @param[in]  eva    An eva to translate
 * @param[out] npa    An npa to be set by translating #eva and #id
 * @param[out] sz     The size in bytes of the NPA segment for the #eva
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
__attribute__((warn_unused_result))
int hb_mc_eva_to_npa(const hb_mc_config_t *cfg,
		     const hb_mc_eva_id_t *id,
		     const hb_mc_coordinate_t *c, const hb_mc_eva_t *eva,
		     hb_mc_npa_t *npa, size_t *sz);

/**
 * Write memory out to manycore hardware starting at a given EVA
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  id     An eva ID for computing the eva to npa map
 * @param[in]  c      A target tile to compute the NPA
 * @param[in]  eva    A valid eva_t
 * @param[in]  data   A buffer to be written out manycore hardware
 * @param[in]  sz     The number of bytes to write to manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
// I think this is impossible to implement with this signature?
__attribute__((warn_unused_result))
int hb_mc_manycore_eva_write(const hb_mc_config_t *cfg,
			     const hb_mc_eva_id_t *id,
			     const hb_mc_coordinate_t *c, const hb_mc_eva_t *eva,
			     const void *data, size_t sz);

/**
 * Read memory from manycore hardware starting at a given EVA
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  id     An eva ID for computing the eva to npa map
 * @param[in]  c      A coordinate on the the Manycore
 * @param[in]  eva    A valid eva_t
 * @param[out] data   A buffer into which data will be read
 * @param[in]  sz     The number of bytes to read from the manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
// I think this is impossible to implement with this signature?
__attribute__((warn_unused_result))
int hb_mc_manycore_eva_read(const hb_mc_config_t *cfg,
			    const hb_mc_eva_id_t *id,
			    const hb_mc_coordinate_t *c, const hb_mc_eva_t *eva,
			    const void *data, size_t sz);

// What is this function supposed to do?
static inline uint32_t hb_mc_eva_addr(const hb_mc_eva_t *eva)
{
	return *eva;
}
#ifdef __cplusplus
}
#endif

#endif
