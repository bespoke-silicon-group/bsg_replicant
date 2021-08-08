// Copyright (c) 2020, University of Washington All rights reserved.
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

// BlackParrot utilities header file
// Commonly used BlackParrot functions with modifications for
// the manycore platform

#ifndef BP_UTILS_H
#define BP_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <bsg_newlib_intf.h>
#include <hb_bp_platform.h>

#define BP_CFG_BASE_ADDR 0x00200000

/**************************** BlackParrot helper functions ****************************/

/*
 * Get hardware thread ID
 * @returns result of reading the mhartid register
 */
uint64_t bp_get_hart();

/*
 * Checks to see if a barrier is finished
 * @param[in] barrier_address --> An address in memory that all cores to write to after hitting the barrier
 * @param[in] total_num_cores --> Number of cores
 */
void bp_barrier_end(volatile uint64_t *barrier_address, uint64_t total_num_cores);

/*
 * Sends a hex digit to the host to print
 * @param[in] hex --> Hex digit to print
 */
void bp_hprint(uint8_t hex);

/*
 * Sends a character to the host to print
 * @param[in] ch --> Character to print
 */
void bp_cprint(uint8_t ch);

/*
 * Sends a finish packet to the host
 * @param[in] code --> Finish code
 */
void bp_finish(int16_t code);

#ifdef __cplusplus
}
#endif

#endif
