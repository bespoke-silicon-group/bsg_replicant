// Copyright (c) 2019, University of Washington All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list
// of conditions and the following disclaimer.
// 
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
// 
// Neither the name of the copyright holder nor the names of its contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef BSG_MANYCORE_H
#define BSG_MANYCORE_H

#include <bsg_manycore_features.h>
#include <bsg_manycore_config.h>
#include <bsg_manycore_config_pod.h>
#include <bsg_manycore_npa.h>
#include <bsg_manycore_epa.h>
#include <bsg_manycore_packet.h>
#include <bsg_manycore_fifo.h>

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
                const char *name;      //!< the name of this manycore
                hb_mc_config_t config; //!< configuration of the manycore
                void *platform;        //!< machine-specific data pointer
                int dram_enabled;      //!< operating in no-dram mode?
        } hb_mc_manycore_t;

#define HB_MC_MANYCORE_INIT {0}
        /*********************/
        /* Configuration API */
        /*********************/
        static hb_mc_coordinate_t hb_mc_manycore_get_host_coordinate(hb_mc_manycore_t *mc)
        {
                return hb_mc_config_get_host_interface(&mc->config);
        }

        /**
         * Get a pointer to the configuration struct
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @return NULL if uninitialized, else hb_mc_config_t*
         */
        static inline const hb_mc_config_t* hb_mc_manycore_get_config(const hb_mc_manycore_t *mc)
        {
                return &mc->config;
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
        __attribute__((warn_unused_result))
        int hb_mc_manycore_exit(hb_mc_manycore_t *mc);

        ////////////////
        // Packet API //
        ////////////////

        /**
         * Transmit a request packet to manycore hardware
         * @param[in] mc      A manycore instance initialized with hb_mc_manycore_init()
         * @param[in] request A request packet to transmit to manycore hardware
         * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_request_tx(hb_mc_manycore_t *mc,
                                      hb_mc_request_packet_t *request,
                                      long timeout);

        /**
         * Receive a response packet from manycore hardware
         * @param[in] mc       A manycore instance initialized with hb_mc_manycore_init()
         * @param[in] response A packet into which data should be read
         * @param[in] timeout  A timeout counter. Unused - set to -1 to wait forever.
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_response_rx(hb_mc_manycore_t *mc,
                                       hb_mc_response_packet_t *response,
                                       long timeout);

        /**
         * Transmit a response packet to manycore hardware
         * @param[in] mc        A manycore instance initialized with hb_mc_manycore_init()
         * @param[in] response  A response packet to transmit to manycore hardware
         * @param[in] timeout   A timeout counter. Unused - set to -1 to wait forever.
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_response_tx(hb_mc_manycore_t *mc,
                                       hb_mc_response_packet_t *response,
                                       long timeout);

        /**
         * Receive a request packet from manycore hardware
         * @param[in] mc      A manycore instance initialized with hb_mc_manycore_init()
         * @param[in] request A packet into which data should be read
         * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_request_rx(hb_mc_manycore_t *mc,
                                      hb_mc_request_packet_t *request,
                                      long timeout);

        /**
         * Transmit a packet to manycore hardware
         * @param[in] mc      A manycore instance initialized with hb_mc_manycore_init()
         * @param[in] packet  A packet to transmit to manycore hardware
         * @param[in] type    Is this packet a request or response packet?
         * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        __attribute__((warn_unused_result, deprecated))
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
        /**
         * Wait for a finish packet from the Manycore instance. 
         * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init()
         * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
         * @return HB_MC_SUCCESS on packet that writes to HB_MC_HOST_EPA_FINISH, 
         *         HB_MC_FAIL on a packet that writes to HB_MC_HOST_EPA_FAIL or underlying failure, 
         *         HB_MC_INVALID otherwise.
         */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_wait_finish(hb_mc_manycore_t *mc,
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
         * Do a 32-bit amoadd to manycore hardware at a given NPA (must be a DRAM address)
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @param[in]  npa    A valid hb_mc_npa_t aligned to a four byte boundary
         * @param[in]  v      A word value to be added to the NPA
         * @param[out] vpo    Pointer to the previous value at the NPA
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_amoadd32(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, uint32_t vpi, uint32_t *vpo);

        /**
         * Set memory to a given value starting at a given NPA
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @param[in]  npa    A valid hb_mc_npa_t
         * @param[in]  val    Value to be written out
         * @param[in]  sz     The number of bytes to write to manycore hardware
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_memset(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa,
                                  uint8_t data, size_t sz);

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


        /**
         * Read memory from a vector of NPAs
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @param[in]  npa    A vector of valid hb_mc_npa_t of length <= #words
         * @param[out] data   A word vector into which data will be read
         * @param[in]  words  The number of words to read from manycore hardware
         * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
         */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_read_mem_scatter_gather(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa,
                                                   uint32_t *data, size_t words);

        /***********/
        /* DMA API */
        /***********/

        /**
         * Check if DMA writing is supported.
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @return One if DMA writing is supported - Zero otherwise.
         */
        int hb_mc_manycore_supports_dma_write(const hb_mc_manycore_t *mc);

        /**
         * Check if DMA reading is supported.
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @return One if DMA reading is supported - Zero otherwise.
         */
        int hb_mc_manycore_supports_dma_read(const hb_mc_manycore_t *mc);

        /**
         * Write memory via DMA to manycore DRAM starting at a given NPA
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @param[in]  npa    A valid hb_mc_npa_t (must map to DRAM)
         * @param[in]  data   A buffer to be written out manycore hardware
         * @param[in]  sz     The number of bytes to write to manycore hardware
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         *
         * This function is used to write to HammerBlade DRAM directly via DMA.
         * Any data in the cache that becomes stale will be invalidated - this function is 'safe' in that respect.
         *
         * However, invalidating every address might be expensive - and perhaps unnecessary if you 'know'
         * that the memory being written is guaranteed to be un-cached.
         * See hb_mc_manycore_dma_write_no_cache_ainv() for an unsafe version of this function.
         *
         * This function is not supported on all HammerBlade platforms.
         * Please check the return code for HB_MC_NOIMPL.
         */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_dma_write(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa,
                                     const void *data, size_t sz);

        /**
         * Write memory via DMA to manycore DRAM starting at a given NPA - unsafe
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @param[in]  npa    A valid hb_mc_npa_t (must map to DRAM)
         * @param[in]  data   A buffer to be written out manycore hardware
         * @param[in]  sz     The number of bytes to write to manycore hardware
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         *
         * This function is used to write to HammerBlade DRAM directly via DMA.
         * Stale data may remain in the cache - this function is unsafe in that respect.
         * This function is not supported on all HammerBlade platforms.
         * Please check the return code for HB_MC_NOIMPL.
         */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_dma_write_no_cache_ainv(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa,
                                                  const void *data, size_t sz);

        /**
         * Read memory via DMA from manycore DRAM starting at a given NPA
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @param[in]  npa    A valid hb_mc_npa_t (must map to DRAM)
         * @param[in]  data   A buffer into which data will be read
         * @param[in]  sz     The number of bytes to read from manycore hardware
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         *
         * This function is used to read from HammerBlade DRAM directly via DMA.
         * Any cached data for this memory range will be flushed - this function is 'safe' in that respect.
         *
         * However, sending a flush packet for every address in this range might be expensive -
         * and perhaps unnecessary if you 'know'  the data is uncached.
         * See hb_mc_manycore_dma_read_no_cache_afl() for an unsafe alternative to this function.
         *
         * This function is not supported on all HammerBlade platforms.
         * Please check the return code for HB_MC_NOIMPL.
         */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_dma_read(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa,
                                    void *data, size_t sz);

        /**
         * Read memory via DMA from manycore DRAM starting at a given NPA - unsafe
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @param[in]  npa    A valid hb_mc_npa_t (must map to DRAM)
         * @param[in]  data   A buffer into which data will be read
         * @param[in]  sz     The number of bytes to read from manycore hardware
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         *
         * This function is used to read from HammerBlade DRAM directly via DMA.
         * Cached data for this memory range migth not be flushed - this function is 'unsafe' in that respect.
         *
         * This function is not supported on all HammerBlade platforms.
         * Please check the return code for HB_MC_NOIMPL.
         */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_dma_read_no_cache_afl(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa,
                                                 void *data, size_t sz);

        /************************/
        /* Cache Operations API */
        /************************/
        /**
         * Check if DMA reading is supported.
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @return One if there's cache, zero otherwise.
         */
        static int hb_mc_manycore_has_cache(hb_mc_manycore_t *mc)
        {
                return hb_mc_config_memsys_feature_cache(hb_mc_manycore_get_config(mc)) == 1;
        }


        /**
         * Invalidate a range of manycore DRAM addresses.
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @param[in]  npa    A valid hb_mc_npa_t (must map to DRAM) - start of the range to invalidate
         * @param[in]  sz     The size of the range to invalidate in bytes
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_vcache_invalidate_npa_range(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, size_t sz);

        /**
         * Flush a range of manycore DRAM addresses.
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @param[in]  npa    A valid hb_mc_npa_t (must map to DRAM) - start of the range to flush
         * @param[in]  sz     The size of the range to flush in bytes
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_vcache_flush_npa_range(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, size_t sz);

        /**
         * Flush a cache tag.
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @param[in]  npa    A valid hb_mc_npa_t (must map to a cache tag) - start of the range to flush
         * @param[in]  sz     The size of the range to flush in bytes
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_vcache_flush_tag(hb_mc_manycore_t *mc, const hb_mc_npa_t *way_npa);

        /**
         * Invalidate entire victim cache.
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_invalidate_vcache(hb_mc_manycore_t *mc);

       /**
        * Mark each way in victim cache as valid.
        * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
        */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_validate_vcache(hb_mc_manycore_t *mc);

        /**
         * Flush entire victim cache.
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_flush_vcache(hb_mc_manycore_t *mc);

        /**
         * Invalidate entire victim cache for pod.
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_pod_invalidate_vcache(hb_mc_manycore_t *mc, hb_mc_coordinate_t pod);

       /**
        * Mark each way in victim cache as valid for pod.
        * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
        */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_pod_validate_vcache(hb_mc_manycore_t *mc, hb_mc_coordinate_t pod);

        /**
         * Flush entire victim cache for pod.
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         */
        __attribute__((warn_unused_result))
        int hb_mc_manycore_pod_flush_vcache(hb_mc_manycore_t *mc, hb_mc_coordinate_t pod);

        /**
         * Query if we are operating in no DRAM mode.
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @return One if DRAM is enabled. Zero otherwise.
         */
        static inline int hb_mc_manycore_dram_is_enabled(const hb_mc_manycore_t *mc)
        {
                return mc->dram_enabled;
        }

        /**
         * Enable DRAM mode on the manycore instance.
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @return One if DRAM is enabled. Zero otherwise.
         */
        int hb_mc_manycore_enable_dram(hb_mc_manycore_t *mc);

        /**
         * Disable DRAM mode on the manycore instance.
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @return One if DRAM is enabled. Zero otherwise.
         */
        int hb_mc_manycore_disable_dram(hb_mc_manycore_t *mc);

        /**
         * Get the max size of program text.
         */
        static inline size_t hb_mc_manycore_get_max_program_text_size(const hb_mc_manycore_t* mc,
                                                                      const hb_mc_coordinate_t *tile)
        {
                return 16ul * (1ul<<20ul); // 16M -- this might be later from the config
        }

        /**
         * Get the size of DRAM.
         * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @param[in]  tile   The coordinate tile to query for its instruction cache size.
         * @return the size of the tiles instruction cache.
         */
        static inline size_t hb_mc_manycore_get_size_dram(const hb_mc_manycore_t *mc)
        {
                return hb_mc_config_get_dram_size(hb_mc_manycore_get_config(mc));
        }

        /**
         * Stall until the all requests (and responses to the host) have reached their destination.
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_manycore_host_request_fence(hb_mc_manycore_t *mc, long timeout);

        /**
         * Get the current cycle counter value of the Manycore Platform
         *
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @param[out] time   The current counter value.
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_manycore_get_cycle(hb_mc_manycore_t *mc, uint64_t *time);

        typedef enum {
                e_instr_float = 0, //<! Floating Point Instructions
                e_instr_int = 1, //<! Integer Instructions
                e_instr_all = 2 //<! All instructions (including branches, jumps, and control flow)
        } bsg_instr_type_e;

        /**
         * Get the number of instructions executed for a certain class of instructions
         * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
         * @param[in] itype An enum defining the class of instructions to query.
         * @param[out] count The number of instructions executed in the queried class.
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_manycore_get_icount(hb_mc_manycore_t *mc, bsg_instr_type_e itype, int *count);

        /**
         * Enable trace file generation (vanilla_operation_trace.csv)
         * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_manycore_trace_enable(hb_mc_manycore_t *mc);

        /**
         * Disable trace file generation (vanilla_operation_trace.csv)
         * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_manycore_trace_disable(hb_mc_manycore_t *mc);

        /**
         * Enable log file generation (vanilla.log)
         * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_manycore_log_enable(hb_mc_manycore_t *mc);

        /**
         * Disable log file generation (vanilla.log)
         * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
         * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
         */
        int hb_mc_manycore_log_disable(hb_mc_manycore_t *mc);

        /**
         * Convenience macro for calling a manycore function and handling an error return code.
         * @param[in] stmt  A C/C++ statement that evaluates to an integer return code.
         *
         * Example:
         * BSG_MANYCORE_CALL(hb_mc_manycore_init(&mc, ...));
         *
         * The return code must be an integer defined in bsg_manycore_errno.h - otherwise behavior is undefined.
         * This macro will cause the invoking to return if an error code is returned.
         * An error message will be printing with the code statement that failed.
         */
#define BSG_MANYCORE_CALL(mc, stmt)                                     \
        {                                                               \
                int __r = stmt;                                         \
                if (__r != HB_MC_SUCCESS) {                             \
                        bsg_pr_err("Instance %s: Call to '%s' failed -- %s\n", \
                                   mc->name, #stmt, hb_mc_strerror(__r)); \
                        return __r;                                     \
                }                                                       \
        }

#ifdef __cplusplus
}
#endif
#endif
