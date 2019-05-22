#ifndef BSG_MANYCORE_MEM_H
#define BSG_MANYCORE_MEM_H

/* #ifndef _BSD_SOURCE */
/* 	#define _BSD_SOURCE */
/* #endif */
/* #ifndef _XOPEN_SOURCE */
/* 	#define _XOPEN_SOURCE 500 */
/* #endif */

#ifndef COSIM
#include <bsg_manycore_features.h>
#include <bsg_manycore_driver.h>
#include <bsg_manycore_coordinate.h>
#else
#include "bsg_manycore_features.h"
#include "bsg_manycore_driver.h"
#include "bsg_manycore_coordinate.h"
#endif

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t hb_mc_epa_t;
typedef uint32_t eva_id_t;

typedef uint32_t eva_t;

typedef struct {
	hb_mc_idx_t x;
	hb_mc_idx_t y; 
	hb_mc_epa_t epa;
} npa_t;

typedef struct __hb_mc_eva_id_t{
	int (*eva_to_npa)(const hb_mc_coordinate_t *c, const eva_t *eva, npa_t *npa, size_t *sz);
	int (*npa_to_eva)(const hb_mc_coordinate_t *c, const npa_t *npa, eva_t *eva, size_t *sz);
} hb_mc_eva_id_t;

int hb_mc_copy_from_epa (uint8_t fd, hb_mc_response_packet_t *buf, uint32_t x, uint32_t y, uint32_t epa, uint32_t size); 
int hb_mc_copy_to_epa (uint8_t fd, uint32_t x, uint32_t y, uint32_t epa, uint32_t *buf, uint32_t size);
int hb_mc_eva_to_npa (eva_id_t eva_id, eva_t eva, npa_t *npa);
int hb_mc_npa_to_eva (eva_id_t eva_id, npa_t *npa, eva_t *eva);

static inline npa_t hb_mc_epa_to_npa(hb_mc_coordinate_t c, hb_mc_epa_t epa){
	npa_t result = {hb_mc_coordinate_get_x(c), hb_mc_coordinate_get_y(c), epa};
	return result;
}

#ifdef __cplusplus
}
#endif

#endif
