#ifndef BSG_MANYCORE_H
#define BSG_MANYCORE_H

#ifndef COSIM
#include <bsg_manycore_features.h>
#include <bsg_manycore_driver.h>
#include <bsg_manycore_mem.h>
#else
#include "bsg_manycore_features.h"
#include "bsg_manycore_driver.h"
#include "bsg_manycore_mem.h"
#endif

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define error_code __attribute__((warn_unused_result)) int

typedef uint32_t hb_mc_config_t[HB_MC_CONFIG_MAX];

typedef int hb_mc_manycore_id_t;
#define HB_MC_MANYCORE_ID_ANY -1

typedef struct hb_mc_manycore {
	hb_mc_manycore_id_t id; //!< which manycore instance is this
	const char    *name;     //!< the name of this manycore
	uintptr_t      mmio;     //!< pointer to memory mapped io
	hb_mc_config_t config;   //!< configuration of the manycore
} hb_mc_manycore_t;

///////////////////
// Init/Exit API //
///////////////////

/**
 * Initialize a manycore instance
 * @param[in] mc    A manycore to initialize
 * @param[in] name  A name to give this manycore instance (used for debugging)
 * @param[in] id    ID which selects the physical hardware from which this manycore is configured
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
error_code  hb_mc_manycore_init(hb_mc_manycore_t *mc, const char *name, hb_mc_manycore_id_t id);

/**
 * Cleanup an initialized manycore instance
 * @param[in] mc   A manycore instance that has been initialized with hb_mc_manycore_init()
 */
void hb_mc_manycore_exit(hb_mc_manycore_t *mc);

////////////////
// Packet API //
////////////////

/**
 * Transmit a packet to manycore hardware
 * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] packet A packet to transmit to manycore hardware
 * @param[in] type   Is this packet a request or response packet?
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
error_code hb_mc_manycore_packet_tx(hb_mc_manycore_t *mc,
				    hb_mc_packet_t *packet,
				    hb_mc_fifo_tx_t type);

/**
 * Receive a packet from manycore hardware
 * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] packet A packet into which data should be read
 * @param[in] type   Is this packet a request or response packet?
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
error_code hb_mc_manycore_packet_rx(hb_mc_manycore_t *mc,
				    hb_mc_packet_t *packet,
				    hb_mc_fifo_rx_t type);
////////////////
// Memory API //
////////////////

error_code hb_mc_manycore_read8(hb_mc_manycore_t *mc, npa_t *npa, uint8_t *vp);

error_code hb_mc_manycore_read16(hb_mc_manycore_t *mc, npa_t *npa, uint16_t *vp);

error_code hb_mc_manycore_read32(hb_mc_manycore_t *mc, npa_t *npa, uint32_t *vp);


error_code hb_mc_manycore_write8(hb_mc_manycore_t *mc, npa_t *npa, uint8_t v);

error_code hb_mc_manycore_write16(hb_mc_manycore_t *mc, npa_t *npa, uint16_t v);

error_code hb_mc_manycore_write32(hb_mc_manycore_t *mc, npa_t *npa, uint32_t v);

error_code hb_mc_manycore_write_mem(hb_mc_manycore_t *mc, npa_t *npa,
				    const void *data, size_t sz);

error_code hb_mc_manycore_read_mem(hb_mc_manycore_t *mc, npa_t *npa,
				   void *data, size_t sz);
/////////////////////////////
// Address Translation API //
/////////////////////////////

error_code hb_mc_manycore_npa_to_eva(hb_mc_manycore_t *mc, npa_t *npa, eva_t *eva, ...);

error_code hb_mc_manycore_eva_to_npa(hb_mc_manycore_t *mc, eva_t eva,  npa_t *npa, ...);

#undef error_code

#ifdef __cplusplus    
}
#endif
#endif
