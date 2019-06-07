#ifndef BSG_MANYCORE_MEM_H
#define BSG_MANYCORE_MEM_H

#include <bsg_manycore_features.h>
#include <bsg_manycore_driver.h>
#include <bsg_manycore_printing.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t hb_mc_epa_t; // Should be removed
typedef uint32_t eva_id_t; // Should be removed

typedef uint32_t eva_t; // TODO: Should be removed

typedef struct {
	hb_mc_idx_t x;
	hb_mc_idx_t y; 
	hb_mc_epa_t epa;
} npa_t; // Should be moved to bsg_manycore

int hb_mc_copy_from_epa (uint8_t fd, hb_mc_response_packet_t *buf, uint32_t x, uint32_t y, uint32_t epa, uint32_t size); 
int hb_mc_copy_to_epa (uint8_t fd, uint32_t x, uint32_t y, uint32_t epa, uint32_t *buf, uint32_t size);

__attribute__((deprecated))
int hb_mc_eva_to_npa_deprecated (eva_id_t eva_id, eva_t eva, npa_t *npa);
__attribute__((deprecated))
int hb_mc_npa_to_eva_deprecated (eva_id_t eva_id, npa_t *npa, eva_t *eva);


#ifdef __cplusplus
}
#endif

#endif
