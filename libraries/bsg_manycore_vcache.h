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

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
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
#define EPA_VCACHE_FROM_BYTE_OFFSET(offset)			\
	EPA_FROM_BASE_AND_OFFSET(HB_MC_VCACHE_EPA_BASE, offset)

#define HB_MC_VCACHE_EPA_DRAM						\
	EPA_VCACHE_FROM_BYTE_OFFSET(HB_MC_VCACHE_EPA_OFFSET_DRAM)

#define HB_MC_VCACHE_EPA_TAG						\
	EPA_VCACHE_FROM_BYTE_OFFSET(HB_MC_VCACHE_EPA_OFFSET_TAG)

/* Victim Cache Data Bits */
#define HB_MC_VCACHE_VALID_BITIDX 31
#define HB_MC_VCACHE_VALID (1 << HB_MC_VCACHE_VALID_BITIDX)

#ifdef __cplusplus
};
#endif

#endif
