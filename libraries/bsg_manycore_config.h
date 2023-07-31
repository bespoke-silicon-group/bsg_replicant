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

#ifndef BSG_MANYCORE_CONFIG_H
#define BSG_MANYCORE_CONFIG_H

/* MAX AND DUSTIN'S RULE OF THUMB FOR WHAT GOES IN CONFIG
 *
 * An entity requires an accessor in config if:
 * 1. It is in the ROM (or other configuration register)
 * 2. It is trivially derivable from data in the ROM
 * 3. It is a top-level parameter that SHOULD BE in the ROM.
 */

#include <bsg_manycore_features.h>
#include <bsg_manycore_coordinate.h>
#include <bsg_manycore_rom.h>
#include <bsg_manycore_memsys.h>

#ifdef __cplusplus
#include <cstdint>
#include <cmath>
#else
#include <stdint.h>
#include <math.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

        // We expect a 32-bit bus for Addresses, but two bits are removed because the
        // network uses word-level addressing and handles byte-writes with a mask
        #define HB_MC_CONFIG_MAX_BITWIDTH_ADDR 30
        #define HB_MC_CONFIG_MAX_BITWIDTH_DATA 32

        // normal limit for the flow-control parameters
        #define HB_MC_REMOTE_LOAD_MIN 1
        #define HB_MC_REMOTE_LOAD_MAX 32

        #define HB_MC_HOST_CREDITS_MIN 1
        #define HB_MC_HOST_CREDITS_MAX 32

        typedef hb_mc_rom_word_t hb_mc_config_raw_t;

        /* Compilation Metadata */
        typedef struct __hb_mc_version_t {
                uint8_t major;
                uint8_t minor;
                uint8_t revision;
        } hb_mc_version_t;

        typedef struct __hb_mc_date_t {
                uint8_t month;
                uint8_t day;
                uint16_t year;
        } hb_mc_date_t;

        typedef uint32_t hb_mc_githash_t;

        typedef struct __hb_mc_config_t{
                hb_mc_version_t design_version;
                hb_mc_date_t timestamp;
                uint32_t network_bitwidth_addr;
                uint32_t network_bitwidth_data;
                hb_mc_dimension_t pods;      // how many vcore pods?
                hb_mc_dimension_t pod_shape; // what is the shape of a pod?
                hb_mc_dimension_t noc_coord_width;
                hb_mc_dimension_t noc_ruche_factor;
                hb_mc_dimension_t bar_ruche_factor;
                hb_mc_dimension_t wh_ruche_factor;
                hb_mc_dimension_t pod_coord_width;
                hb_mc_dimension_t tile_coord_width;
                hb_mc_coordinate_t host_interface;
                hb_mc_coordinate_t origin;
                hb_mc_githash_t basejump;
                hb_mc_githash_t manycore;
                hb_mc_githash_t f1;
                uint32_t vcache_ways;
                uint32_t vcache_sets;
                uint32_t vcache_block_words;
                uint32_t vcache_stripe_words;
                uint32_t io_remote_load_cap;
                uint32_t io_host_credits_cap;
                uint32_t io_endpoint_max_out_credits;
                uint32_t chip_id;
                hb_mc_memsys_t memsys;
        } hb_mc_config_t;

        typedef enum __hb_mc_config_id_t {
                HB_MC_CONFIG_MIN = 0,
                HB_MC_CONFIG_VERSION = 0,
                HB_MC_CONFIG_TIMESTAMP = 1,
                HB_MC_CONFIG_NETWORK_ADDR_WIDTH = 2,
                HB_MC_CONFIG_NETWORK_DATA_WIDTH = 3,
                HB_MC_CONFIG_DIM_PODS_X = 4,
                HB_MC_CONFIG_DIM_PODS_Y = 5,
                HB_MC_CONFIG_POD_DIM_X = 6,
                HB_MC_CONFIG_POD_DIM_Y = 7,
                HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_X = 8,
                HB_MC_CONFIG_DEVICE_HOST_INTF_COORD_Y = 9,
                HB_MC_CONFIG_ORIGIN_COORD_X = 10,
                HB_MC_CONFIG_ORIGIN_COORD_Y = 11,
                HB_MC_CONFIG_NOC_COORD_X_WIDTH = 12,
                HB_MC_CONFIG_NOC_COORD_Y_WIDTH = 13,
                HB_MC_CONFIG_NOC_RUCHE_FACTOR_X = 14,
                HB_MC_CONFIG_BARRIER_RUCHE_FACTOR_X = 15,
                HB_MC_CONFIG_WH_RUCHE_FACTOR_X = 16,
                HB_MC_CONFIG_REPO_BASEJUMP_HASH = 17,
                HB_MC_CONFIG_REPO_MANYCORE_HASH = 18,
                HB_MC_CONFIG_REPO_F1_HASH = 19,
                HB_MC_CONFIG_VCACHE_WAYS = 20,
                HB_MC_CONFIG_VCACHE_SETS = 21,
                HB_MC_CONFIG_VCACHE_BLOCK_WORDS = 22,
                HB_MC_CONFIG_VCACHE_STRIPE_WORDS = 23,
                HB_MC_CONFIG_VCACHE_MISS_FIFO_ELS = 24,
                HB_MC_CONFIG_IO_REMOTE_LOAD_CAP = 25,
                HB_MC_CONFIG_IO_HOST_CREDITS_CAP = 26,
                HB_MC_CONFIG_IO_EP_MAX_OUT_CREDITS = 27,
                HB_MC_CONFIG_CHIP_ID = 28,
                HB_MC_CONFIG_MEMSYS = 29,
                HB_MC_CONFIG_MAX=HB_MC_CONFIG_MEMSYS + HB_MC_MEMSYS_ROM_IDX_MAX,
        } hb_mc_config_id_t;

        int hb_mc_config_init(const hb_mc_config_raw_t mc[HB_MC_CONFIG_MAX], hb_mc_config_t *config);

        static inline uint64_t hb_mc_config_id_to_addr(uint64_t addr, hb_mc_config_id_t id)
        {
                return (addr + (id << 2));
        }

        const char *hb_mc_config_id_to_string(hb_mc_config_id_t id);

        static inline uint8_t hb_mc_config_get_version_major(const hb_mc_config_t *cfg){
                return cfg->design_version.major;
        }

        static inline uint8_t hb_mc_config_get_version_minor(const hb_mc_config_t *cfg){
                return cfg->design_version.minor;
        }

        static inline uint8_t hb_mc_config_get_version_revision(const hb_mc_config_t *cfg){
                return cfg->design_version.revision;
        }

        static inline uint8_t hb_mc_config_get_compilation_month(const hb_mc_config_t *cfg){
                return cfg->timestamp.month;
        }

        static inline uint8_t hb_mc_config_get_compilation_day(const hb_mc_config_t *cfg){
                return cfg->timestamp.day;
        }

        static inline uint16_t hb_mc_config_get_compilation_year(const hb_mc_config_t *cfg){
                return cfg->timestamp.year;
        }

        static inline hb_mc_githash_t hb_mc_config_get_githash_basejump(const hb_mc_config_t *cfg){
                return cfg->basejump;
        }

        static inline hb_mc_githash_t hb_mc_config_get_githash_manycore(const hb_mc_config_t *cfg){
                return cfg->manycore;
        }

        static inline hb_mc_githash_t hb_mc_config_get_githash_f1(const hb_mc_config_t *cfg){
                return cfg->f1;
        }

        /* Device Metadata */
        static inline uint32_t hb_mc_config_get_network_bitwidth_addr(const hb_mc_config_t *cfg){
                return cfg->network_bitwidth_addr;
        }

        static inline uint32_t hb_mc_config_get_network_bitwidth_data(const hb_mc_config_t *cfg){
                return cfg->network_bitwidth_data;
        }

        static inline hb_mc_coordinate_t hb_mc_config_get_host_interface(const hb_mc_config_t *cfg){
                return cfg->host_interface;
        }

        static inline hb_mc_coordinate_t hb_mc_config_get_origin(const hb_mc_config_t *cfg){
                return cfg->origin;
        }

        static inline hb_mc_dimension_t hb_mc_config_get_dimension_vcore(const hb_mc_config_t *cfg){
                return cfg->pod_shape;
        }

        static inline hb_mc_idx_t hb_mc_config_get_vcore_base_y(const hb_mc_config_t *cfg){
                return cfg->origin.y;
        }

        static inline hb_mc_idx_t hb_mc_config_get_vcore_base_x(const hb_mc_config_t *cfg){
                return cfg->origin.x;
        }

        static inline hb_mc_coordinate_t hb_mc_config_get_origin_vcore(const hb_mc_config_t *cfg){
                return hb_mc_coordinate(hb_mc_config_get_vcore_base_x(cfg),
                                        hb_mc_config_get_vcore_base_y(cfg));
        }

        static inline hb_mc_coordinate_t hb_mc_config_get_origin_network(const hb_mc_config_t *cfg){
                return hb_mc_coordinate(0, 0);
        }

        static inline hb_mc_dimension_t hb_mc_config_noc_coord_width(const hb_mc_config_t *cfg){
                return cfg->noc_coord_width;
        }

        static inline hb_mc_dimension_t hb_mc_config_pod_coord_width(const hb_mc_config_t *cfg){
                return cfg->pod_coord_width;
        }

        static inline hb_mc_dimension_t hb_mc_config_tile_coord_width(const hb_mc_config_t *cfg){
                return cfg->tile_coord_width;
        }

        static inline hb_mc_dimension_t hb_mc_config_get_dimension_network(const hb_mc_config_t *cfg){
                hb_mc_dimension_t noc_width = hb_mc_config_noc_coord_width(cfg);
                return hb_mc_dimension(1<<hb_mc_dimension_get_x(noc_width),
                                       1<<hb_mc_dimension_get_y(noc_width));
        }

        static inline uint8_t hb_mc_config_get_vcache_bitwidth_tag_addr(const hb_mc_config_t *cfg)
        {
                // At the moment, the high-order bit in the network is used to address
                // tags in the Victim Cache. All other bits are used for data
                return 1;
        }

        static inline uint32_t hb_mc_config_get_vcache_bitwidth_data_addr(const hb_mc_config_t *cfg)
        {
                // All network address bits are routed to the victim caches. Some
                // high-order bits of the network address are reserved for addressing
                // tags in the victim cache and are not accessible for data accesses, so
                // we subtract them. The network address is also a word address, so we
                // add two bits to make it a byte address.
                return hb_mc_config_get_network_bitwidth_addr(cfg) -
                        hb_mc_config_get_vcache_bitwidth_tag_addr(cfg) +
                        log2(sizeof(uint32_t));
        }

        static inline uint8_t hb_mc_config_get_dmem_bitwidth_addr(const hb_mc_config_t *cfg)
        {
                return 12; // 12 bits, this might be read from ROM
        }

        static inline size_t hb_mc_config_get_dmem_size(const hb_mc_config_t *cfg)
        {
                // 4K: this might be read from ROM if the value ever changes
                return (1 << hb_mc_config_get_dmem_bitwidth_addr(cfg));
        }

        static inline uint8_t hb_mc_config_get_icache_bitwidth_addr(const hb_mc_config_t *cfg)
        {
                return 12; // 12 bits, this might be read from ROM
        }

        static inline size_t hb_mc_config_get_icache_size(const hb_mc_config_t *cfg)
        {
                // 4K: this might be read from ROM if the value ever changes
                return (1 << hb_mc_config_get_icache_bitwidth_addr(cfg));
        }

        static inline
        hb_mc_idx_t hb_mc_config_get_num_dram_coordinates(const hb_mc_config_t *cfg)
        {
                // each pod has banks at the north and south
                return cfg->pods.x * cfg->pods.y * cfg->pod_shape.x * 2;
        }

#define hb_mc_config_foreach_dram_id(dram_id_var, config)               \
        for (dram_id_var = 0;                                           \
             dram_id_var < hb_mc_config_get_num_dram_coordinates(config); \
             dram_id_var++)


        static inline size_t hb_mc_config_get_dram_bank_size(const hb_mc_config_t *cfg)
        {
                return cfg->memsys.dram_bank_size;
        }

        /* Returns the size of DRAM accessible to each manycore tile */
        static inline size_t hb_mc_config_get_dram_size(const hb_mc_config_t *cfg)
        {
                return hb_mc_config_get_dram_bank_size(cfg)
                    *  hb_mc_config_get_num_dram_coordinates(cfg);
        }

        /**
         * Return the associativity of the victim cache from the configuration.
         * @param[in] cfg A configuration initialized from the manycore ROM.
         * @return the number of ways per set in the victim cache.
         */
        static inline uint32_t hb_mc_config_get_vcache_ways(const hb_mc_config_t *cfg)
        {
                return cfg->vcache_ways;
        }

        /**
         * Return the number of sets in the victim cache from the configuration.
         * @param[in] cfg A configuration initialized from the manycore ROM.
         * @return the number of sets in the manycore victim cache.
         */
        static inline uint32_t hb_mc_config_get_vcache_sets(const hb_mc_config_t *cfg)
        {
                return cfg->vcache_sets;
        }

        /**
         * Return the number of words in a victim cache block.
         * @param[in] cfg A configuration initialized from the manycore ROM.
         * @return the number of words in a victim cache block.
         */
        static inline uint32_t hb_mc_config_get_vcache_block_words(const hb_mc_config_t *cfg)
        {
                return cfg->vcache_block_words;
        }

        /**
         * Return the number of bytes in a victim cache block.
         * @param[in] cfg A configuration initialized from the manycore ROM.
         * @return the number of words in a victim cache block.
         */
        static inline uint32_t hb_mc_config_get_vcache_block_size(const hb_mc_config_t *cfg)
        {
                return cfg->vcache_block_words * sizeof(uint32_t); // words are four bytes
        }

        /**
         * Return the victim cache stripe size in words.
         * @param[in] cfg A configuration initialized from the manycore ROM.
         * @return the victim cache stripe size in words.
         */
        static inline uint32_t hb_mc_config_get_vcache_stripe_words(const hb_mc_config_t *cfg)
        {
                return cfg->vcache_stripe_words;
        }

        /**
         * Return the victim cache stripe size in bytes.
         * @param[in] cfg A configuration initialized from the manycore ROM.
         * @return the victim cache stripe size in bytes.
         */
        static inline uint32_t hb_mc_config_get_vcache_stripe_size(const hb_mc_config_t *cfg)
        {
                return cfg->vcache_stripe_words * sizeof(uint32_t);
        }

        /**
         * Return the number of bytes in a victim cache block.
         * @param[in] cfg A configuration initialized from the manycore ROM.
         * @return the number of words in a victim cache block.
         */
        static inline uint32_t hb_mc_config_get_vcache_size(const hb_mc_config_t *cfg)
        {
                return hb_mc_config_get_vcache_block_size(cfg) *
                        hb_mc_config_get_vcache_sets(cfg) *
                        hb_mc_config_get_vcache_ways(cfg);
        }

        /**
         * Return the host capacity for batching remote loads.
         * @param[in] cfg A configuration initialized from the manycore ROM.
         * @return the host capacity for batching remote loads.
         */
        static inline uint32_t hb_mc_config_get_io_remote_load_cap(const hb_mc_config_t *cfg)
        {
                return cfg->io_remote_load_cap;
        }

        /**
         * Return the host capacity for batching requests.
         * @param[in] cfg A configuration initialized from the manycore ROM.
         * @return the host capacity for batching requests.
         */
        static inline uint32_t hb_mc_config_get_transmit_vacancy_max(const hb_mc_config_t *cfg)
        {
                return cfg->io_host_credits_cap;
        }

        /**
         * Return the max out credits of the manycore endpoint standard module in the IO interface.
         * @param[in] cfg A configuration initialized from the manycore ROM.
         * @return the max out credits of the manycore endpoint standard module in the IO interface.
         */
        static inline uint32_t hb_mc_config_get_io_endpoint_max_out_credits(const hb_mc_config_t *cfg)
        {
                return cfg->io_endpoint_max_out_credits;
	}

        /**
         * Return the number of DRAM channels in the system.
         * @param[in] cfg A configuration initialized from the manycore ROM.
         * @return the number of DRAM channels in the system.
         */
        static inline uint32_t hb_mc_config_get_dram_channels(const hb_mc_config_t *cfg)
        {
                return cfg->memsys.dram_channels;
        }

        /**
         * Return a the configuration value for the DMA feature
         * @param[in] cfg A configuration initialized from the manycore ROM.
         * @return 1 if DMA is a supported memory system feature, 0 otherwise.
         */
        static inline uint32_t hb_mc_config_memsys_feature_dma(const hb_mc_config_t *cfg)
        {
                return cfg->memsys.feature_dma;
        }

        /**
         * Return a the configuration value for the cache feature of the memory system.
         * @param[in] cfg A configuration initialized from the manycore ROM.
         * @return 1 if cache is a supported memory system feature, 0 otherwise.
         */
        static inline uint32_t hb_mc_config_memsys_feature_cache(const hb_mc_config_t *cfg)
        {
                return cfg->memsys.feature_cache;
        }

        /**
         * Returns an identifier for the memory system type
         * @param[in] cfg A configuration initialized from the manycore ROM.
         * @return An enum value from hb_mc_memsys_id_t defining the memory type
         */
        static inline hb_mc_memsys_id_t hb_mc_config_memsys_id(const hb_mc_config_t *cfg)
        {
                return cfg->memsys.id;
        }


#ifdef __cplusplus
}
#endif
#endif
