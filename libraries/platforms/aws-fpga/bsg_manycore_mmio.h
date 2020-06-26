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
#include <bsg_manycore_platform.h>
#include <bsg_manycore_printing.h>
#include <cstdint>

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

// Low 32 bits of the manycore cycle counter
#define HB_MC_MMIO_CYCLE_CTR_LO_OFFSET 0x1FF0
// High 32 bits of the manycore cycle counter
#define HB_MC_MMIO_CYCLE_CTR_HI_OFFSET 0x1FF4

// Out credits of the endpoint standard in mcl
#define HB_MC_MMIO_OUT_CREDITS_HOST_OFFSET 0x2000

/* AXI base address */
/* Hammerblade-Manycore ROM */
#define HB_MC_MMIO_ROM_BASE 0x0000
/* Link to AXIL Lifos */
#define HB_MC_MMIO_FIFO_BASE 0x1000

/* Flow control */
#define HB_MC_MMIO_MAX_READ_CREDITS 256

#define hb_mc_mmio_fifo_get_direction_offset(dir)                       \
        (HB_MC_MMIO_FIFO_BASE + dir * HB_MC_MMIO_FIFO_NUM_BYTES)

#define hb_mc_mmio_fifo_get_addr(dir, ofs)                      \
        (hb_mc_mmio_fifo_get_direction_offset(dir) + ofs)

#define hb_mc_mmio_out_credits_get_addr()       \
        HB_MC_MMIO_OUT_CREDITS_HOST_OFFSET

#define hb_mc_mmio_rom_get_addr(ofs)            \
        (HB_MC_MMIO_ROM_BASE + ofs)


/* AWS (Physical hardware) and VCS (simulated hardware) differ
 * slightly, but provide the same interface to the platform. To hide
 * this fact, we use a union.
 * 
 * AWS Uses a memory mapped IO pointer to do native reads/writes.
 * VCS uses a DPI call that takes a handle (index)
 */
typedef union {
        uintptr_t p;
        int handle;
} hb_mc_mmio_t;

#ifdef __cplusplus
extern "C" {
#endif

        /**
         * Write data to manycore hardware at a given AXI Address
         * @param[in]  mmio   MMIO pointer initialized with hb_mc_mmio_init()
         * @param[in]  offset An offset into the manycore's MMIO address space
         * @param[in]  vp     A pointer to a value to be written out
         * @param[in]  sz     Number of bytes in the pointer to be written out
         * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
         */
        int hb_mc_mmio_write(hb_mc_mmio_t mmio, uintptr_t offset,
                                             void *vp, size_t sz);

        /**
         * Read data from manycore hardware at a given AXI Address
         * @param[in]  mmio   MMIO pointer initialized with hb_mc_mmio_init()
         * @param[in]  offset An offset into the manycore's MMIO address space
         * @param[out] vp     A pointer to read a value in to
         * @param[in]  sz     Number of bytes in the pointer to be written out
         * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
         */
        int hb_mc_mmio_read(hb_mc_mmio_t mmio, uintptr_t offset,
                                            void *vp, size_t sz);

        /**
         * Read one byte from manycore hardware at a given AXI Address
         * @param[in]  mmio   MMIO pointer initialized with hb_mc_mmio_init()
         * @param[in]  offset An offset into the manycore's MMIO address space
         * @param[out] vp     A byte to be set to the data read
         * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
         */
        static inline int hb_mc_mmio_read8(hb_mc_mmio_t mmio, uintptr_t offset, uint8_t *vp)
        {
                return hb_mc_mmio_read(mmio, offset, (void*)vp, 1);
        }

        /**
         * Read a 16-bit half-word from manycore hardware at a given AXI Address
         * @param[in]  mmio   MMIO pointer initialized with hb_mc_mmio_init()
         * @param[in]  offset An offset into the manycore's MMIO address space
         * @param[out] vp     A half-word to be set to the data read
         * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
         */
        static inline int hb_mc_mmio_read16(hb_mc_mmio_t mmio, uintptr_t offset, uint16_t *vp)
        {
                return hb_mc_mmio_read(mmio, offset, (void*)vp, 2);
        }

        /**
         * Read a 32-bit word from manycore hardware at a given AXI Address
         * @param[in]  mmio   MMIO pointer initialized with hb_mc_mmio_init()
         * @param[in]  offset An offset into the manycore's MMIO address space
         * @param[out] vp     A word to be set to the data read
         * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
         */
        static inline int hb_mc_mmio_read32(hb_mc_mmio_t mmio, uintptr_t offset, uint32_t *vp)
        {
                return hb_mc_mmio_read(mmio, offset, (void*)vp, 4);
        }

        /**
         * Write one byte to manycore hardware at a given AXI Address
         * @param[in]  mmio   MMIO pointer initialized with hb_mc_mmio_init()
         * @param[in]  offset An offset into the manycore's MMIO address space
         * @param[in]  v      A byte value to be written out
         * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
         */

        static inline int hb_mc_mmio_write8(hb_mc_mmio_t mmio, uintptr_t offset, uint8_t v)
        {
                return hb_mc_mmio_write(mmio, offset, (void*)&v, 1);
        }

        /**
         * Write a 16-bit half-word to manycore hardware at a given AXI Address
         * @param[in]  mmio   MMIO pointer initialized with hb_mc_mmio_init()
         * @param[in]  offset An offset into the manycore's MMIO address space
         * @param[in]  v      A half-word value to be written out
         * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
         */

        static inline int hb_mc_mmio_write16(hb_mc_mmio_t mmio, uintptr_t offset, uint16_t v)
        {
                return hb_mc_mmio_write(mmio, offset, (void*)&v, 2);
        }

        /**
         * Write a 32-bit word to manycore hardware at a given AXI Address
         * @param[in]  mmio   MMIO pointer initialized with hb_mc_mmio_init()
         * @param[in]  offset An offset into the manycore's MMIO address space
         * @param[in]  v      A word value to be written out
         * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
         */
        static inline int hb_mc_mmio_write32(hb_mc_mmio_t mmio, uintptr_t offset, uint32_t v)
        {
                return hb_mc_mmio_write(mmio, offset, (void*)&v, 4);
        }

        /**
         * Initialize MMIO for operation
         * @param[in]  mmio   MMIO pointer to initialize
         * @param[in]  handle PCI BAR handle to map
         * @param[in]  id     ID which selects the physical hardware from which this manycore is configured
         * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
         */
        int hb_mc_mmio_init(hb_mc_mmio_t *mmio, int* handle, hb_mc_manycore_id_t id);

        /**
         * Clean up MMIO for termination
         * @param[in]  mmio   MMIO pointer to clean up
         * @param[in]  handle PCI BAR handle to unmap
         * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
         */
        int hb_mc_mmio_cleanup(hb_mc_mmio_t *mmio, int *handle);
                                      
/* these are convenience macros that are only good for one line prints */
#define mmio_pr_dbg(m, fmt, ...)                    \
        bsg_pr_dbg("%p: " fmt, m.p, ##__VA_ARGS__)

#define mmio_pr_err(m, fmt, ...)                    \
        bsg_pr_err("%p: " fmt, m.p, ##__VA_ARGS__)

#define mmio_pr_warn(m, fmt, ...)                   \
        bsg_pr_warn("%p: " fmt, m.p, ##__VA_ARGS__)

#define mmio_pr_info(m, fmt, ...)                   \
        bsg_pr_info("%p: " fmt, m.p, ##__VA_ARGS__)
}
#endif
