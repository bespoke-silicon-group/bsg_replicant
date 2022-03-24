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
#include <bsg_manycore_mmio.h>
#include <fpga_pci.h>

/**
 * Initialize MMIO for operation
 * @param[in]  mmio   MMIO pointer to initialize
 * @param[in]  handle PCI BAR handle to map
 * @param[in]  id     ID which selects the physical hardware from which this manycore is configured
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_mmio_init(hb_mc_mmio_t *mmio,
                           pci_bar_handle_t* handle,
                           hb_mc_manycore_id_t id)
{
        int pf_id = FPGA_APP_PF, write_combine = 0, bar_id = APP_PF_BAR0;
        int r = HB_MC_FAIL, err;

        // all IDs except 0 are unused at the moment
        if (id != 0) {
                mmio_pr_err((*mmio), "Failed to init MMIO: invalid ID\n");
                return HB_MC_INVALID;
        }

        if ((err = fpga_pci_attach(id, pf_id, bar_id, write_combine, handle)) != 0) {
                mmio_pr_err((*mmio), "Failed to init MMIO: %s\n", FPGA_ERR2STR(err));
                mmio_pr_err((*mmio), "Are you running with sudo?\n");
                return r;
        }

        // it is not clear to me where 0x4000 comes from...
        // map in the base address register to our address space
        if ((err = fpga_pci_get_address(*handle, 0, 0x4000, (void**)&mmio->p)) != 0) {
                mmio_pr_err((*mmio), "Failed to init MMIO: %s\n", FPGA_ERR2STR(err));
                goto cleanup;
        }
        r = HB_MC_SUCCESS;
        mmio_pr_dbg(mmio, "%s: mmio = 0x%" PRIxPTR "\n", __func__, mmio->p);
        goto done;

 cleanup:
        fpga_pci_detach(*handle);
        *handle = PCI_BAR_HANDLE_INIT;
 done:
        return r;
}

/**
 * Clean up MMIO for termination
 * @param[in]  mmio   MMIO pointer to clean up
 * @param[in]  handle PCI BAR handle to unmap
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_mmio_cleanup(hb_mc_mmio_t *mmio,
                              pci_bar_handle_t *handle)
{
        int err;

        if (*handle == PCI_BAR_HANDLE_INIT)
                return HB_MC_SUCCESS;

        if ((err = fpga_pci_detach(*handle)) != 0)
                mmio_pr_err((*mmio), "Failed to cleanup MMIO: %s\n", FPGA_ERR2STR(err));

        *handle = PCI_BAR_HANDLE_INIT;
        (*mmio).p = reinterpret_cast<uintptr_t>(nullptr);
        return HB_MC_SUCCESS;
}

/**
 * Write data to manycore hardware at a given AXI Address
 * @param[in]  mmio     An MMIO pointer instance initialized with hb_mc_mmio_init()
 * @param[in]  offset An offset into the manycore's MMIO address space
 * @param[in]  vp     A pointer to a value to be written out
 * @param[in]  sz     Number of bytes in the pointer to be written out
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_mmio_read(hb_mc_mmio_t mmio, uintptr_t offset,
                           void *vp, size_t sz)
{
        unsigned char *addr = reinterpret_cast<unsigned char *>(mmio.p);
        uint32_t tmp;

        if (addr == nullptr) {
                mmio_pr_err((mmio), "%s: Failed: MMIO not initialized", __func__);
                return HB_MC_UNINITIALIZED;
        }

        // check that the address is aligned to a four byte boundar
        if (offset % 4) {
                mmio_pr_err((mmio), "%s: Failed: 0x%" PRIxPTR " "
                            "is not aligned to 4 byte boundary\n",
                            __func__, offset);
                return HB_MC_UNALIGNED;
        }

        addr = &addr[offset];

        tmp = *(volatile uint32_t *)addr;

        switch (sz) {
        case 4:
                *(uint32_t*)vp = tmp;
                break;
        case 2:
                *(uint16_t*)vp = tmp;
                break;
        case 1:
                *(uint8_t*)vp  = tmp;
                break;
        default:
                mmio_pr_err((mmio), "%s: Failed: invalid load size (%zu)\n", __func__, sz);
                return HB_MC_INVALID;
        }

        return HB_MC_SUCCESS;
}

/**
 * Write data to manycore hardware at a given AXI Address
 * @param[in]  mmio     An MMIO pointer instance initialized with hb_mc_mmio_init()
 * @param[in]  offset An offset into the manycore's MMIO address space
 * @param[in]  vp     A pointer to a value to be written out
 * @param[in]  sz     Number of bytes in the pointer to be written out
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_mmio_write(hb_mc_mmio_t mmio, uintptr_t offset,
                            void *vp, size_t sz)
{
        unsigned char *addr = reinterpret_cast<unsigned char *>(mmio.p);
        uint32_t tmp;

        if (addr == nullptr) {
                mmio_pr_err((mmio), "%s: Failed: MMIO not initialized", __func__);
                return HB_MC_UNINITIALIZED;
        }

        // check that the address is aligned to a four byte boundary
        if (offset % 4) {
                mmio_pr_err((mmio), "%s: Failed: 0x%" PRIxPTR " "
                            "is not aligned to 4 byte boundary\n",
                            __func__, offset);
                return HB_MC_UNALIGNED;
        }

        addr = &addr[offset];

        switch (sz) {
        case 4:
                tmp = *(uint32_t *)vp;
                break;
        case 2:
                tmp = *(uint16_t*)vp;
                break;
        case 1:
                tmp = *(uint8_t*)vp;
                break;
        default:
                mmio_pr_err((mmio), "%s: Failed: invalid load size (%zu)\n", __func__, sz);
                return HB_MC_INVALID;
        }

        *(volatile uint32_t *)addr = tmp;

        return HB_MC_SUCCESS;
}

/**
 * Signal the hardware to start a bulk transfer over the network
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_start_bulk_transfer(hb_mc_manycore_t *mc){
        return HB_MC_SUCCESS;
}

/**
 * Signal the hardware to end a bulk transfer over the network
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_finish_bulk_transfer(hb_mc_manycore_t *mc){
        return HB_MC_SUCCESS;
}
