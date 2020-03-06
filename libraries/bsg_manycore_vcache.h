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

/* Defines Victim Cache Macros and Constants */
#ifndef BSG_MANYCORE_VCACHE_H
#define BSG_MANYCORE_VCACHE_H
#include <bsg_manycore_features.h>
#include <bsg_manycore_epa.h>
#include <bsg_manycore_config.h>
#include <bsg_manycore.h>
#include <bsg_manycore_coordinate.h>
#ifdef __cplusplus
#include <cstdint>
#include <cstdio>
#else
#include <stdint.h>
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

        /* Offsets in bytes */
#define HB_MC_VCACHE_EPA_BASE          0x00000000
#define HB_MC_VCACHE_EPA_OFFSET_DRAM   0x00000000
#define HB_MC_VCACHE_EPA_OFFSET_TAG    0x20000000
#define HB_MC_VCACHE_EPA_RESERVED_BITS 1

        /* EPA Macros */
#define EPA_VCACHE_FROM_BYTE_OFFSET(offset)                     \
        EPA_FROM_BASE_AND_OFFSET(HB_MC_VCACHE_EPA_BASE, offset)

#define HB_MC_VCACHE_EPA_DRAM                                           \
        EPA_VCACHE_FROM_BYTE_OFFSET(HB_MC_VCACHE_EPA_OFFSET_DRAM)

#define HB_MC_VCACHE_EPA_TAG                                            \
        EPA_VCACHE_FROM_BYTE_OFFSET(HB_MC_VCACHE_EPA_OFFSET_TAG)

        /* Victim Cache Data Bits */
#define HB_MC_VCACHE_VALID_BITIDX 31
#define HB_MC_VCACHE_VALID (1 << HB_MC_VCACHE_VALID_BITIDX)

static
hb_mc_epa_t hb_mc_vcache_set_mask(const hb_mc_manycore_t *mc)
{
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        hb_mc_epa_t sets = hb_mc_config_get_vcache_sets(cfg);
        return sets-1;
}

static
hb_mc_epa_t hb_mc_vcache_set_shift(const hb_mc_manycore_t *mc)
{
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        hb_mc_epa_t bsize = hb_mc_config_get_vcache_block_size(cfg);
        return __builtin_popcount(bsize - 1);
}

static
hb_mc_epa_t hb_mc_vcache_set(const hb_mc_manycore_t *mc, hb_mc_epa_t epa)
{
        return (epa >> hb_mc_vcache_set_shift(mc)) & hb_mc_vcache_set_mask(mc);
}

static
hb_mc_epa_t hb_mc_vcache_way_mask(const hb_mc_manycore_t *mc)
{
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        hb_mc_epa_t ways = hb_mc_config_get_vcache_ways(cfg);
        return ways - 1;
}

static
hb_mc_epa_t hb_mc_vcache_way_shift(const hb_mc_manycore_t *mc)
{
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        hb_mc_epa_t sets = hb_mc_config_get_vcache_sets(cfg);
        return __builtin_popcount(sets-1) + hb_mc_vcache_set_shift(mc);
}

static
hb_mc_epa_t hb_mc_vcache_way(const hb_mc_manycore_t *mc, hb_mc_epa_t epa)
{
        return (epa >> hb_mc_vcache_way_shift(mc)) & hb_mc_vcache_way_mask(mc);
}

static
hb_mc_epa_t hb_mc_vcache_way_addr(const hb_mc_manycore_t *mc, hb_mc_epa_t set, hb_mc_epa_t way)
{
        return HB_MC_VCACHE_EPA_TAG | (way <<  hb_mc_vcache_way_shift(mc)) | (set << hb_mc_vcache_set_shift(mc));
}

static
hb_mc_npa_t hb_mc_vcache_way_npa(const hb_mc_manycore_t *mc, hb_mc_idx_t cache, hb_mc_epa_t set, hb_mc_epa_t way)
{
        const hb_mc_config_t *cfg  = hb_mc_manycore_get_config(mc);
        hb_mc_coordinate_t cache_xy = hb_mc_config_get_dram_coordinate(cfg, cache);
        return hb_mc_npa(cache_xy, hb_mc_vcache_way_addr(mc, set, way));
}

static
hb_mc_epa_t hb_mc_vcache_num_ways(const hb_mc_manycore_t *mc)
{
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        return hb_mc_config_get_vcache_ways(cfg);
}

static
hb_mc_epa_t hb_mc_vcache_num_sets(const hb_mc_manycore_t *mc)
{
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        return hb_mc_config_get_vcache_sets(cfg);
}

static
hb_mc_epa_t hb_mc_vcache_num_caches(const hb_mc_manycore_t *mc)
{
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);

        return hb_mc_config_get_num_dram_coordinates(cfg);
}

static
hb_mc_epa_t hb_mc_vcache_tag_epa(const hb_mc_manycore *mc, uint32_t tag)
{
    return tag;
}

static
const char *hb_mc_vcache_tag_to_string(const hb_mc_manycore_t *mc, uint32_t tag, char *buf, size_t sz)
{
    snprintf(buf, sz,
             "tag { .epa = 0x%08" PRIx32 " }",
             hb_mc_vcache_tag_epa(mc, tag));
    return buf;
}

#ifdef __cplusplus
};
#endif

#endif
