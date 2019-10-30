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
// From https://www.xilinx.com/support/documentation/ip_documentation/axi_fifo_mm_s/v4_1/pg080-axi-fifo-mm-s.pdf
#define HB_MC_MMIO_FIFO_TX_VACANCY_OFFSET 0xC
#define HB_MC_MMIO_FIFO_TX_DATA_OFFSET 0x10
// #define HB_MC_MMIO_FIFO_TX_LENGTH_OFFSET 0x14
#define HB_MC_MMIO_FIFO_RX_OCCUPANCY_OFFSET 0x1C
#define HB_MC_MMIO_FIFO_RX_DATA_OFFSET 0x20
#define HB_MC_MMIO_FIFO_RX_LENGTH_OFFSET 0x24
// #define HB_MC_MMIO_FIFO_ISR_OFFSET 0x0 
// #define HB_MC_MMIO_FIFO_IER_OFFSET 0x4

// #define HB_MC_MMIO_FIFO_IXR_RFPE_BIT 19
// #define HB_MC_MMIO_FIFO_IXR_RFPF_BIT 20
// #define HB_MC_MMIO_FIFO_IXR_TFPE_BIT 21
// #define HB_MC_MMIO_FIFO_IXR_TFPF_BIT 22
// #define HB_MC_MMIO_FIFO_IXR_RRC_BIT 23
// #define HB_MC_MMIO_FIFO_IXR_TRC_BIT 24
// #define HB_MC_MMIO_FIFO_IXR_TSE_BIT 25
// #define HB_MC_MMIO_FIFO_IXR_RC_BIT 26
// #define HB_MC_MMIO_FIFO_IXR_TC_BIT 27
// #define HB_MC_MMIO_FIFO_IXR_TPOE_BIT 28
// #define HB_MC_MMIO_FIFO_IXR_RPUE_BIT 29
// #define HB_MC_MMIO_FIFO_IXR_RPORE_BIT 30
// #define HB_MC_MMIO_FIFO_IXR_RPURE_BIT 31

#define HB_MC_MMIO_FIFO_DATA_WIDTH 32
#define HB_MC_MMIO_FIFO_BASE 0x1000
#define HB_MC_MMIO_FIFO_NUM_BYTES 0x1000

#define hb_mc_mmio_fifo_get_direction_offset(dir)       \
        (HB_MC_MMIO_FIFO_BASE + dir * HB_MC_MMIO_FIFO_NUM_BYTES)

#define hb_mc_mmio_fifo_get_reg_addr(dir, reg)                  \
        (hb_mc_mmio_fifo_get_direction_offset(dir) + reg)

/* Hammerblade-Manycore ROM */
#define HB_MC_MMIO_ROM_BASE 0x0000

/* Flow control */ 
#define HB_MC_MMIO_CREDITS_BASE 0x1000
#define HB_MC_MMIO_MAX_CREDITS 16

#define HB_MC_MMIO_CREDITS_FIFO_HOST_VACANCY_OFFSET 0x002C
#define HB_MC_MMIO_CREDITS_FIFO_DEVICE_VACANCY_OFFSET 0x102C
#define HB_MC_MMIO_CREDITS_HOST_OFFSET 0x0030

#define hb_mc_mmio_credits_get_reg_addr(reg)    \
        (HB_MC_MMIO_CREDITS_BASE + reg)


#endif
