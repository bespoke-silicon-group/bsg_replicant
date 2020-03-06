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

#ifndef BSG_MANYCORE_TILE_H
#define BSG_MANYCORE_TILE_H

#include <bsg_manycore_features.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore.h>
#include <bsg_manycore_config.h>
#include <bsg_manycore_eva.h>

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

        typedef enum __hb_mc_csr_freeze_t{
                HB_MC_CSR_FREEZE = 1,
                HB_MC_CSR_UNFREEZE = 0
        } hb_mc_csr_freeze_t;

        /*************/
        /* Tile EPAs */
        /*************/

        /* Offsets in bytes */
#define HB_MC_TILE_EPA_DMEM_BASE   0x00001000
#define HB_MC_TILE_EVA_DMEM_BASE   0x00001000
#define HB_MC_TILE_EPA_ICACHE_BASE 0x01000000
#define HB_MC_TILE_EPA_CSR_BASE                       0x20000
#define HB_MC_TILE_EPA_CSR_FREEZE_OFFSET              0x00
#define HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_X_OFFSET 0x04
#define HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_Y_OFFSET 0x08
#define HB_MC_TILE_EPA_CSR_PC_INIT_VALUE_OFFSET       0x0C
#define HB_MC_TILE_EPA_CSR_DRAM_ENABLE_OFFSET         0x10

#define EPA_TILE_CSR_FROM_BYTE_OFFSET(offset)                           \
        EPA_FROM_BASE_AND_OFFSET(HB_MC_TILE_EPA_CSR_BASE, offset)

        /* EPAs */
#define HB_MC_TILE_EPA_ICACHE                                   \
        EPA_FROM_BASE_AND_OFFSET(HB_MC_TILE_EPA_ICACHE_BASE, 0)

#define HB_MC_TILE_EPA_CSR_FREEZE                                       \
        EPA_TILE_CSR_FROM_BYTE_OFFSET(HB_MC_TILE_EPA_CSR_FREEZE_OFFSET)

#define HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_X                          \
        EPA_TILE_CSR_FROM_BYTE_OFFSET(HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_X_OFFSET)

#define HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_Y                          \
                EPA_TILE_CSR_FROM_BYTE_OFFSET(HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_Y_OFFSET)

#define HB_MC_TILE_EPA_CSR_PC_INIT_VALUE                                \
        EPA_TILE_CSR_FROM_BYTE_OFFSET(HB_MC_TILE_EPA_CSR_PC_INIT_VALUE_OFFSET)

#define HB_MC_TILE_EPA_CSR_DRAM_ENABLE                                  \
        EPA_TILE_CSR_FROM_BYTE_OFFSET(HB_MC_TILE_EPA_CSR_DRAM_ENABLE_OFFSET)

        /**
         * Set a tile's x origin
         * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
         * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
         * @param[in] tile   A tile to set the origin of.
         * @param[in] x      The X coordinate of the origin tile
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
                int hb_mc_tile_set_origin_x(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile, 
                                            const hb_mc_idx_t x);
        /**
         * Set a tile's y origin
         * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
         * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
         * @param[in] tile   A tile to set the origin of.
         * @param[in] y      The Y coordinate of the origin tile
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_tile_set_origin_y(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile, 
                                    const hb_mc_idx_t y);
        /**
         * Set a tile's origin
         * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
         * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
         * @param[in] tile   A tile to set the origin of.
         * @param[in] o      The origin tile
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_tile_set_origin(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile, 
                                  const hb_mc_coordinate_t *o);

        /**
         * Freeze a tile.
         * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
         * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
         * @param[in] tile   A tile to freeze.
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_tile_freeze(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile);

        /**
         * Unfreeze a tile.
         * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
         * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
         * @param[in] tile   A tile to unfreeze.
         * @return HB_MC_SUCCESS if succesful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_tile_unfreeze(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile);

        /**
         * Set the DRAM enabled bit for a tile.
         * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
         * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
         * @param[in] tile   A tile to unfreeze.
         * @return HB_MC_SUCCESS if successful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_tile_set_dram_enabled(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile);

        /**
         * Clear the DRAM enabled bit for a tile.
         * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
         * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init().
         * @param[in] tile   A tile to unfreeze.
         * @return HB_MC_SUCCESS if successful. Otherwise an error code is returned.
         */
        __attribute__((warn_unused_result))
        int hb_mc_tile_clear_dram_enabled(hb_mc_manycore_t *mc, const hb_mc_coordinate_t *tile);

        /****************************************************************************************/
        /* TODO: these should actually check if there's a vanilla core at the given tile.       */
        /* At the moment that would mean checking the coordinates according to a static config. */
        /* In the future some sort of ID register would be a nice feature.                      */
        /****************************************************************************************/

        /**
         * Get the size of a tiles local data memory.
         * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @param[in]  tile   The coordinate tile to query for its data memory size.
         * @return the size of the tiles data memory.
         */
        static inline size_t hb_mc_tile_get_size_dmem(const hb_mc_manycore_t *mc,
                                                      const hb_mc_coordinate_t *tile)
        {
                return hb_mc_config_get_dmem_size(hb_mc_manycore_get_config(mc));
        }

        /**
         * Get the size of a tile's local icache.
         * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @param[in]  tile   The coordinate tile to query for its instruction cache size.
         * @return the size of the tiles instruction cache.
         */
        static inline size_t hb_mc_tile_get_size_icache(const hb_mc_manycore_t *mc,
                                                        const hb_mc_coordinate_t *tile)
        {
                return hb_mc_config_get_icache_size(hb_mc_manycore_get_config(mc));
        }




        /*!
         * Sets a Vanilla Core Endpoint's tile group's origin hardware registers CSR_TGO_X/Y.
         * Behavior is undefined if #mc is not initialized with hb_mc_manycore_init().
         * @param[in] mc         A manycore instance initialized with hb_mc_manycore_init().
         * @param[in] coord      Tile coordinates to set the origin of.
         * @param[in] origin     Origin coordinates.
         * @return HB_MC_SUCCESS on success and HB_MC_FAIL on failure.
         */
        __attribute__((warn_unused_result))
        int hb_mc_tile_set_origin_registers (hb_mc_manycore_t *mc,
                                             const hb_mc_coordinate_t *coord,
                                             const hb_mc_coordinate_t *origin); 








        /** Deprecated **/
#define hb_mc_tile_epa_get_byte_addr(base, offset) (base + offset)

#define hb_mc_tile_epa_get_word_addr(base, offset)              \
        (hb_mc_tile_epa_get_byte_addr(base, offset) >> 2)



#ifdef __cplusplus
}
#endif

#endif
