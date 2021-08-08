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

// BlackParrot platform-specific header file
// This file contains manycore-specific platform functions and constants

#ifndef HB_BP_PLATFORM_H
#define HB_BP_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <bsg_manycore.h>
#include <bsg_manycore_epa.h>

// Host X and Y coordinates
#ifndef HOST_X_COORD
#error HOST_X_COORD is undefined
#endif

#ifndef HOST_Y_COORD
#error HOST_Y_COORD is undefined
#endif

// Coordinates of BP-HB bridge used for manycore communication
#define BP_HOST_LINK_X 15
#define BP_HOST_LINK_Y 9
// Coordinates of BP-HB bridge used for DRAM loads and stores
#define BP_DRAM_0_LINK_X 15
#define BP_DRAM_0_LINK_Y 10
#define BP_DRAM_1_LINK_X 15
#define BP_DRAM_1_LINK_Y 11

// BlackParrot pod coordinates
#ifndef BP_POD_X
#error BP_POD_X is not defined
#endif

#ifndef BP_POD_Y
#error BP_POD_Y is not defined
#endif

// Memory mapped addresses to interact with the BlackParrot host (aka simulator)
#define HB_BP_HOST_EPA_ARGS_START 0x0000
#define HB_BP_HOST_EPA_ARGS_FINISH 0x00FF
#define HB_BP_HOST_EPA_CONFIG_START 0x0100
#define HB_BP_HOST_EPA_CONFIG_FINISH 0x01FF
#define HB_BP_HOST_EPA_RESET_DONE 0x0200
#define HB_BP_HOST_EPA_TX_VACANT 0x0300

#define HB_BP_HOST_OP_FINISH_CODE 0xFFFFFFFF
#define DROMAJO_RW_FAIL_CODE 0xFFFFFFFF

// Memory mapped addresses to interact with the manycore bridge
// BlackParrot currently sets the MSB of its address to 1 to commnicate with the manycore
// Dromajo however is setup to identify the manycore as a device
#define MC_BASE_ADDR 0x500000
#define BP_TO_MC_REQ_FIFO_ADDR 0x1000
#define BP_TO_MC_REQ_CREDITS_ADDR 0x2000
#define MC_TO_BP_RESP_FIFO_ADDR 0x3000
#define MC_TO_BP_RESP_ENTRIES_ADDR 0x4000
#define MC_TO_BP_REQ_FIFO_ADDR 0x5000
#define MC_TO_BP_REQ_ENTRIES_ADDR 0x6000

/**************************** Manycore Bridge helper functions ****************************/
// Use these functions to enable Dromajo/BlackParrot to write to the bridge. These functions
// are undefined and carry no meaning when used by the BlackParrot host (aka simulator)

/*
 * Reads the manycore bridge for number of credits used in the endpoint
 * @returns number of credits in the manycore bridge enddpoint
 */
int hb_bp_get_credits_used(int *credits_used);

/*
 * Writes a 128-bit manycore packet in 32-bit chunks to the manycore bridge FIFO
 * @param[in] pkt --> Pointer to the manycore packet
 */
int hb_bp_write_to_mc_bridge(hb_mc_packet_t *pkt);

/*
 * Checks if the MC to BP FIFO contains any entries
 * @param[in] type --> Type of FIFO to read from
 * @returns number of entries in the MC to BP FIFO
 */
int hb_bp_get_fifo_entries(int *entries, hb_mc_fifo_rx_t type);

/*
 * Reads the manycore bridge FIFOs in 32-bit chunks to form the 128-bit packet
 * @param[in] pkt --> Pointer to the manycore packet
 * @param[in] type --> Type of FIFO to read from
 * @returns HB_MC_SUCCESS on success, HB_MC_FAIL if FIFO type is unknown
 */
int hb_bp_read_from_mc_bridge(hb_mc_packet_t *pkt, hb_mc_fifo_rx_t type);

#ifdef __cplusplus
}
#endif

#endif
