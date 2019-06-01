#ifndef BSG_MANYCORE_ORIGIN_EVA_MAP_H
#define BSG_MANYCORE_ORIGIN_EVA_MAP_H

#include <bsg_manycore_features.h>
#include <bsg_manycore_eva.h>
#include <bsg_manycore_coordinate.h>
#ifdef __cplusplus
#else
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize an EVA map for tiles centered at an origin.
 * @param[in] map     An EVA<->NPA map to initialize.
 * @param[in] origin  An origin tile around which the map is centered.
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
__attribute__((warn_unused_result))
int  hb_mc_origin_eva_map_init(hb_mc_eva_map_t *map, hb_mc_coordinate_t origin);

/**
 * Cleanup an EVA map for tiles centered at an origin.
 * @param[in] map  An EVA<->NPA map initialized with hb_mc_origin_eva_map_init().
 */
void hb_mc_origin_eva_map_exit(hb_mc_eva_map_t *map);

#ifdef __cplusplus
}
#endif
#endif
