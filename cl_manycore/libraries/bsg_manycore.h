#ifndef BSG_MANYCORE_H
#define BSG_MANYCORE_H

#ifndef COSIM
#include <bsg_manycore_features.h>
#include <bsg_manycore_config.h>
#include <bsg_manycore_npa.h>
#include <bsg_manycore_epa.h>
#include <bsg_manycore_driver.h>
#else
#include "bsg_manycore_features.h"
#include "bsg_manycore_config.h"
#include "bsg_manycore_npa.h"
#include "bsg_manycore_epa.h"
#include "bsg_manycore_driver.h"
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

typedef int hb_mc_manycore_id_t;
#define HB_MC_MANYCORE_ID_ANY -1

typedef struct hb_mc_manycore {
        hb_mc_manycore_id_t id; //!< which manycore instance is this
        const char    *name;     //!< the name of this manycore
        uintptr_t      mmio;     //!< pointer to memory mapped io
        hb_mc_config_t config;   //!< configuration of the manycore
        void    *private_data;   //!< implementation private data
} hb_mc_manycore_t;

#define HB_MC_MANYCORE_INIT {0}
/*********************/
/* Configuration API */
/*********************/
static hb_mc_coordinate_t hb_mc_manycore_get_host_coordinate(hb_mc_manycore_t *mc)
{
        return hb_mc_config_get_host_interface(&mc->config);
}

///////////////////
// Init/Exit API //
///////////////////

/**
 * Initialize a manycore instance
 * @param[in] mc    A manycore to initialize. This must be zeroed memory.
 * @param[in] name  A name to give this manycore instance (used for debugging)
 * @param[in] id    ID which selects the physical hardware from which this manycore is configured
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
__attribute__((warn_unused_result))
int  hb_mc_manycore_init(hb_mc_manycore_t *mc, const char *name, hb_mc_manycore_id_t id);

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
 * @param[in] mc      A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] packet  A packet to transmit to manycore hardware
 * @param[in] type    Is this packet a request or response packet?
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
__attribute__((warn_unused_result))
int hb_mc_manycore_packet_tx(hb_mc_manycore_t *mc,
                             hb_mc_packet_t *packet,
                             hb_mc_fifo_tx_t type,
                             long timeout);

/**
 * Receive a packet from manycore hardware
 * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] packet A packet into which data should be read
 * @param[in] type   Is this packet a request or response packet?
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
__attribute__((warn_unused_result))
int hb_mc_manycore_packet_rx(hb_mc_manycore_t *mc,
                             hb_mc_packet_t *packet,
                             hb_mc_fifo_rx_t type,
                             long timeout);
////////////////
// Memory API //
////////////////

/**
 * Read one byte from manycore hardware at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t
 * @param[out] vp     A byte to be set to the data read
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
__attribute__((warn_unused_result))
int hb_mc_manycore_read8(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, uint8_t *vp);

/**
 * Read a 16-bit half-word from manycore hardware at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t aligned to a two byte boundary
 * @param[out] vp     A half-word to be set to the data read
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
__attribute__((warn_unused_result))
int hb_mc_manycore_read16(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, uint16_t *vp);

/**
 * Read a 32-bit word from manycore hardware at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t aligned to a four byte boundary
 * @param[out] vp     A word to be set to the data read
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
__attribute__((warn_unused_result))
int hb_mc_manycore_read32(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, uint32_t *vp);

/**
 * Write one byte to manycore hardware at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t
 * @param[in]  v      A byte value to be written out
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
__attribute__((warn_unused_result))
int hb_mc_manycore_write8(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, uint8_t v);

/**
 * Write a 16-bit half-word to manycore hardware at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t aligned to a two byte boundary
 * @param[in]  v      A half-word value to be written out
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
__attribute__((warn_unused_result))
int hb_mc_manycore_write16(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, uint16_t v);

/**
 * Write a 32-bit word to manycore hardware at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t aligned to a four byte boundary
 * @param[in]  v      A word value to be written out
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
__attribute__((warn_unused_result))
int hb_mc_manycore_write32(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, uint32_t v);

/**
 * Write memory out to manycore hardware starting at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t
 * @param[in]  data   A buffer to be written out manycore hardware
 * @param[in]  sz     The number of bytes to write to manycore hardware
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
__attribute__((warn_unused_result))
int hb_mc_manycore_write_mem(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa,
                             const void *data, size_t sz);

/**
 * Read memory from manycore hardware starting at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t
 * @param[in]  data   A buffer into which data will be read
 * @param[in]  sz     The number of bytes to read from manycore hardware
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
__attribute__((warn_unused_result))
int hb_mc_manycore_read_mem(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa,
                            void *data, size_t sz);
/************/
/* MMIO API */
/************/
/**
 * Read one byte from manycore hardware at a given AXI Address
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  offset An  offset into the manycore's MMIO address space
 * @param[out] vp     A byte to be set to the data read
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
__attribute__((warn_unused_result))
int hb_mc_manycore_mmio_read8(hb_mc_manycore_t *mc, uintptr_t offset, uint8_t *vp);

/**
 * Read a 16-bit half-word from manycore hardware at a given AXI Address
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  offset An  offset into the manycore's MMIO address space
 * @param[out] vp     A half-word to be set to the data read
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
__attribute__((warn_unused_result))
int hb_mc_manycore_mmio_read16(hb_mc_manycore_t *mc, uintptr_t offset, uint16_t *vp);

/**
 * Read a 32-bit word from manycore hardware at a given AXI Address
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  offset An  offset into the manycore's MMIO address space
 * @param[out] vp     A word to be set to the data read
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
__attribute__((warn_unused_result))
int hb_mc_manycore_mmio_read32(hb_mc_manycore_t *mc, uintptr_t offset, uint32_t *vp);

/**
 * Write one byte to manycore hardware at a given AXI Address
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  offset An  offset into the manycore's MMIO address space
 * @param[in]  v      A byte value to be written out
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
__attribute__((warn_unused_result))
int hb_mc_manycore_mmio_write8(hb_mc_manycore_t *mc, uintptr_t offset, uint8_t v);

/**
 * Write a 16-bit half-word to manycore hardware at a given AXI Address
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  offset An  offset into the manycore's MMIO address space
 * @param[in]  v      A half-word value to be written out
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
__attribute__((warn_unused_result))
int hb_mc_manycore_mmio_write16(hb_mc_manycore_t *mc, uintptr_t offset, uint16_t v);

/**
 * Write a 32-bit word to manycore hardware at a given AXI Address
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  offset An  offset into the manycore's MMIO address space
 * @param[in]  v      A word value to be written out
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
__attribute__((warn_unused_result))
int hb_mc_manycore_mmio_write32(hb_mc_manycore_t *mc, uintptr_t offset, uint32_t v);

/////////////////////////////
// Address Translation API //
/////////////////////////////


/**
 * Get a pointer to the configuration struct
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return NULL if uninitialized, else hb_mc_config_t*
 */
static inline const hb_mc_config_t* hb_mc_manycore_get_config(const hb_mc_manycore_t *mc)
{
        return &mc->config;
}

/**
 * Get the max size of program text.
 */
static inline size_t hb_mc_manycore_get_max_program_text_size(const hb_mc_manycore_t* mc,
							      const hb_mc_coordinate_t *tile)
{
    return 16ul * (1ul<<20ul); // 16M -- this might be later from the config
}
/**
 * Get the size of a tiles local data memory.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  tile   The coordinate tile to query for its data memory size.
 * @return the size of the tiles data memory.
 */
static inline size_t hb_mc_manycore_get_dmem_size(const hb_mc_manycore_t *mc,
						  const hb_mc_coordinate_t *tile)
{
	return 4 * (1<<10); //4K -- this might be later read from the config
}


/**
 * Get the size of a tiles local data memory.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  tile   The coordinate tile to query for its instruction cache size.
 * @return the size of the tiles instruction cache.
 */
static inline size_t hb_mc_manycore_get_icache_size(const hb_mc_manycore_t *mc,
						    const hb_mc_coordinate_t *tile)
{
	return 4 * (1<<10); //4K -- this might be later read from the config
}


/**
 * Get the size of DRAM.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  tile   The coordinate tile to query for its instruction cache size.
 * @return the size of the tiles instruction cache.
 */
static inline size_t hb_mc_manycore_get_dram_size(const hb_mc_manycore_t *mc)
{
	return 4ul * (1ul<<30ul); //4G -- this might be later read from the config
}

/**
 * Read the number of remaining available host credits
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_FAIL if an error occured. Number of remaining host credits otherwise
 */
int hb_mc_manycore_get_host_credits(hb_mc_manycore_t *mc);

/**
 * Freeze a tile.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] tile   A tile to freeze.
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
__attribute__((warn_unused_result))
int hb_mc_manycore_freeze_tile(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile);

/**
 * Unfreeze a tile.
 * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
 * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
 * @param[in] tile   A tile to unfreeze.
 * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
 */
__attribute__((warn_unused_result))
int hb_mc_manycore_unfreeze_tile(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile);

#ifdef __cplusplus
}
#endif
#endif
