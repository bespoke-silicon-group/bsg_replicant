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

#include <bsg_manycore_eva.h>
#include <bsg_manycore_tile.h>
#include <bsg_manycore_vcache.h>
#include <bsg_manycore_printing.h>


#ifdef __cplusplus
#include <cmath>
#else
#include <math.h>
#endif

#define MAKE_MASK(WIDTH) ((1ULL << (WIDTH)) - 1ULL)

#define DEFAULT_GROUP_X_LOGSZ 6
#define DEFAULT_GROUP_X_BITIDX HB_MC_EPA_LOGSZ
#define DEFAULT_GROUP_X_BITMASK (MAKE_MASK(DEFAULT_GROUP_X_LOGSZ) << DEFAULT_GROUP_X_BITIDX)

#define DEFAULT_GROUP_Y_LOGSZ 5
#define DEFAULT_GROUP_Y_BITIDX (DEFAULT_GROUP_X_BITIDX + DEFAULT_GROUP_X_LOGSZ)
#define DEFAULT_GROUP_Y_BITMASK (MAKE_MASK(DEFAULT_GROUP_Y_LOGSZ) << DEFAULT_GROUP_Y_BITIDX)

#define DEFAULT_GROUP_BITIDX (DEFAULT_GROUP_Y_BITIDX + DEFAULT_GROUP_Y_LOGSZ)
#define DEFAULT_GROUP_BITMASK (1ULL << DEFAULT_GROUP_BITIDX)

#define DEFAULT_GLOBAL_X_LOGSZ 6
#define DEFAULT_GLOBAL_X_BITIDX HB_MC_EPA_LOGSZ
#define DEFAULT_GLOBAL_X_BITMASK (MAKE_MASK(DEFAULT_GLOBAL_X_LOGSZ) << DEFAULT_GLOBAL_X_BITIDX)

#define DEFAULT_GLOBAL_Y_LOGSZ 6
#define DEFAULT_GLOBAL_Y_BITIDX (DEFAULT_GLOBAL_X_BITIDX + DEFAULT_GLOBAL_X_LOGSZ)
#define DEFAULT_GLOBAL_Y_BITMASK (MAKE_MASK(DEFAULT_GLOBAL_Y_LOGSZ) << DEFAULT_GLOBAL_Y_BITIDX)

#define DEFAULT_GLOBAL_BITIDX (DEFAULT_GLOBAL_Y_BITIDX + DEFAULT_GLOBAL_Y_LOGSZ)
#define DEFAULT_GLOBAL_BITMASK (1ULL << DEFAULT_GLOBAL_BITIDX)

#define DEFAULT_DRAM_BITIDX (DEFAULT_GLOBAL_BITIDX + 1)
#define DEFAULT_DRAM_BITMASK (1ULL << DEFAULT_DRAM_BITIDX)

/**
 * Determines if an EVA is a tile-local EVA
 * @return true if EVA addresses tile-local memory, false otherwise
 */
static bool default_eva_is_local(const hb_mc_eva_t *eva)
{
        /* A LOCAL EVA is indicated by all non-EPA high-order bits set to 0 */
        return !(hb_mc_eva_addr(eva) & ~(MAKE_MASK(HB_MC_EPA_LOGSZ)));
}

/**
 * Returns the EPA and number of contiguous bytes for an EVA in a tile,
 * regardless of the continuity of the underlying NPA.
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  eva    An Endpoint Virtual Address
 * @param[out] epa    An Endpoint Physical Address to be set by translating #eva
 * @param[out] sz     Number of contiguous bytes remaining in the #eva segment
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int default_eva_to_epa_tile(
                                   const hb_mc_config_t *cfg,
                                   const hb_mc_eva_t *eva,
                                   hb_mc_epa_t *epa,
                                   size_t *sz)
{
        hb_mc_eva_t eva_masked, eva_dmem;
        size_t dmem_size;
        dmem_size = hb_mc_config_get_dmem_size(cfg);
        eva_masked = hb_mc_eva_addr(eva) & MAKE_MASK(HB_MC_EPA_LOGSZ);
        eva_dmem = eva_masked - HB_MC_TILE_EVA_DMEM_BASE;

        if(eva_dmem < dmem_size){
                *epa = eva_dmem + HB_MC_TILE_EPA_DMEM_BASE;
                *sz = dmem_size - eva_dmem;
        }else if(eva_masked == HB_MC_TILE_EPA_CSR_FREEZE){
                *epa = eva_masked;
                *sz = sizeof(uint32_t);
        }else if(eva_masked == HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_X){
                *epa = eva_masked;
                *sz = sizeof(uint32_t);
        }else if(eva_masked == HB_MC_TILE_EPA_CSR_TILE_GROUP_ORIGIN_Y){
                *epa = eva_masked;
                *sz = sizeof(uint32_t);
        } else {
                bsg_pr_err("%s: Invalid EVA Address 0x%08" PRIx32 ". Does not map to an"
                           " addressible tile memory locatiion.\n",
                           __func__, hb_mc_eva_addr(eva));
                *epa = 0;
                *sz = 0;
                return HB_MC_FAIL;
        }
        return HB_MC_SUCCESS;
}


/**
 * Converts a local Endpoint Virtual Address to a Network Physical Address
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  o      Coordinate of the origin for this tile's group
 * @param[in]  src    Coordinate of the tile issuing this #eva
 * @param[in]  eva    An eva to translate
 * @param[out] npa    An npa to be set by translating #eva
 * @param[out] sz     The size in bytes of the NPA segment for the #eva
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int default_eva_to_npa_local(const hb_mc_config_t *cfg,
                                    const hb_mc_coordinate_t *o,
                                    const hb_mc_coordinate_t *src,
                                    const hb_mc_eva_t *eva,
                                    hb_mc_npa_t *npa, size_t *sz)
{
        int rc;
        hb_mc_idx_t x, y;
        hb_mc_epa_t epa;

        x = hb_mc_coordinate_get_x(*src);
        y = hb_mc_coordinate_get_y(*src);

        rc = default_eva_to_epa_tile(cfg, eva, &epa, sz);
        if (rc != HB_MC_SUCCESS)
                return rc;
        *npa = hb_mc_epa_to_npa(hb_mc_coordinate(x,y), epa);

        bsg_pr_dbg("%s: Translating EVA 0x%08" PRIx32 " for tile (x: %d y: %d) to NPA {x: %d y: %d, EPA: 0x%08" PRIx32 "}. \n",
                   __func__, hb_mc_eva_addr(eva),
                   hb_mc_coordinate_get_x(*src),
                   hb_mc_coordinate_get_y(*src),
                   hb_mc_npa_get_x(npa),
                   hb_mc_npa_get_y(npa),
                   hb_mc_npa_get_epa(npa));
        return HB_MC_SUCCESS;
}

/**
 * Determines if an EVA is a group EVA
 * @return true if EVA addresses group memory, false otherwise
 */
static bool default_eva_is_group(const hb_mc_eva_t *eva)
{
        return (hb_mc_eva_addr(eva) & DEFAULT_GROUP_BITMASK) != 0;
}

/**
 * Converts a group Endpoint Virtual Address to a Network Physical Address
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  o      Coordinate of the origin for this tile's group
 * @param[in]  src    Coordinate of the tile issuing this #eva
 * @param[in]  eva    An eva to translate
 * @param[out] npa    An npa to be set by translating #eva
 * @param[out] sz     The size in bytes of the NPA segment for the #eva
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int default_eva_to_npa_group(const hb_mc_config_t *cfg,
                                    const hb_mc_coordinate_t *o,
                                    const hb_mc_coordinate_t *src,
                                    const hb_mc_eva_t *eva,
                                    hb_mc_npa_t *npa, size_t *sz)
{
        int rc;
        hb_mc_dimension_t dim;
        hb_mc_idx_t x, y, ox, oy, dim_x, dim_y;
        hb_mc_epa_t epa;

        dim = hb_mc_config_get_dimension_vcore(cfg);
        dim_x = hb_mc_dimension_get_x(dim) + hb_mc_config_get_vcore_base_x(cfg);
        dim_y = hb_mc_dimension_get_y(dim) + hb_mc_config_get_vcore_base_y(cfg);
        ox = hb_mc_coordinate_get_x(*o);
        oy = hb_mc_coordinate_get_y(*o);
        x = ((hb_mc_eva_addr(eva) & DEFAULT_GROUP_X_BITMASK) >> DEFAULT_GROUP_X_BITIDX);
        x += ox;
        y = ((hb_mc_eva_addr(eva) & DEFAULT_GROUP_Y_BITMASK) >> DEFAULT_GROUP_Y_BITIDX);
        y += oy;
        if(dim_x < x){
                bsg_pr_err("%s: Invalid Group EVA. X coordinate destination %d"
                           "is larger than current manycore configuration\n",
                           __func__, x);
                return HB_MC_FAIL;
        }

        if(dim_y < y){
                bsg_pr_err("%s: Invalid Group EVA. Y coordinate destination %d"
                           "is larger than current manycore configuration\n",
                           __func__, y);
                return HB_MC_FAIL;
        }

        rc = default_eva_to_epa_tile(cfg, eva, &epa, sz);
        if (rc != HB_MC_SUCCESS)
                return rc;
        *npa = hb_mc_epa_to_npa(hb_mc_coordinate(x,y), epa);

        bsg_pr_dbg("%s: Translating EVA 0x%08" PRIx32 " for tile (x: %d y: %d) to NPA {x: %d y: %d, EPA: 0x%08" PRIx32 "}. \n",
                   __func__, hb_mc_eva_addr(eva),
                   hb_mc_coordinate_get_x(*src),
                   hb_mc_coordinate_get_y(*src),
                   hb_mc_npa_get_x(npa),
                   hb_mc_npa_get_y(npa),
                   hb_mc_npa_get_epa(npa));

        return HB_MC_SUCCESS;
}

/**
 * Determines if an EVA is a global EVA
 * @return true if EVA addresses global memory, false otherwise
 */
static bool default_eva_is_global(const hb_mc_eva_t *eva)
{
        return (hb_mc_eva_addr(eva) & DEFAULT_GLOBAL_BITMASK) != 0;
}

/**
 * Converts a global Endpoint Virtual Address to a Network Physical Address
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  o      Coordinate of the origin for this tile's group
 * @param[in]  src    Coordinate of the tile issuing this #eva
 * @param[in]  eva    An eva to translate
 * @param[out] npa    An npa to be set by translating #eva
 * @param[out] sz     The size in bytes of the NPA segment for the #eva
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int default_eva_to_npa_global(const hb_mc_config_t *cfg,
                                     const hb_mc_coordinate_t *o,
                                     const hb_mc_coordinate_t *src,
                                     const hb_mc_eva_t *eva,
                                     hb_mc_npa_t *npa, size_t *sz)
{
        int rc;
        hb_mc_idx_t x, y;
        hb_mc_epa_t epa;


        x = ((hb_mc_eva_addr(eva) & DEFAULT_GLOBAL_X_BITMASK) >> DEFAULT_GLOBAL_X_BITIDX);
        y = ((hb_mc_eva_addr(eva) & DEFAULT_GLOBAL_Y_BITMASK) >> DEFAULT_GLOBAL_Y_BITIDX);
        rc = default_eva_to_epa_tile(cfg, eva, &epa, sz);
        if (rc != HB_MC_SUCCESS)
                return rc;
        *npa = hb_mc_epa_to_npa(hb_mc_coordinate(x,y), epa);

        bsg_pr_dbg("%s: Translating EVA 0x%08" PRIx32 " for tile (x: %d y: %d) to NPA {x: %d y: %d, EPA: 0x%08" PRIx32 "}. \n",
                   __func__, hb_mc_eva_addr(eva),
                   hb_mc_coordinate_get_x(*src),
                   hb_mc_coordinate_get_y(*src),
                   hb_mc_npa_get_x(npa),
                   hb_mc_npa_get_y(npa),
                   hb_mc_npa_get_epa(npa));

        return HB_MC_SUCCESS;
}

/**
 * Determines if an EVA is in DRAM
 * @return true if EVA addresses DRAM memory, false otherwise
 */
static bool default_eva_is_dram(const hb_mc_eva_t *eva)
{
        return (hb_mc_eva_addr(eva) & DEFAULT_DRAM_BITMASK) != 0;
}

static uint32_t default_get_dram_max_x_coord(const hb_mc_config_t *cfg)
{
        hb_mc_dimension_t dim = hb_mc_config_get_dimension_network(cfg);
        return hb_mc_dimension_get_x(dim);
}

static uint32_t default_get_x_dimlog(const hb_mc_config_t *cfg)
{
        hb_mc_dimension_t dim = hb_mc_config_get_dimension_network(cfg);
        return ceil(log2(hb_mc_dimension_get_x(dim)));
}

static uint32_t default_get_dram_x_bitidx(const hb_mc_config_t *cfg)
{
        uint32_t xdimlog;
        // The number of bits used for the x index is determined by clog2 of the
        // x dimension (or the number of bits needed to represent the maximum x
        // dimension).
        xdimlog = default_get_x_dimlog(cfg);
        return MAKE_MASK(xdimlog);
}

static uint32_t default_get_dram_stripe_size_log(const hb_mc_manycore_t *mc)
{
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        return ceil(log2(hb_mc_config_get_vcache_stripe_size(cfg)));
}

static uint32_t default_get_dram_bitwidth(const hb_mc_manycore_t *mc)
{
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        if (hb_mc_manycore_dram_is_enabled(mc)) {
                return hb_mc_config_get_vcache_bitwidth_data_addr(cfg);
        } else {
                return ceil(log2(hb_mc_config_get_vcache_size(cfg))); // clog2(victim cache size)
        }
}

static uint32_t default_get_dram_x_shift_dep(const hb_mc_manycore_t *mc)
{
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        if (hb_mc_manycore_dram_is_enabled(mc)) {
                return hb_mc_config_get_vcache_bitwidth_data_addr(cfg);
        } else {
                return ceil(log2(hb_mc_config_get_vcache_size(cfg))); // clog2(victim cache size)
        }
}

// See comments on default_eva_to_npa_dram 
static int default_eva_get_x_coord_dram(const hb_mc_manycore_t *mc,
                                        const hb_mc_config_t *cfg,
                                        const hb_mc_eva_t *eva,
                                        hb_mc_idx_t *x) { 
        uint32_t stripe_log = default_get_dram_stripe_size_log(mc);
        uint32_t xmask = default_get_dram_x_bitidx(cfg);
        uint32_t dram_max_x_coord = default_get_dram_max_x_coord(cfg);

        *x = (hb_mc_eva_addr(eva) >> stripe_log) & xmask;
        if ( *x > dram_max_x_coord) { 
                bsg_pr_err("%s: Translation of EVA 0x%08" PRIx32 " failed. The X coordinate "
                           "of the DRAM bank for the requested EPA %d is larger than max %d\n.",
                           __func__, hb_mc_eva_addr(eva),
                           *x, dram_max_x_coord);
                return HB_MC_INVALID;
        }
        return HB_MC_SUCCESS;
}

// See comments on default_eva_to_npa_dram 
static int default_eva_get_y_coord_dram(const hb_mc_manycore_t *mc,
                                        const hb_mc_config_t *cfg,
                                        const hb_mc_eva_t *eva,
                                        hb_mc_idx_t *y) { 

        // Y can either be the North or South boundary of the chip
        uint32_t shift
                = default_get_dram_stripe_size_log(mc) // stripe byte-offset bits
                + default_get_x_dimlog(cfg); // x-coordinate bits

        uint32_t is_south = (hb_mc_eva_addr(eva) >> shift) & 1;

        *y = is_south
                ? hb_mc_config_get_dram_high_y(cfg)
                : hb_mc_config_get_dram_low_y(cfg);

        bsg_pr_dbg("%s: Translating Y-coordinate = %u for EVA 0x%08" PRIx32 "\n",
                   __func__, *y, *eva);

        return HB_MC_SUCCESS;
}

// See comments on default_eva_to_npa_dram 
static int default_eva_get_epa_dram (const hb_mc_manycore_t *mc,
                                     const hb_mc_config_t *cfg,
                                     const hb_mc_eva_t *eva,
                                     hb_mc_epa_t *epa,
                                     size_t *sz) { 
 
        uint32_t xdimlog    = default_get_x_dimlog(cfg);
        uint32_t stripe_log = default_get_dram_stripe_size_log(mc);
        uint32_t shift
                = stripe_log // stripe byte-offset bits
                + xdimlog    // x-coordinate bits
                + 1;         // north-south bit

        // Refer to comments on default_eva_to_npa_dram for more clarification
        // DRAM EPA  =  EPA_top + block_offset + word_addressible  
        // Construct (block_offset + word_addressible) portion of EPA
        // i.e. the <stripe_log> lower bits of the EVA 
        *epa = (hb_mc_eva_addr(eva) & MAKE_MASK(stripe_log));
        // Construct the EPA_top portion of EPA and append to lower bits  
        // Shift right by (stripe_log + x_dimlog) and shift left by stripe_log
        // to remove the X_coord porition of EVA 
        *epa |= (((hb_mc_eva_addr(eva) & MAKE_MASK(DEFAULT_DRAM_BITIDX)) >> shift ) << stripe_log);


        // The EPA portion of an EVA is technically determined by EPA_top + 
        // block_offset + word_addressible (refer to the comments above this function).
        // However, this creates undefined behavior when (addrbits + 1 +
        // xdimlog) != DEFAULT_DRAM_BITIDX, since there are unused bits between
        // the x index and EPA.  To avoid really awful debugging, we check this
        // situation.
        uint32_t addrbits = default_get_dram_bitwidth(mc);
        uint32_t errmask = MAKE_MASK(addrbits);
        size_t max_dram_sz = 1 << addrbits;

        if (*epa >= max_dram_sz){
                bsg_pr_err("%s: Translation of EVA 0x%08" PRIx32 " failed. "
                           "Requested EPA 0x%08" PRIx32 " is outside of "
                           "DRAM's addressable range 0x%08" PRIx32 ".\n",
                           __func__,
                           hb_mc_eva_addr(eva),
                           *epa,
                           uint32_t(max_dram_sz));
                return HB_MC_INVALID;
        }


        // Maximum permitted size to write starting from this epa is from 
        // the block offset until the end of the striped block.
        uint32_t max_striped_block_size = 1 << stripe_log;
        *sz = max_striped_block_size - (hb_mc_eva_addr(eva) & MAKE_MASK(stripe_log));

        return HB_MC_SUCCESS;
}

/**
 * Converts a DRAM Endpoint Virtual Address to a Network Physical Address and
 * size (contiguous bytes following the specified EVA)
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  o      Coordinate of the origin for this tile's group
 * @param[in]  src    Coordinate of the tile issuing this #eva
 * @param[in]  eva    An eva to translate
 * @param[out] npa    An npa to be set by translating #eva
 * @param[out] sz     The size in bytes of the NPA segment for the #eva
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 *
 * To better understand the translation:
 * DRAM EVA:                 1        -    ******     -    ******    -       ******       -          00
 * Section                DRAM bit    -    EPA_top    -    X coord   -     block_offset   -     word_addressable
 * # of bits                 1                        -<---xdimlog-->-<-------------stripe_log----------------->
 * # of bits                          -<---------->         +         <----------------------------------------> = addrbits     
 * Stripe size (32)         [31]      -    [30:7]     -     [6:5]    -        [4:2]       -         [1:0]
 * No stripe (deprecated)   [31]      -      N/A      -    [30:29]   -        [28:2]      -         [1:0]
 * (i.e. stripe size = dram bank size = 0x800_0000)
 *
 * DRAM EPA  =  EPA_top + block_offset + word_addressible  
 * DRAM NPA  =  <Y coord, X coord, DRAM EPA>
 */
static int default_eva_to_npa_dram(const hb_mc_manycore_t *mc,
                                   const hb_mc_coordinate_t *o,
                                   const hb_mc_coordinate_t *src,
                                   const hb_mc_eva_t *eva,
                                   hb_mc_npa_t *npa,
                                   size_t *sz)
{
        int rc;
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        hb_mc_idx_t x,y;
        hb_mc_epa_t epa;

        // Calculate X coordinate of NPA from EVA
        rc = default_eva_get_x_coord_dram (mc, cfg, eva, &x); 
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to generate x coordinate from eva 0x%08" PRIx32 ".\n",
                           __func__,
                           hb_mc_eva_addr(eva));
                return rc;
        }

        // Calculate Y coordinate of NPA from EVA
        rc = default_eva_get_y_coord_dram (mc, cfg, eva, &y);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to generate y coordinate from eva 0x%08" PRIx32 ".\n",
                           __func__,
                           hb_mc_eva_addr(eva));
                return rc;
        }


        // Calculate EPA Portion of NPA from EVA
        rc = default_eva_get_epa_dram (mc, cfg, eva, &epa, sz);
        if (rc != HB_MC_SUCCESS) { 
                bsg_pr_err("%s: failed to generate npa from eva 0x%08" PRIx32 ".\n",
                           __func__,
                           hb_mc_eva_addr(eva));
                return rc;
        }

        *npa = hb_mc_epa_to_npa(hb_mc_coordinate(x,y), epa);

        bsg_pr_dbg("%s: Translating EVA 0x%08" PRIx32 " for tile (x: %d y: %d) to NPA {x: %d y: %d, EPA: 0x%08" PRIx32 "} sz = %08x. \n",
                   __func__, hb_mc_eva_addr(eva),
                   hb_mc_coordinate_get_x(*src),
                   hb_mc_coordinate_get_y(*src),
                   hb_mc_npa_get_x(npa),
                   hb_mc_npa_get_y(npa),
                   hb_mc_npa_get_epa(npa),
                   uint32_t(*sz));

        return HB_MC_SUCCESS;
}

/**
 * Translate an Endpoint Virtual Address in a source tile's address space
 * to a Network Physical Address
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  priv   Private data used for this EVA Map
 * @param[in]  src    Coordinate of the tile issuing this #eva
 * @param[in]  eva    An eva to translate
 * @param[out] npa    An npa to be set by translating #eva
 * @param[out] sz     The size in bytes of the NPA segment for the #eva
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int default_eva_to_npa(hb_mc_manycore_t *mc,
                       const void *priv,
                       const hb_mc_coordinate_t *src,
                       const hb_mc_eva_t *eva,
                       hb_mc_npa_t *npa, size_t *sz)
{
        const hb_mc_coordinate_t *origin;
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        origin = (const hb_mc_coordinate_t *) priv;

        if(default_eva_is_dram(eva))
                return default_eva_to_npa_dram(mc, origin, src, eva, npa, sz);
        if(default_eva_is_global(eva))
                return default_eva_to_npa_global(cfg, origin, src, eva, npa, sz);
        if(default_eva_is_group(eva))
                return default_eva_to_npa_group(cfg, origin, src, eva, npa, sz);
        if(default_eva_is_local(eva))
                return default_eva_to_npa_local(cfg, origin, src, eva, npa, sz);

        bsg_pr_err("%s: EVA 0x%08" PRIx32 " did not map to a known region\n",
                   __func__, hb_mc_eva_addr(eva));
        return HB_MC_FAIL;
}

/**
 * Check if a DRAM EPA is valid.
 * @param[in] config  An initialized manycore configuration struct
 * @param[in] npa     An npa to translate
 * @param[in] tgt     Coordinates of the target tile
 * @return true if the EPA is valid, false otherwise.
 */
static bool default_dram_epa_is_valid(const hb_mc_manycore_t *mc,
                                      hb_mc_epa_t epa,
                                      const hb_mc_coordinate_t *tgt)
{
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        if (hb_mc_manycore_dram_is_enabled(mc)) {
                return epa < hb_mc_config_get_dram_size(cfg);
        } else {
                return epa < hb_mc_config_get_vcache_size(cfg);
        }
}


/**
 * Check if a local EPA is valid.
 * @param[in] config  An initialized manycore configuration struct
 * @param[in] npa     An npa to translate
 * @param[in] tgt     Coordinates of the target tile
 * @return true if the EPA is valid, false otherwise.
 */
static bool default_local_epa_is_valid(const hb_mc_config_t *config,
                                       hb_mc_epa_t epa,
                                       const hb_mc_coordinate_t *tgt)
{
        hb_mc_epa_t floor = HB_MC_TILE_EPA_DMEM_BASE;
        hb_mc_epa_t ceil  = HB_MC_TILE_EPA_DMEM_BASE + hb_mc_config_get_dmem_size(config);
        return (epa >= floor) && (epa < ceil);
}

/**
 * Check if an NPA is a host DRAM.
 * @param[in] config  An initialized manycore configuration struct
 * @param[in] npa     An npa to translate
 * @param[in] tgt     Coordinates of the target tile
 * @return true if the NPA is DRAM, false otherwise.
 */
static bool default_npa_is_dram(const hb_mc_manycore_t *mc,
                                const hb_mc_npa_t *npa,
                                const hb_mc_coordinate_t *tgt)
{
        char npa_str[64];
        const hb_mc_config_t *config = hb_mc_manycore_get_config(mc);
        bool is_dram
                = hb_mc_config_is_dram_y(config, hb_mc_npa_get_y(npa))
                && default_dram_epa_is_valid(mc, hb_mc_npa_get_epa(npa), tgt);

        bsg_pr_dbg("%s: npa %s %s DRAM\n",
                   __func__,
                   hb_mc_npa_to_string(npa, npa_str, sizeof(npa_str)),
                   (is_dram ? "is" : "is not"));

        return is_dram;
}

/**
 * Check if an NPA is a host address.
 * @param[in] config  An initialized manycore configuration struct
 * @param[in] npa     An npa to translate
 * @param[in] tgt     Coordinates of the target tile
 * @return true if the NPA is host, false otherwise.
 */
static bool default_npa_is_host(const hb_mc_config_t *config,
                                const hb_mc_npa_t *npa,
                                const hb_mc_coordinate_t *tgt)
{
        char npa_str[64];
        hb_mc_coordinate_t host = hb_mc_config_get_host_interface(config);
        bool is_host = hb_mc_coordinate_get_x(host) == hb_mc_npa_get_x(npa) &&
                hb_mc_coordinate_get_y(host) == hb_mc_npa_get_y(npa);

        bsg_pr_dbg("%s: npa %s %s a host address\n",
                   __func__,
                   hb_mc_npa_to_string(npa, npa_str, sizeof(npa_str)),
                   (is_host ? "is" : "is not"));

        // does your coordinate map to the host?
        // I guess we're generally permissive with host EPAs
        return is_host;
}

/**
 * Check if an NPA is a local address.
 * @param[in] config  An initialized manycore configuration struct
 * @param[in] npa     An npa to translate
 * @param[in] tgt     Coordinates of the target tile
 * @return true if the NPA is local, false otherwise.
 */
static bool default_npa_is_local(const hb_mc_config_t *config,
                                 const hb_mc_npa_t *npa,
                                 const hb_mc_coordinate_t *tgt)
{
        // does your coordinate map to this tgt v-core and is your epa valid?
        return (hb_mc_npa_get_x(npa) == hb_mc_coordinate_get_x(*tgt)) &&
                (hb_mc_npa_get_y(npa) == hb_mc_coordinate_get_y(*tgt)) &&
                default_local_epa_is_valid(config, hb_mc_npa_get_epa(npa), tgt);
}

/**
 * Check if an NPA is a global address.
 * @param[in] config  An initialized manycore configuration struct
 * @param[in] npa     An npa to translate
 * @param[in] tgt     Coordinates of the target tile
 * @return true if the NPA is global, false otherwise.
 */
static bool default_npa_is_global(const hb_mc_config_t *config,
                                  const hb_mc_npa_t *npa,
                                  const hb_mc_coordinate_t *tgt)
{
        hb_mc_dimension_t dim = hb_mc_config_get_dimension_vcore(config);
        hb_mc_idx_t base_x = hb_mc_config_get_vcore_base_x(config);
        hb_mc_idx_t base_y = hb_mc_config_get_vcore_base_y(config);
        hb_mc_idx_t ceil_x = base_x + hb_mc_coordinate_get_x(dim);
        hb_mc_idx_t ceil_y = base_y + hb_mc_coordinate_get_y(dim);

        // does your coordinate map to any v-core and is your epa valid?
        return (hb_mc_npa_get_x(npa) >= base_x) && (hb_mc_npa_get_x(npa) <= ceil_x) &&
                (hb_mc_npa_get_y(npa) >= base_y) && (hb_mc_npa_get_y(npa) <= ceil_y) &&
                default_local_epa_is_valid(config, hb_mc_npa_get_epa(npa), tgt);

}

/**
 * Translate a global NPA to an EVA.
 * @param[in]  cfg      An initialized manycore configuration struct
 * @param[in]  origin   Coordinate of the origin for this tile's group
 * @param[in]  tgt      Coordinates of the target tile
 * @param[in]  npa      An npa to translate
 * @param[out] eva      An eva to set by translating #npa
 * @param[out] sz       The size in bytes of the EVA segment for the #npa
 * @return HB_MC_SUCCESS if succesful. HB_MC_FAIL otherwise.
 */
static int default_npa_to_eva_dram(hb_mc_manycore_t *mc,
                                   const hb_mc_coordinate_t *origin,
                                   const hb_mc_coordinate_t *tgt,
                                   const hb_mc_npa_t *npa,
                                   hb_mc_eva_t *eva,
                                   size_t *sz)
{
        // build the eva
        hb_mc_eva_t addr = 0;
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        uint32_t stripe_log, xdimlog;
        uint32_t is_south = hb_mc_npa_get_y(npa) == hb_mc_config_get_dram_high_y(cfg);

        stripe_log = default_get_dram_stripe_size_log(mc);
        xdimlog    = default_get_x_dimlog(cfg);

        // See comments on default_eva_to_npa_dram for clarification
        addr |= (hb_mc_npa_get_epa(npa) & MAKE_MASK(stripe_log)); // Set byte address and cache block offset
        addr |= (hb_mc_npa_get_x(npa) << stripe_log); // Set the x coordinate
        addr |= (is_south << (stripe_log + xdimlog)); // Set the N-S bit
        addr |= (((hb_mc_npa_get_epa(npa) >> stripe_log)) << (stripe_log + xdimlog + 1)); // Set the EPA section
        addr |= (1 << DEFAULT_DRAM_BITIDX); // Set the DRAM bit
        *eva  = addr;

        // this is lame but we are basically saying "you can write to this word only"
        *sz = 4 - (hb_mc_npa_get_epa(npa) & 0x3);

        return HB_MC_SUCCESS;
}


/**
 * Translate a global NPA to an EVA.
 * @param[in]  cfg      An initialized manycore configuration struct
 * @param[in]  origin   Coordinate of the origin for this tile's group
 * @param[in]  tgt      Coordinates of the target tile
 * @param[in]  npa      An npa to translate
 * @param[out] eva      An eva to set by translating #npa
 * @param[out] sz       The size in bytes of the EVA segment for the #npa
 * @return HB_MC_SUCCESS if succesful. HB_MC_FAIL otherwise.
 */
__attribute__((deprecated))
static int default_npa_to_eva_dram_dep(hb_mc_manycore_t *mc,
                                       const hb_mc_coordinate_t *origin,
                                       const hb_mc_coordinate_t *tgt,
                                       const hb_mc_npa_t *npa,
                                       hb_mc_eva_t *eva,
                                       size_t *sz)
{
        // build the eva
        hb_mc_eva_t addr = 0;
        hb_mc_eva_t xshift = default_get_dram_x_shift_dep(mc);

        addr |= hb_mc_npa_get_epa(npa); // set the byte address
        addr |= hb_mc_npa_get_x(npa) << xshift; // set the x coordinate
        addr |= 1 << DEFAULT_DRAM_BITIDX; // set the DRAM bit
        *eva  = addr;

        // this is lame but we are basically saying "you can write to this word only"
        *sz = 4 - (hb_mc_npa_get_epa(npa) & 0x3);

        // done
        return HB_MC_SUCCESS;
}

/**
 * Translate a global NPA to an EVA.
 * @param[in]  cfg      An initialized manycore configuration struct
 * @param[in]  origin   Coordinate of the origin for this tile's group
 * @param[in]  tgt      Coordinates of the target tile
 * @param[in]  npa      An npa to translate
 * @param[out] eva      An eva to set by translating #npa
 * @param[out] sz       The size in bytes of the EVA segment for the #npa
 * @return HB_MC_SUCCESS if succesful. HB_MC_FAIL otherwise.
 */
static int default_npa_to_eva_global_remote(const hb_mc_config_t *cfg,
                                            const hb_mc_coordinate_t *origin,
                                            const hb_mc_coordinate_t *tgt,
                                            const hb_mc_npa_t *npa,
                                            hb_mc_eva_t *eva,
                                            size_t *sz)
{
        // build the eva
        hb_mc_eva_t addr = 0;
        addr |= hb_mc_npa_get_epa(npa); // set the byte address
        addr |= hb_mc_npa_get_x(npa) << DEFAULT_GLOBAL_X_BITIDX; // set x coordinate
        addr |= hb_mc_npa_get_y(npa) << DEFAULT_GLOBAL_Y_BITIDX; // set y coordinate
        addr |= 1 << DEFAULT_GLOBAL_BITIDX; // set the global bit

        *eva = addr;

        // this is lame but we are basically saying "you can write to this word only"
        *sz = 4 - (hb_mc_npa_get_epa(npa) & 0x3);

        // done
        return HB_MC_SUCCESS;
}

//////////////////////////////////////////////////////////////////
// At the moment we treat host, local, and globals all the same //
//////////////////////////////////////////////////////////////////

/**
 * Translate an NPA for the host interface to an EVA.
 * @param[in]  cfg      An initialized manycore configuration struct
 * @param[in]  origin   Coordinate of the origin for this tile's group
 * @param[in]  tgt      Coordinates of the target tile
 * @param[in]  npa      An npa to translate
 * @param[out] eva      An eva to set by translating #npa
 * @param[out] sz       The size in bytes of the EVA segment for the #npa
 * @return HB_MC_SUCCESS if succesful. HB_MC_FAIL otherwise.
 */
static int default_npa_to_eva_host(const hb_mc_config_t *cfg,
                                   const hb_mc_coordinate_t *origin,
                                   const hb_mc_coordinate_t *tgt,
                                   const hb_mc_npa_t *npa,
                                   hb_mc_eva_t *eva,
                                   size_t *sz)
{
        return default_npa_to_eva_global_remote(cfg, origin, tgt, npa, eva, sz);
}

/**
 * Translate an local NPA to an EVA.
 * @param[in]  cfg      An initialized manycore configuration struct
 * @param[in]  origin   Coordinate of the origin for this tile's group
 * @param[in]  tgt      Coordinates of the target tile
 * @param[in]  npa      An npa to translate
 * @param[out] eva      An eva to set by translating #npa
 * @param[out] sz       The size in bytes of the EVA segment for the #npa
 * @return HB_MC_SUCCESS if succesful. HB_MC_FAIL otherwise.
 */
static int default_npa_to_eva_local(const hb_mc_config_t *cfg,
                                    const hb_mc_coordinate_t *origin,
                                    const hb_mc_coordinate_t *tgt,
                                    const hb_mc_npa_t *npa,
                                    hb_mc_eva_t *eva,
                                    size_t *sz)
{
        return default_npa_to_eva_global_remote(cfg, origin, tgt, npa, eva, sz);
}

/**
 * Translate a global NPA to an EVA.
 * @param[in]  cfg      An initialized manycore configuration struct
 * @param[in]  origin   Coordinate of the origin for this tile's group
 * @param[in]  tgt      Coordinates of the target tile
 * @param[in]  npa      An npa to translate
 * @param[out] eva      An eva to set by translating #npa
 * @param[out] sz       The size in bytes of the EVA segment for the #npa
 * @return HB_MC_SUCCESS if succesful. HB_MC_FAIL otherwise.
 */
static int default_npa_to_eva_global(const hb_mc_config_t *cfg,
                                     const hb_mc_coordinate_t *origin,
                                     const hb_mc_coordinate_t *tgt,
                                     const hb_mc_npa_t *npa,
                                     hb_mc_eva_t *eva,
                                     size_t *sz)
{
        return default_npa_to_eva_global_remote(cfg, origin, tgt, npa, eva, sz);
}

/**
 * Translate a Network Physical Address to an Endpoint Virtual Address in a
 * target tile's address space
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  priv   Private data used for this EVA Map
 * @param[in]  tgt    Coordinates of the target tile
 * @param[in]  len    Number of tiles in the target tile's group
 * @param[in]  npa    An npa to translate
 * @param[out] eva    An eva to set by translating #npa
 * @param[out] sz     The size in bytes of the EVA segment for the #npa
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int default_npa_to_eva(hb_mc_manycore_t *mc,
                       const void *priv,
                       const hb_mc_coordinate_t *tgt,
                       const hb_mc_npa_t *npa,
                       hb_mc_eva_t *eva, size_t *sz)
{
        const hb_mc_coordinate_t *origin = (const hb_mc_coordinate_t*)priv;
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);

        if(default_npa_is_dram(mc, npa, tgt))
                return default_npa_to_eva_dram(mc, origin, tgt, npa, eva, sz);

        if(default_npa_is_host(cfg, npa, tgt))
                return default_npa_to_eva_host(cfg, origin, tgt, npa, eva, sz);

        if(default_npa_is_local(cfg, npa, tgt))
                return default_npa_to_eva_local(cfg, origin, tgt, npa, eva, sz);

        if(default_npa_is_global(cfg, npa, tgt))
                return default_npa_to_eva_global(cfg, origin, tgt, npa, eva, sz);

        return HB_MC_FAIL;
}

/**
 * Returns the number of contiguous bytes following an EVA, regardless of
 * the continuity of the underlying NPA.
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  priv   Private data used for this EVA Map
 * @param[in]  eva    An eva
 * @param[out] sz     Number of contiguous bytes remaining in the #eva segment
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int default_eva_size(
                     hb_mc_manycore_t *mc,
                     const void *priv,
                     const hb_mc_eva_t *eva,
                     size_t *sz)
{
        hb_mc_npa_t npa;
        hb_mc_epa_t epa;
        const hb_mc_coordinate_t *o;
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        o = (const hb_mc_coordinate_t *) priv;

        if(default_eva_is_dram(eva))
                return default_eva_to_npa_dram(mc, o, o, eva, &npa, sz);
        if(default_eva_is_global(eva))
                return default_eva_to_npa_global(cfg, o, o, eva, &npa, sz);
        if(default_eva_is_group(eva))
                return default_eva_to_npa_group(cfg, o, o, eva, &npa, sz);
        if(default_eva_is_local(eva))
                return default_eva_to_npa_local(cfg, o, o, eva, &npa, sz);

        bsg_pr_err("%s: EVA 0x%08" PRIx32 " did not map to a known region\n",
                   __func__, hb_mc_eva_addr(eva));
        return HB_MC_FAIL;

}

const hb_mc_coordinate_t default_origin = {.x = HB_MC_CONFIG_VCORE_BASE_X,
                                           .y = HB_MC_CONFIG_VCORE_BASE_Y};
hb_mc_eva_map_t default_map = {
        .eva_map_name = "Default EVA space",
        .priv = (const void *)(&default_origin),
        .eva_to_npa = default_eva_to_npa,
        .eva_size = default_eva_size,
        .npa_to_eva  = default_npa_to_eva,
};

/**
 * Translate a Network Physical Address to an Endpoint Virtual Address in a
 * target tile's address space
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  map    An eva map for computing the eva to npa translation
 * @param[in]  tgt    Coordinates of the target tile
 * @param[in]  npa    An npa to translate
 * @param[out] eva    An eva to set by translating #npa
 * @param[out] sz     The size in bytes of the EVA segment for the #npa
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_npa_to_eva(hb_mc_manycore_t *mc,
                     const hb_mc_eva_map_t *map,
                     const hb_mc_coordinate_t *tgt,
                     const hb_mc_npa_t *npa,
                     hb_mc_eva_t *eva, size_t *sz)
{
        int err;

        err = map->npa_to_eva(mc, map->priv, tgt, npa, eva, sz);
        if (err != HB_MC_SUCCESS)
                return err;

        return HB_MC_SUCCESS;
}

/**
 * Translate an Endpoint Virtual Address in a source tile's address space
 * to a Network Physical Address
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  map    An eva map for computing the eva to npa translation
 * @param[in]  src    Coordinate of the tile issuing this #eva
 * @param[in]  eva    An eva to translate
 * @param[out] npa    An npa to be set by using #map to translate #eva
 * @param[out] sz     The size in bytes of the NPA segment for the #eva
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_eva_to_npa(hb_mc_manycore_t *mc,
                     const hb_mc_eva_map_t *map,
                     const hb_mc_coordinate_t *src,
                     const hb_mc_eva_t *eva,
                     hb_mc_npa_t *npa, size_t *sz)
{
        int err;

        err = map->eva_to_npa(mc, map->priv, src, eva, npa, sz);
        if (err != HB_MC_SUCCESS)
                return err;

        return HB_MC_SUCCESS;
}

/**
 * Translate an Endpoint Virtual Address in a source tile's address space
 * to a Network Physical Address
 * @param[in]  cfg    An initialized manycore configuration struct
 * @param[in]  eva    An eva to translate
 * @param[out] sz     Number of contiguous bytes remaining in the #eva segment
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_eva_size(hb_mc_manycore_t *mc,
                   const hb_mc_eva_map_t *map,
                   const hb_mc_eva_t *eva, size_t *sz)
{
        int err;

        err = map->eva_size(mc, map->priv, eva, sz);
        if (err != HB_MC_SUCCESS)
                return err;

        return HB_MC_SUCCESS;
}


static size_t min_size_t(size_t x, size_t y)
{
        return x < y ? x : y;
}

/**
 * Internal function to write memory out to manycore hardware starting at a given EVA
 * @param[in]  mc     An initialized manycore struct
 * @param[in]  map    An eva map for computing the eva to npa translation
 * @param[in]  tgt    Coordinate of the tile issuing this #eva
 * @param[in]  eva    A valid hb_mc_eva_t
 * @param[in]  data   A buffer to be written out manycore hardware
 * @param[in]  sz     The number of bytes to write to manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 *
 * This function implements the general algorithm for writing a contiguous EVA region.
 */
template <typename WriteFunction>
int hb_mc_manycore_eva_write_internal(hb_mc_manycore_t *mc,
                                      const hb_mc_eva_map_t *map,
                                      const hb_mc_coordinate_t *tgt,
                                      const hb_mc_eva_t *eva,
                                      const void *data, size_t sz,
                                      WriteFunction write_function)
{
        int err;
        size_t dest_sz, xfer_sz;
        hb_mc_npa_t dest_npa;
        char *destp;
        hb_mc_eva_t curr_eva = *eva;

        destp = (char *)data;
        while(sz > 0){
                err = hb_mc_eva_to_npa(mc, map, tgt, &curr_eva, &dest_npa, &dest_sz);
                if(err != HB_MC_SUCCESS){
                        bsg_pr_err("%s: Failed to translate EVA into a NPA\n",
                                   __func__);
                        return err;
                }
                xfer_sz = min_size_t(sz, dest_sz);

                char npa_str[256];
                bsg_pr_dbg("writing %zd bytes to eva %08x (%s)\n",
                           xfer_sz,
                           curr_eva,
                           hb_mc_npa_to_string(&dest_npa, npa_str, sizeof(npa_str)));

                err = write_function(mc, &dest_npa, destp, xfer_sz);
                if(err != HB_MC_SUCCESS){
                        bsg_pr_err("%s: Failed to copy data from host to NPA\n",
                                   __func__);
                        return err;
                }

                destp += xfer_sz;
                sz -= xfer_sz;
                curr_eva += xfer_sz;
        }

        return HB_MC_SUCCESS;
}

/**
 * Write memory out to manycore hardware starting at a given EVA via DMA
 * @param[in]  mc     An initialized manycore struct
 * @param[in]  map    An eva map for computing the eva to npa translation
 * @param[in]  tgt    Coordinate of the tile issuing this #eva
 * @param[in]  eva    A valid hb_mc_eva_t - must map to DRAM
 * @param[in]  data   A buffer to be written out manycore hardware
 * @param[in]  sz     The number of bytes to write to manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_eva_write_dma(hb_mc_manycore_t *mc,
                                 const hb_mc_eva_map_t *map,
                                 const hb_mc_coordinate_t *tgt,
                                 const hb_mc_eva_t *eva,
                                 const void *data, size_t sz)
{
        return hb_mc_manycore_eva_write_internal(mc, map, tgt, eva, data, sz,
                                                 hb_mc_manycore_dma_write_no_cache_ainv);
}

/**
 * Write memory out to manycore hardware starting at a given EVA
 * @param[in]  mc     An initialized manycore struct
 * @param[in]  map    An eva map for computing the eva to npa translation
 * @param[in]  tgt    Coordinate of the tile issuing this #eva
 * @param[in]  eva    A valid hb_mc_eva_t
 * @param[in]  data   A buffer to be written out manycore hardware
 * @param[in]  sz     The number of bytes to write to manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_eva_write(hb_mc_manycore_t *mc,
                             const hb_mc_eva_map_t *map,
                             const hb_mc_coordinate_t *tgt,
                             const hb_mc_eva_t *eva,
                             const void *data, size_t sz)
{
        // otherwise do write using the manycore mesh network
        return hb_mc_manycore_eva_write_internal(mc,map, tgt, eva, data, sz,
                                                 hb_mc_manycore_write_mem);
}


/**
 * Internal function to read memory from manycore hardware starting at a given EVA
 * @param[in]  mc     An initialized manycore struct
 * @param[in]  map    An eva map for computing the eva to npa map
 * @param[in]  tgt    Coordinate of the tile issuing this #eva
 * @param[in]  eva    A valid hb_mc_eva_t
 * @param[out] data   A buffer into which data will be read
 * @param[in]  sz     The number of bytes to read from the manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 *
 * This function implements the general algorithm for reading a contiguous EVA region.
 */
template <typename ReadFunction>
int hb_mc_manycore_eva_read_internal(hb_mc_manycore_t *mc,
                                     const hb_mc_eva_map_t *map,
                                     const hb_mc_coordinate_t *tgt,
                                     const hb_mc_eva_t *eva,
                                     void *data, size_t sz,
                                     ReadFunction read_function)
{
        int err;
        size_t src_sz, xfer_sz;
        hb_mc_npa_t src_npa;
        char *srcp;
        hb_mc_eva_t curr_eva = *eva;

        srcp = (char *)data;
        while(sz > 0){
                err = hb_mc_eva_to_npa(mc, map, tgt, &curr_eva, &src_npa, &src_sz);
                if(err != HB_MC_SUCCESS){
                        bsg_pr_err("%s: Failed to translate EVA into a NPA\n",
                                   __func__);
                        return err;
                }

                xfer_sz = min_size_t(sz, src_sz);

                char npa_str[256];
                bsg_pr_dbg("read %zd bytes from eva %08x (%s)\n",
                           xfer_sz,
                           curr_eva,
                           hb_mc_npa_to_string(&src_npa, npa_str, sizeof(npa_str)));

                err = read_function(mc, &src_npa, srcp, xfer_sz);
                if(err != HB_MC_SUCCESS){
                        bsg_pr_err("%s: Failed to copy data from host to NPA\n",
                                   __func__);
                        return err;
                }

                srcp += xfer_sz;
                sz -= xfer_sz;
                curr_eva += xfer_sz;
        }

        return HB_MC_SUCCESS;
}

/**
 * Read memory from manycore hardware starting at a given EVA via DMA
 * @param[in]  mc     An initialized manycore struct
 * @param[in]  map    An eva map for computing the eva to npa map
 * @param[in]  tgt    Coordinate of the tile issuing this #eva
 * @param[in]  eva    A valid hb_mc_eva_t - must map to DRAM
 * @param[out] data   A buffer into which data will be read
 * @param[in]  sz     The number of bytes to read from the manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_eva_read_dma(hb_mc_manycore_t *mc,
                                const hb_mc_eva_map_t *map,
                                const hb_mc_coordinate_t *tgt,
                                const hb_mc_eva_t *eva,
                                void *data, size_t sz)
{
        return hb_mc_manycore_eva_read_internal(mc, map, tgt, eva, data, sz,
                                                hb_mc_manycore_dma_read_no_cache_afl);
}

/**
 * Read memory from manycore hardware starting at a given EVA
 * @param[in]  mc     An initialized manycore struct
 * @param[in]  map    An eva map for computing the eva to npa map
 * @param[in]  tgt    Coordinate of the tile issuing this #eva
 * @param[in]  eva    A valid hb_mc_eva_t
 * @param[out] data   A buffer into which data will be read
 * @param[in]  sz     The number of bytes to read from the manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_eva_read(hb_mc_manycore_t *mc,
                            const hb_mc_eva_map_t *map,
                            const hb_mc_coordinate_t *tgt,
                            const hb_mc_eva_t *eva,
                            void *data, size_t sz)
{
        return hb_mc_manycore_eva_read_internal(mc, map, tgt, eva, data, sz,
                                                hb_mc_manycore_read_mem);
}

/**
 * Set a EVA memory region to a value
 * @param[in]  mc     An initialized manycore struct
 * @param[in]  map    An eva map for computing the eva to npa translation
 * @param[in]  tgt    Coordinate of the tile issuing this #eva
 * @param[in]  eva    A valid hb_mc_eva_t
 * @param[in]  val    The value to write to the region
 * @param[in]  sz     The number of bytes to write to manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_eva_memset(hb_mc_manycore_t *mc,
                              const hb_mc_eva_map_t *map,
                              const hb_mc_coordinate_t *tgt,
                              const hb_mc_eva_t *eva,
                              uint8_t val, size_t sz)
{
        int err;
        size_t dest_sz, xfer_sz;
        hb_mc_npa_t dest_npa;
        hb_mc_eva_t curr_eva = *eva;

        while(sz > 0){
                err = hb_mc_eva_to_npa(mc, map, tgt, &curr_eva, &dest_npa, &dest_sz);
                if(err != HB_MC_SUCCESS){
                        bsg_pr_err("%s: Failed to translate EVA into a NPA\n",
                                   __func__);
                        return err;
                }
                xfer_sz = min_size_t(sz, dest_sz);


                char npa_str[256];
                bsg_pr_dbg("read %zd bytes from eva %08x (%s)\n",
                           xfer_sz,
                           curr_eva,
                           hb_mc_npa_to_string(&dest_npa, npa_str, sizeof(npa_str)));

                err = hb_mc_manycore_memset(mc, &dest_npa, val, xfer_sz);
                if(err != HB_MC_SUCCESS){
                        bsg_pr_err("%s: Failed to set NPA region to value\n",
                                   __func__);
                        return err;
                }

                sz -= xfer_sz;
                curr_eva += xfer_sz;
        }

        return HB_MC_SUCCESS;
}
