#ifndef __BSG_MANYCORE_PLATFORM_HPP
#define __BSG_MANYCORE_PLATFORM_HPP
#include <bsg_manycore.h>

/**
 * This file defines the interface that runtime platforms provide to
 * BSG Manycore and CUDA Lite Runtime libraries. This interface
 * provides methods for initialization and cleanup, transmit and
 * receive, fence, and configuration.
 *
 * To support a new platform, define these functions in a new
 * bsg_manycore_platform.cpp file.
 *
 * Editing this file should be VERY rare. Do not add methods to this
 * interface without SERIOUSLY considering the implications.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

        /**
         * Clean up the runtime platform
         * @param[in] mc    A manycore to clean up
         */
        void hb_mc_platform_cleanup(hb_mc_manycore_t *mc);

        /**
         * Initialize the runtime platform
         * @param[in] mc    A manycore to initialize
         * @param[in] id    ID which selects the physical hardware from which this manycore is configured
         * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
         */
        int hb_mc_platform_init(hb_mc_manycore_t *mc,
                                hb_mc_manycore_id_t id);

        /**
         * Transmit a request packet to manycore hardware
         * @param[in] mc      A manycore instance initialized with hb_mc_manycore_init()
         * @param[in] request A request packet to transmit to manycore hardware
         * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_platform_transmit(hb_mc_manycore_t *mc,
                                    hb_mc_packet_t *packet,
                                    hb_mc_fifo_tx_t type,
                                    long timeout);

        /**
         * Receive a packet from manycore hardware
         * @param[in] mc       A manycore instance initialized with hb_mc_manycore_init()
         * @param[in] response A packet into which data should be read
         * @param[in] timeout  A timeout counter. Unused - set to -1 to wait forever.
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_platform_receive(hb_mc_manycore_t *mc,
                                   hb_mc_packet_t *packet,
                                   hb_mc_fifo_rx_t type,
                                   long timeout);

        /**
         * Read the configuration register at an index
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @param[in]  idx    Configuration register index to access
         * @param[out] config Configuration value at index
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_platform_get_config_at(hb_mc_manycore_t *mc,
                                         unsigned int idx,
                                         hb_mc_config_raw_t *config);

        /**
         * Stall until the all requests (and responses) have reached their destination.
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_platform_fence(hb_mc_manycore_t *mc, long timeout);

        /**
         * Signal the hardware to start a bulk transfer over the network
         *
         * Hooks that the platform may implement to perform special
         * initialization and cleanup tasks for assisting in doing
         * packet-based bulk transfers from the host and device.
         *
         * Unless otherwise needed, most platforms can simply return HB_MC_SUCCESS
         *
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_platform_start_bulk_transfer(hb_mc_manycore_t *mc);

        /**
         * Signal the hardware to end a bulk transfer over the network
         *
         * Hooks that the platform may implement to perform special
         * initialization and cleanup tasks for assisting in doing
         * packet-based bulk transfers from the host and device.
         *
         * Unless otherwise needed, most platforms can simply return HB_MC_SUCCESS
         *
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_platform_finish_bulk_transfer(hb_mc_manycore_t *mc);

        /**
         * Get the current cycle counter of the Manycore Platform
         *
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @param[out] time   The current counter value.
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_platform_get_cycle(hb_mc_manycore_t *mc, uint64_t *time);

        /**
         * Get the number of instructions executed for a certain class of instructions
         * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
         * @param[in] itype An enum defining the class of instructions to query.
         * @param[out] count The number of instructions executed in the queried class.
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_platform_get_icount(hb_mc_manycore_t *mc, bsg_instr_type_e itype, int *count);

        /**
         * Enable trace file generation (vanilla_operation_trace.csv)
         * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_platform_trace_enable(hb_mc_manycore_t *mc);

        /**
         * Disable trace file generation (vanilla_operation_trace.csv)
         * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_platform_trace_disable(hb_mc_manycore_t *mc);

        /**
         * Enable log file generation (vanilla.log)
         * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_platform_log_enable(hb_mc_manycore_t *mc);

        /**
         * Disable log file generation (vanilla.log)
         * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_platform_log_disable(hb_mc_manycore_t *mc);
#ifdef __cplusplus
}
#endif
#endif
