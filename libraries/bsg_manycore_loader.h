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

#ifndef BSG_MANYCORE_LOADER_H
#define BSG_MANYCORE_LOADER_H

#include <bsg_manycore_features.h>
#include <bsg_manycore.h>
#include <bsg_manycore_eva.h>
#include <bsg_manycore_errno.h>

#ifdef __cplusplus
extern "C" {
#endif

        /**
         * Loads a binary object into a list of tiles and DRAM
         * @param[in]  bin    A memory buffer containing a valid manycore binary
         * @param[in]  sz     Size of #bin in bytes
         * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
         * @param[in]  map    An eva map for computing the eva to npa translation
         * @param[in]  tiles  A list of manycore to load with #bin, with the origin at 0
         * @param[in]  len    The number of tiles in #tiles
         * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
         */
        int hb_mc_loader_load(const void *bin, size_t sz,
                              hb_mc_manycore_t *mc,
                              const hb_mc_eva_map_t *map,
                              const hb_mc_coordinate_t *tiles,
                              uint32_t len, int enable_dram);

        /**
         * Get an EVA for a symbol from a program data.
         * @param[in]  bin     A memory buffer containing a valid manycore binary.
         * @param[in]  sz      Size of #bin in bytes.
         * @param[in]  symbol  A program symbol. Behavior is undefined if #symbol is not a zero terminated string.
         * @param[out] eva     An EVA that addresses #symbol. Behavior is undefined if #eva is invalid.
         * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
         */
        int hb_mc_loader_symbol_to_eva(const void *bin, size_t sz, const char *symbol,
                                       hb_mc_eva_t *eva);



        /**
         * Takes in the path to a binary and loads it into a buffer and sets the binary size.
         * @param[in]  file_name A memory buffer containing a valid manycore binary.
         * @param[out] file_data Pointer to the memory buffer to be loaded with a valid binary
         * @param[out] file_size Size of the binary in bytes.
         * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
         */
        int hb_mc_loader_read_program_file(const char *file_name, unsigned char **file_data, size_t *file_size);


#ifdef __cplusplus
}
#endif

#endif
