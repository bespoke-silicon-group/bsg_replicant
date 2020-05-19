#ifndef __BSG_MANYCORE_VERILATOR_HPP
#define __BSG_MANYCORE_VERILATOR_HPP
#include <bsg_manycore.h>

/**
 * Clean up the runtime machine
 * @param[in] mc    A manycore to clean up
 */
void hb_mc_machine_cleanup(hb_mc_manycore_t *mc);

/**
 * Initialize the runtime machine
 * @param[in] mc    A manycore to initialize
 * @param[in] id    ID which selects the physical hardware from which this manycore is configured
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_machine_init(hb_mc_manycore_t *mc,
                       hb_mc_manycore_id_t id);

/**
 * Transmit a request packet to manycore hardware
 * @param[in] mc      A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] request A request packet to transmit to manycore hardware
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_machine_transmit(hb_mc_manycore_t *mc,
                           hb_mc_packet_t *packet,
                           long timeout);

/**
 * Receive a packet from manycore hardware
 * @param[in] mc       A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] response A packet into which data should be read
 * @param[in] timeout  A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_machine_receive(hb_mc_manycore_t *mc,
                          hb_mc_packet_t *packet,
                          long timeout);

/**
 * Read the configuration register at an index
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  idx    Configuration register index to access
 * @param[out] config Configuration value at index
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_machine_get_config_at(hb_mc_manycore_t *mc, 
                                unsigned int idx,
                                hb_mc_config_raw_t *config);

/**
 * Read the number of remaining credits of host
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[out] credits The number of remaining credits
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_machine_get_credits(hb_mc_manycore_t *mc, 
                              int *credits, 
                              long timeout);

#endif
