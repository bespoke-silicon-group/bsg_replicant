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

#ifndef BSG_MANYCORE_MMIO
#define BSG_MANYCORE_MMIO
#include <bsg_manycore_features.h>

/* PCIe FIFOs */
#define HB_MC_MMIO_FIFO_DATA_WIDTH 32
#define HB_MC_MMIO_FIFO_NUM_BYTES 0x10


// host transmit fifo vacancy to the manycore request 0x00
#define HB_MC_MMIO_FIFO_TX_VACANCY_OFFSET 0x00

// For host sending request to manycore
#define HB_MC_MMIO_FIFO_TX_DATA_OFFSET 0x04

// host receive fifo occupancy from manycore request 0x18
#define HB_MC_MMIO_FIFO_RX_OCCUPANCY_OFFSET 0x08

// For host receiving response from manycore 0x0C
// For host receiving request from manycore 0x1C
#define HB_MC_MMIO_FIFO_RX_DATA_OFFSET 0x0C

// Out credits of the endpoint standard in mcl
#define HB_MC_MMIO_OUT_CREDITS_HOST_OFFSET 0x2000

/* AXI base address */
/* Hammerblade-Manycore ROM */
#define HB_MC_MMIO_ROM_BASE 0x0000
/* Link to AXIL Lifos */
#define HB_MC_MMIO_FIFO_BASE 0x1000

/* Flow control */
#define HB_MC_MMIO_MAX_READ_CREDITS 256

#define hb_mc_mmio_fifo_get_direction_offset(dir)       \
        (HB_MC_MMIO_FIFO_BASE + dir * HB_MC_MMIO_FIFO_NUM_BYTES)

#define hb_mc_mmio_fifo_get_addr(dir, ofs) \
        (hb_mc_mmio_fifo_get_direction_offset(dir) + ofs)

#define hb_mc_mmio_out_credits_get_addr() \
        HB_MC_MMIO_OUT_CREDITS_HOST_OFFSET

#define hb_mc_mmio_rom_get_addr(ofs) \
        (HB_MC_MMIO_ROM_BASE + ofs)

#endif
