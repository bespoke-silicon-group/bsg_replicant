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

#include <bsg_manycore.h>
#include <bsg_manycore_fifo.h>
#include <bsg_manycore_mmio.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_tile.h>
#include <bsg_manycore_responder.h>
#include <bsg_manycore_epa.h>
#include <bsg_manycore_vcache.h>

#ifndef COSIM
#include <fpga_pci.h>
#include <fpga_mgmt.h>
#else
#include <fpga_pci_sv.h>
#include <utils/sh_dpi_tasks.h>
#if defined(USING_DRAMSIM3)
#include <bsg_test_dram_channel.hpp>
#define DRAM_HACK_WRITE
#define DRAM_HACK_READ
#endif

#endif

#ifdef __cplusplus
#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <climits>
#include <cstdbool>
#include <cassert>
#else
#include <inttypes>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>
#include <assert.h>
#endif

#include <type_traits>
#include <stack>
#include <queue>
#include <vector>

#define array_size(x)                           \
        (sizeof(x)/sizeof(x[0]))

#define sarray_size(x)                          \
        ((ssize_t)array_size(x))

/* these are conveniance macros that are only good for one line prints */
#define manycore_pr_dbg(mc, fmt, ...)                   \
        bsg_pr_dbg("%s: " fmt, mc->name, ##__VA_ARGS__)

#define manycore_pr_err(mc, fmt, ...)                   \
        bsg_pr_err("%s: " fmt, mc->name, ##__VA_ARGS__)

#define manycore_pr_warn(mc, fmt, ...)                          \
        bsg_pr_warn("%s: " fmt, mc->name, ##__VA_ARGS__)

#define manycore_pr_info(mc, fmt, ...)                          \
        bsg_pr_info("%s: " fmt, mc->name, ##__VA_ARGS__)


// #undef manycore_pr_err
// #define manycore_pr_err(...)

typedef struct hb_mc_manycore_private {
        pci_bar_handle_t handle;
} hb_mc_manycore_private_t;


static int  hb_mc_manycore_mmio_read_mmio(hb_mc_manycore_t *mc, uintptr_t offset,
                                          void *vo, size_t sz);

static int  hb_mc_manycore_mmio_read_pci(hb_mc_manycore_t *mc, uintptr_t offset,
                                         void *vp, size_t sz);
static int hb_mc_manycore_mmio_read(hb_mc_manycore_t *mc, uintptr_t offset,
                                    void *vp, size_t sz);

static int  hb_mc_manycore_init_mmio(hb_mc_manycore_t *mc, hb_mc_manycore_id_t id);
static void hb_mc_manycore_cleanup_mmio(hb_mc_manycore_t *mc);
static int  hb_mc_manycore_init_private_data(hb_mc_manycore_t *mc);
static void hb_mc_manycore_cleanup_private_data(hb_mc_manycore_t *mc);

static int hb_mc_manycore_packet_tx_internal(hb_mc_manycore_t *mc,
                                             hb_mc_packet_t *packet,
                                             hb_mc_fifo_tx_t type,
                                             long timeout);

static int hb_mc_manycore_packet_rx_internal(hb_mc_manycore_t *mc,
                                             hb_mc_packet_t *packet,
                                             hb_mc_fifo_rx_t type,
                                             long timeout);
///////////////////////////
// FIFO Helper Functions //
///////////////////////////

/* get the number of unread packets in a FIFO (rx only) */
static int hb_mc_manycore_rx_fifo_get_occupancy(hb_mc_manycore_t *mc,
                                                hb_mc_fifo_rx_t type,
                                                uint32_t *occupancy)
{
        uint32_t val, occ;
        uintptr_t occupancy_addr = hb_mc_mmio_fifo_get_addr(type, HB_MC_MMIO_FIFO_RX_OCCUPANCY_OFFSET);
        const char *typestr = hb_mc_fifo_rx_to_string(type);
        int err;
        if (type == HB_MC_FIFO_RX_RSP) {
            manycore_pr_err(mc, "RX FIFO Occupancy register %s is disabled!\n", typestr);
            occupancy = NULL;
            return HB_MC_FAIL;
        }
        else {
            err = hb_mc_manycore_mmio_read32(mc, occupancy_addr, &val);
            if (err != HB_MC_SUCCESS) {
                    manycore_pr_err(mc, "Failed to get %s occupancy\n", typestr);
                    return err;
            }

            // All packets recieved packets should have an integral occupancy that
            // is determined by the Packet Bit-Width / FIFO Bit-Width
            occ = ((sizeof(hb_mc_packet_t) * 8))/HB_MC_MMIO_FIFO_DATA_WIDTH;
            if((occ < ((sizeof(hb_mc_packet_t) * 8))/HB_MC_MMIO_FIFO_DATA_WIDTH) && (val % occ != 0)) {
                    manycore_pr_err(mc, "Invalid occupancy: Non-integral packet"
                                    " received from %s\n", typestr);
                    return HB_MC_FAIL;
            }

            *occupancy = (val / occ);
            return HB_MC_SUCCESS;
        }

}

/* get the number of remaining packets in a host tx FIFO */
static int hb_mc_manycore_tx_fifo_get_vacancy(hb_mc_manycore_t *mc,
                                              hb_mc_fifo_tx_t type,
                                              uint32_t *vacancy)
{
        uint32_t val, vac;
        uintptr_t vacancy_addr = hb_mc_mmio_fifo_get_addr(type, HB_MC_MMIO_FIFO_TX_VACANCY_OFFSET);
        const char *typestr = hb_mc_fifo_tx_to_string(type);
        int err;
        if (type == HB_MC_FIFO_TX_RSP) {
            manycore_pr_err(mc, "TX FIFO Vacancy register %s is disabled!\n", typestr);
            vacancy = NULL;
            return HB_MC_FAIL;
        }
        else {
            err = hb_mc_manycore_mmio_read32(mc, vacancy_addr, &val);
            if (err != HB_MC_SUCCESS) {
                    manycore_pr_err(mc, "Failed to get %s vacancy\n", typestr);
                    return err;
            }

            // All packets recieved packets should have an integral vacancy that
            // is determined by the Packet Bit-Width / FIFO Bit-Width
            vac = ((sizeof(hb_mc_packet_t) * 8))/HB_MC_MMIO_FIFO_DATA_WIDTH;
            if((vac < ((sizeof(hb_mc_packet_t) * 8))/HB_MC_MMIO_FIFO_DATA_WIDTH) && (val % vac != 0)) {
                    manycore_pr_err(mc, "Invalid vacancy: Non-integral packet"
                                    " received from %s\n", typestr);
                    return HB_MC_FAIL;
            }
            *vacancy = (val / vac);
            return HB_MC_SUCCESS;
        }

}

/* read all unread packets from a fifo (rx only) */
static int hb_mc_manycore_rx_fifo_drain(hb_mc_manycore_t *mc, hb_mc_fifo_rx_t type)
{
        const char *typestr = hb_mc_fifo_rx_to_string(type);
        hb_mc_request_packet_t recv;
        uint32_t occupancy;
        int rc;

        for (int drains = 0; drains < 20; drains++) {
                /* first check how many unread packets are currently in the FIFO */
                rc = hb_mc_manycore_rx_fifo_get_occupancy(mc, type, &occupancy);
                if (rc != HB_MC_SUCCESS)
                        return rc;

                /* break if occupancy is zero */
                if (occupancy == 0)
                        break;

                /* Read stale packets from fifo */
                for (unsigned i = 0; i < occupancy; i++){
                        rc = hb_mc_manycore_packet_rx_internal(mc, (hb_mc_packet_t*) &recv, type, -1);
                        if (rc != HB_MC_SUCCESS) {
                                manycore_pr_err(mc, "%s: Failed to read packet from %s fifo\n",
                                                __func__, typestr);
                                return HB_MC_FAIL;
                        }

                        manycore_pr_dbg(mc,
                                        "%s: packet drained from %s fifo: "
                                        "src (%d,%d), "
                                        "dst (%d,%d), "
                                        "addr: 0x%08x, "
                                        "data: 0x%08x\n",
                                        __func__, typestr,
                                        recv.x_src, recv.y_src,
                                        recv.x_dst, recv.y_dst,
                                        recv.addr,
                                        recv.data);
                }
        }

        /* recheck occupancy to make sure all packets are drained. */
        rc = hb_mc_manycore_rx_fifo_get_occupancy(mc, type, &occupancy);
        if (rc != HB_MC_SUCCESS)
                return HB_MC_FAIL;

        /* fail if new packets have arrived */
        if (occupancy > 0){
                manycore_pr_err(mc, "%s: Failed to drain %s fifo: new packets generated\n",
                                __func__, type);
                return HB_MC_FAIL;
        }

        return HB_MC_SUCCESS;
}


///////////////////
// Init/Exit API //
///////////////////

/* initialize manycore MMIO */
static int hb_mc_manycore_init_mmio(hb_mc_manycore_t *mc, hb_mc_manycore_id_t id)
{
        hb_mc_manycore_private_t *pdata = (hb_mc_manycore_private_t*)mc->private_data;
        int pf_id = FPGA_APP_PF, write_combine = 0, bar_id = APP_PF_BAR0;
        int r = HB_MC_FAIL, err;

        // all IDs except 0 are unused at the moment
        if (id != 0) {
                manycore_pr_err(mc, "Failed to init MMIO: invalid ID\n");
                return HB_MC_INVALID;
        }

        if ((err = fpga_pci_attach(id, pf_id, bar_id, write_combine, &pdata->handle)) != 0) {
                manycore_pr_err(mc, "Failed to init MMIO: %s\n", FPGA_ERR2STR(err));
                manycore_pr_err(mc, "Are you running with sudo?\n");
                return r;
        }

#if !defined(COSIM) // on F1
        // it is not clear to me where 0x4000 comes from...
        // map in the base address register to our address space
        if ((err = fpga_pci_get_address(pdata->handle, 0, 0x4000, (void**)&mc->mmio)) != 0) {
                manycore_pr_err(mc, "Failed to init MMIO: %s\n", FPGA_ERR2STR(err));
                goto cleanup;
        }
#else
        mc->mmio = (uintptr_t)nullptr;
#endif
        mc->id = id;
        r = HB_MC_SUCCESS;
        manycore_pr_dbg(mc, "%s: mc->mmio = 0x%" PRIxPTR "\n", __func__, mc->mmio);
        goto done;

 cleanup:
        fpga_pci_detach(pdata->handle);
        pdata->handle = PCI_BAR_HANDLE_INIT;
 done:
        return r;
}

/* cleanup manycore MMIO */
static void hb_mc_manycore_cleanup_mmio(hb_mc_manycore_t *mc)
{
        hb_mc_manycore_private_t *pdata = (hb_mc_manycore_private_t*)mc->private_data;
        int err;

        if (pdata->handle == PCI_BAR_HANDLE_INIT)
                return;

        if ((err = fpga_pci_detach(pdata->handle)) != 0)
                manycore_pr_err(mc, "Failed to cleanup MMIO: %s\n", FPGA_ERR2STR(err));

        pdata->handle = PCI_BAR_HANDLE_INIT;
        mc->mmio = (uintptr_t)nullptr;
        mc->id = 0;
        return;
}

/* initialize manycore private data */
static int hb_mc_manycore_init_private_data(hb_mc_manycore_t *mc)
{
        int r = HB_MC_FAIL, err;
        hb_mc_manycore_private_t *pdata;

        mc->private_data = nullptr;

        pdata = (hb_mc_manycore_private_t*)calloc(sizeof(*pdata), 1);
        if (!pdata) {
                manycore_pr_err(mc, "%s failed: %m\n", __func__);
                return HB_MC_NOMEM;
        }

        pdata->handle = PCI_BAR_HANDLE_INIT;
        mc->private_data = pdata;

        return HB_MC_SUCCESS;
}

/* cleanup manycore private data */
static void hb_mc_manycore_cleanup_private_data(hb_mc_manycore_t *mc)
{
        free(mc->private_data);
}

/* initialize configuration */
static int hb_mc_manycore_init_config(hb_mc_manycore_t *mc)
{
        int err;
        uintptr_t addr;
        int idx;
        hb_mc_config_raw_t config[HB_MC_CONFIG_MAX];

        for (idx = HB_MC_CONFIG_MIN; idx < HB_MC_CONFIG_MAX; idx++) {
                addr = hb_mc_config_id_to_addr(HB_MC_MMIO_ROM_BASE,
                                               (hb_mc_config_id_t) idx);

                err = hb_mc_manycore_mmio_read32(mc, addr, &config[idx]);
                if (err != HB_MC_SUCCESS) {
                        manycore_pr_err(mc, "%s: Failed to read config word %d from ROM\n",
                                        __func__, idx);
                        return err;
                }
        }

        err = hb_mc_config_init(config, &(mc->config));
        if (err != HB_MC_SUCCESS) {
                manycore_pr_err(mc, "%s: Failed to initialize configuration"
                                " fields of manycore struct\n",
                                __func__);
                return err;
        }

        manycore_pr_dbg(mc, "Initialized configuration from ROM\n");

        return HB_MC_SUCCESS;
}


/////////////////////////////////
/* Flow Control Help Functions */
/////////////////////////////////

static int hb_mc_manycore_get_remote_load_cap(hb_mc_manycore_t *mc, unsigned *cap)
{
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        *cap = hb_mc_config_get_io_remote_load_cap(cfg);
        return HB_MC_SUCCESS;
}

static int hb_mc_manycore_incr_host_requests(hb_mc_manycore_t*mc)
{

        mc->htod_requests++;
        return HB_MC_SUCCESS;
}

static int hb_mc_manycore_decr_host_requests(hb_mc_manycore_t *mc)
{
        if (mc->htod_requests == 0) {
                manycore_pr_err(mc, "%s: No outstanding requests!\n", __func__);
                return HB_MC_FAIL;
        }

        mc->htod_requests--;
        return HB_MC_SUCCESS;
}

static int hb_mc_manycore_host_request_fence(hb_mc_manycore_t *mc)
{
    unsigned cap;
    unsigned max_credits;

    uint32_t vacancy;
    int ep_out_credits;
    int err;

    const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);

    cap = hb_mc_config_get_io_host_credits_cap(cfg);
    max_credits = hb_mc_config_get_io_endpoint_max_out_credits(cfg);

    // wait until out credts are fully resumed, and the tx fifo vacancy equals to host credits
    while ((vacancy != cap) | (ep_out_credits != max_credits)) {
        err = hb_mc_manycore_tx_fifo_get_vacancy(mc, HB_MC_FIFO_TX_REQ, &vacancy);
        if (err != HB_MC_SUCCESS)
                return err;
        ep_out_credits = hb_mc_manycore_get_endpoint_out_credits(mc);
    }

    return HB_MC_SUCCESS;
}

static int hb_mc_manycore_update_requests(hb_mc_manycore_t *mc)
{
    unsigned threshold, cap;
    uint32_t vacancy;
    int err;

    const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);

    threshold = hb_mc_config_get_io_credit_update_threshold(cfg);
    cap = hb_mc_config_get_io_host_credits_cap(cfg);

    while (mc->htod_requests >= threshold) {
        err = hb_mc_manycore_tx_fifo_get_vacancy(mc, HB_MC_FIFO_TX_REQ, &vacancy);
        if (err != HB_MC_SUCCESS)
                return err;
        mc->htod_requests = (cap - vacancy);
    }

    return HB_MC_SUCCESS;
}

static int hb_mc_manycore_host_requests_init(hb_mc_manycore_t *mc)
{
        mc->htod_requests = 0;
        return HB_MC_SUCCESS;
}



static int hb_mc_manycore_init_fifos(hb_mc_manycore_t *mc)
{
        int rc;

        /* Drain the Manycore-To-Host (RX) Request FIFO */
        rc = hb_mc_manycore_rx_fifo_drain(mc, HB_MC_FIFO_RX_REQ);
        if (rc != HB_MC_SUCCESS)
                return rc;

        /* initialize the outstanding request counter */
        rc = hb_mc_manycore_host_requests_init(mc);
        if (rc != HB_MC_SUCCESS)
                return rc;

        return HB_MC_SUCCESS;
}

static void hb_mc_manycore_cleanup_fifos(hb_mc_manycore_t *mc)
{
        /* Drain the Manycore-To-Host (RX) Request FIFO */
        hb_mc_manycore_rx_fifo_drain(mc, HB_MC_FIFO_RX_REQ);
}

/**
 * Initialize a manycore instance
 * @param[in] mc    A manycore to initialize
 * @param[in] name  A name to give this manycore instance (used for debugging)
 * @param[in] id    ID which selects the physical hardware from which this manycore is configured
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int  hb_mc_manycore_init(hb_mc_manycore_t *mc, const char *name, hb_mc_manycore_id_t id)
{
        int r = HB_MC_FAIL, err;

        // check if null
        if (!mc || !name)
                return HB_MC_INVALID;

        // check if mc is already initialized
        if (mc->name || mc->private_data)
                return HB_MC_INITIALIZED_TWICE;

        // copy name
        mc->name = strdup(name);
        if (!mc->name) {
                bsg_pr_err("Failed to initialize %s: %m\n", name);
                return r;
        }

        // initialize private data
        if ((err = hb_mc_manycore_init_private_data(mc)) != HB_MC_SUCCESS)
                goto cleanup;

        // initialize manycore for MMIO
        if ((err = hb_mc_manycore_init_mmio(mc, id)) != HB_MC_SUCCESS)
                goto cleanup;

        // read configuration
        if ((err = hb_mc_manycore_init_config(mc)) != HB_MC_SUCCESS)
                goto cleanup;

        // initialize FIFOs
        if ((err = hb_mc_manycore_init_fifos(mc)) != HB_MC_SUCCESS)
                goto cleanup;

        // initialize responders
        if ((err = hb_mc_responders_init(mc)))
                goto cleanup;

        // enable dram
        if ((err = hb_mc_manycore_enable_dram(mc)) != HB_MC_SUCCESS)
                goto cleanup;

        r = HB_MC_SUCCESS;
        goto done;

 cleanup:
        r = err;
        hb_mc_manycore_cleanup_fifos(mc);
        hb_mc_manycore_cleanup_mmio(mc);
        hb_mc_manycore_cleanup_private_data(mc);
        free((void*)mc->name);

 done:
        return r;
}

/**
 * Cleanup an initialized manycore instance
 * @param[in] mc   A manycore instance that has been initialized with hb_mc_manycore_init()
 */
int hb_mc_manycore_exit(hb_mc_manycore_t *mc)
{
        int err;
        err = hb_mc_responders_quit(mc);
        if (err != HB_MC_SUCCESS) {
                bsg_pr_err("%s: failed to cleanup responders: %s\n",
                           __func__, hb_mc_strerror(err));
                return err;
        }
        hb_mc_manycore_cleanup_fifos(mc);
        hb_mc_manycore_cleanup_mmio(mc);
        hb_mc_manycore_cleanup_private_data(mc);
        free((void*)mc->name);
        return HB_MC_SUCCESS;
}

/************/
/* MMIO API */
/************/
/**
 * Reads data for MMIO by actualling doing loads
 */
static int  hb_mc_manycore_mmio_read_mmio(hb_mc_manycore_t *mc, uintptr_t offset,
                                          void *vp, size_t sz)
{
        unsigned char *addr = (unsigned char *)mc->mmio;
        uint32_t tmp;

        if (addr == nullptr) {
                manycore_pr_err(mc, "%s: Failed: MMIO not initialized", __func__);
                return HB_MC_UNINITIALIZED;
        }

        // check that the address is aligned to a four byte boundar
        if (offset % 4) {
                manycore_pr_err(mc, "%s: Failed: 0x%" PRIxPTR " "
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
                manycore_pr_err(mc, "%s: Failed: invalid load size (%zu)\n", __func__, sz);
                return HB_MC_INVALID;
        }

        return HB_MC_SUCCESS;
}
/**
 * Reads data for MMIO instead by using PCI ops (used in COSIM)
 */
static int  hb_mc_manycore_mmio_read_pci(hb_mc_manycore_t *mc, uintptr_t offset,
                                         void *vp, size_t sz)
{
        hb_mc_manycore_private_t *pdata = (hb_mc_manycore_private_t*)mc->private_data;
        uint32_t val;
        int err;

        if ((err = fpga_pci_peek(pdata->handle, offset, &val)) != 0) {
                manycore_pr_err(mc, "%s: Failed: %s\n", __func__, FPGA_ERR2STR(err));
                return HB_MC_FAIL;
        }

        switch (sz) {
        case 4:
                *(uint32_t*)vp = val;
                break;
        case 2:
                *(uint16_t*)vp = val;
                break;
        case 1:
                *(uint8_t *)vp = val;
                break;
        default:
                manycore_pr_err(mc, "%s: Failed: invalid load size (%zu)\n", __func__, sz);
                return HB_MC_INVALID;
        }
        return HB_MC_SUCCESS;
}

/**
 * Read the number of remaining available endpoint out credits
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_FAIL if an error occured. Number of remaining endpoint out credits otherwise
 */
int hb_mc_manycore_get_endpoint_out_credits(hb_mc_manycore_t *mc)
{
        uint64_t addr;
        uint32_t value;
        int err;
        addr = hb_mc_mmio_out_credits_get_addr();
        err = hb_mc_manycore_mmio_read32(mc, addr, &value);
        if (err != HB_MC_SUCCESS) {
                manycore_pr_err(mc, "%s: Failed to read endpoint out credits: %s\n",
                                __func__, hb_mc_strerror(err));
                return err;
        }
        return value;
}

/**
 * Read the number of remaining credits of host
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_FAIL if an error occured. Number of remaining credits of host otherwise
 */
int hb_mc_manycore_get_host_request_credits(hb_mc_manycore_t *mc)
{
        uint32_t value;
        int err;
        err = hb_mc_manycore_tx_fifo_get_vacancy(mc, HB_MC_FIFO_TX_REQ, &value);
        if (err != HB_MC_SUCCESS) {
                manycore_pr_err(mc, "%s: Failed to read Host Credits: %s\n",
                                __func__, hb_mc_strerror(err));
                return err;
        }
        return value;
}


static int hb_mc_manycore_mmio_read(hb_mc_manycore_t *mc, uintptr_t offset,
                                    void *vp, size_t sz)
{
#if !defined(COSIM)
        return hb_mc_manycore_mmio_read_mmio(mc, offset, vp, sz);
#else
        return hb_mc_manycore_mmio_read_pci(mc,  offset, vp, sz);
#endif
}

/**
 * Writes data for MMIO by direct stores
 */
static int hb_mc_manycore_mmio_write_mmio(hb_mc_manycore_t *mc, uintptr_t offset,
                                          void *vp, size_t sz)
{
        unsigned char *addr = (unsigned char *)mc->mmio;
        uint32_t tmp;

        if (addr == nullptr) {
                manycore_pr_err(mc, "%s: Failed: MMIO not initialized", __func__);
                return HB_MC_UNINITIALIZED;
        }

        // check that the address is aligned to a four byte boundary
        if (offset % 4) {
                manycore_pr_err(mc, "%s: Failed: 0x%" PRIxPTR " "
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
                manycore_pr_err(mc, "%s: Failed: invalid load size (%zu)\n", __func__, sz);
                return HB_MC_INVALID;
        }

        *(volatile uint32_t *)addr = tmp;

        return HB_MC_SUCCESS;
}

/**
 * Writes data for MMIO instead by  PCI ops (used in COSIM)
 */
static int hb_mc_manycore_mmio_write_pci(hb_mc_manycore_t *mc, uintptr_t offset,
                                         void *vp, size_t sz)
{
        hb_mc_manycore_private_t *pdata = (hb_mc_manycore_private_t*)mc->private_data;
        uint32_t val;
        int err;

        switch (sz) {
        case 4:
                val = *(uint32_t*)vp;
                break;
        case 2:
                val = *(uint16_t*)vp;
                break;
        case 1:
                val = *(uint8_t*)vp;
                break;
        default:
                manycore_pr_err(mc, "%s: Failed: invalid store size (%zu)\n", __func__, sz);
                return HB_MC_INVALID;
        }

        err = fpga_pci_poke(pdata->handle, offset, val);
        if (err != 0) {
                manycore_pr_err(mc, "%s: Failed: %s\n", __func__, FPGA_ERR2STR(err));
                return HB_MC_FAIL;
        }
        return HB_MC_SUCCESS;
}

static int hb_mc_manycore_mmio_write(hb_mc_manycore_t *mc, uintptr_t offset,
                                     void *vp, size_t sz)
{
#if !defined(COSIM)
        return hb_mc_manycore_mmio_write_mmio(mc, offset, vp, sz);
#else
        return hb_mc_manycore_mmio_write_pci(mc, offset, vp, sz);
#endif
}
/**
 * Read one byte from manycore hardware at a given AXI Address
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  offset An  offset into the manycore's MMIO address space
 * @param[out] vp     A byte to be set to the data read
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */

int hb_mc_manycore_mmio_read8(hb_mc_manycore_t *mc, uintptr_t offset, uint8_t *vp)
{
        return hb_mc_manycore_mmio_read(mc, offset, (void*)vp, 1);
}

/**
 * Read a 16-bit half-word from manycore hardware at a given AXI Address
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  offset An  offset into the manycore's MMIO address space
 * @param[out] vp     A half-word to be set to the data read
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */

int hb_mc_manycore_mmio_read16(hb_mc_manycore_t *mc, uintptr_t offset, uint16_t *vp)
{
        return hb_mc_manycore_mmio_read(mc, offset, (void*)vp, 2);
}

/**
 * Read a 32-bit word from manycore hardware at a given AXI Address
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  offset An  offset into the manycore's MMIO address space
 * @param[out] vp     A word to be set to the data read
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */

int hb_mc_manycore_mmio_read32(hb_mc_manycore_t *mc, uintptr_t offset, uint32_t *vp)
{
        return hb_mc_manycore_mmio_read(mc, offset, (void*)vp, 4);
}

/**
 * Write one byte to manycore hardware at a given AXI Address
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  offset An  offset into the manycore's MMIO address space
 * @param[in]  v      A byte value to be written out
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */

int hb_mc_manycore_mmio_write8(hb_mc_manycore_t *mc, uintptr_t offset, uint8_t v)
{
        return hb_mc_manycore_mmio_write(mc, offset, (void*)&v, 1);
}

/**
 * Write a 16-bit half-word to manycore hardware at a given AXI Address
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  offset An  offset into the manycore's MMIO address space
 * @param[in]  v      A half-word value to be written out
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */

int hb_mc_manycore_mmio_write16(hb_mc_manycore_t *mc, uintptr_t offset, uint16_t v)
{
        return hb_mc_manycore_mmio_write(mc, offset, (void*)&v, 2);
}

/**
 * Write a 32-bit word to manycore hardware at a given AXI Address
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  offset An  offset into the manycore's MMIO address space
 * @param[in]  v      A word value to be written out
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */

int hb_mc_manycore_mmio_write32(hb_mc_manycore_t *mc, uintptr_t offset, uint32_t v)
{
        return hb_mc_manycore_mmio_write(mc, offset, (void*)&v, 4);
}



////////////////
// Packet API //
////////////////



/**
 * Transmit a packet to manycore hardware
 * @param[in] mc      A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] packet  A packet to transmit to manycore hardware
 * @param[in] type    Is this packet a request or response packet?
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */

static int hb_mc_manycore_packet_tx_internal(hb_mc_manycore_t *mc,
                                             hb_mc_packet_t *packet,
                                             hb_mc_fifo_tx_t type,
                                             long timeout) {
        const char *typestr = hb_mc_fifo_tx_to_string(type);
        uintptr_t data_addr, len_addr;
        int err;

        if (timeout != -1) {
                manycore_pr_err(mc, "%s: Only a timeout value of -1 is supported\n",
                                __func__);
                return HB_MC_INVALID;
        }

        // get the address of the transmit data register TDR
        data_addr = hb_mc_mmio_fifo_get_addr(type, HB_MC_MMIO_FIFO_TX_DATA_OFFSET);

        // transmit the data one word at a time
        for (unsigned i = 0; i < array_size(packet->words); i++) {
                err = hb_mc_manycore_mmio_write32(mc, data_addr, packet->words[i]);
                if (err != HB_MC_SUCCESS) {
                        manycore_pr_err(mc, "%s: Failed to transmit word %d via %s FIFO: %s\n",
                                        __func__, i, typestr, hb_mc_strerror(err));
                        return err;
                }
        }

        return HB_MC_SUCCESS;
}

/**
 * Receive a packet from manycore hardware
 * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[out] packet A packet into which data should be read
 * @param[in] type   Is this packet a request or response packet?
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int hb_mc_manycore_packet_rx_internal(hb_mc_manycore_t *mc,
                                             hb_mc_packet_t *packet,
                                             hb_mc_fifo_rx_t type,
                                             long timeout)
{
        const char *typestr = hb_mc_fifo_rx_to_string(type);
        uintptr_t data_addr;
        uint32_t occupancy;
        int err;

        if (timeout != -1) {
                manycore_pr_err(mc, "%s: Only a timeout value of -1 is supported\n",
                                __func__);
                return HB_MC_INVALID;
        }

        data_addr   = hb_mc_mmio_fifo_get_addr(type, HB_MC_MMIO_FIFO_RX_DATA_OFFSET);

        if (type == HB_MC_FIFO_RX_REQ) {
            /* wait for a packet */
            do {
                    err = hb_mc_manycore_rx_fifo_get_occupancy(mc, type, &occupancy);
                    if (err != HB_MC_SUCCESS) {
                            manycore_pr_err(mc, "%s: Failed to get %s FIFO occupancy while waiting for packet: %s\n",
                                            __func__, typestr, hb_mc_strerror(err));
                            return err;
                    }

            } while (occupancy < 1);  // this is packet occupancy, not word occupancy!
        }

        /* read in the packet one word at a time */
        for (unsigned i = 0; i < array_size(packet->words); i++) {
                err = hb_mc_manycore_mmio_read32(mc, data_addr, &packet->words[i]);
                if (err != HB_MC_SUCCESS) {
                        manycore_pr_err(mc, "%s: Failed read data from %s FIFO: %s\n",
                                        __func__, typestr, hb_mc_strerror(err));
                        return err;
                }
        }

        return HB_MC_SUCCESS;
}

/**
 * Transmit a request packet to manycore hardware
 * @param[in] mc      A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] request A request packet to transmit to manycore hardware
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_manycore_request_tx(hb_mc_manycore_t *mc,
                              hb_mc_request_packet_t *request,
                              long timeout)
{
        int err;

        /* send the request packet */
        err = hb_mc_manycore_packet_tx_internal(mc, (hb_mc_packet_t*)request, HB_MC_FIFO_TX_REQ, timeout);
        if (err != HB_MC_SUCCESS)
                return err;

        err = hb_mc_manycore_incr_host_requests(mc);
        if (err != HB_MC_SUCCESS)
                return err;

        return HB_MC_SUCCESS;
}

/**
 * Receive a response packet from manycore hardware
 * @param[in] mc       A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] response A packet into which data should be read
 * @param[in] timeout  A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_manycore_response_rx(hb_mc_manycore_t *mc,
                               hb_mc_response_packet_t *response,
                               long timeout)
{
        int err;

        /* receive the response packet */
        err = hb_mc_manycore_packet_rx_internal(mc, (hb_mc_packet_t*)response, HB_MC_FIFO_RX_RSP, timeout);
        if (err != HB_MC_SUCCESS)
                return err;


        /* update the outstanding requests */
        err = hb_mc_manycore_decr_host_requests(mc);
        if (err != HB_MC_SUCCESS)
                return err;

        return HB_MC_SUCCESS;
}

/**
 * Transmit a response packet to manycore hardware
 * @param[in] mc        A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] response  A response packet to transmit to manycore hardware
 * @param[in] timeout   A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_manycore_response_tx(hb_mc_manycore_t *mc,
                               hb_mc_response_packet_t *response,
                               long timeout)
{
        return hb_mc_manycore_packet_tx_internal(mc, (hb_mc_packet_t*)response, HB_MC_FIFO_TX_RSP, timeout);
}

/**
 * Receive a request packet from manycore hardware
 * @param[in] mc      A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] request A packet into which data should be read
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_manycore_request_rx(hb_mc_manycore_t *mc,
                              hb_mc_request_packet_t *request,
                              long timeout)
{
        int err;
        err = hb_mc_manycore_packet_rx_internal(mc, (hb_mc_packet_t*)request, HB_MC_FIFO_RX_REQ, timeout);
        if (err != HB_MC_SUCCESS)
                return err;

        err = hb_mc_responders_respond(mc, request);
        if (err != HB_MC_SUCCESS) {
                char request_str[64];
                hb_mc_request_packet_to_string(request, request_str, sizeof(request_str));
                bsg_pr_err("responder failure to %s\n", request_str);
        }

        return HB_MC_SUCCESS;
}

/**
 * Transmit a packet to manycore hardware
 * @param[in] mc      A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] packet  A packet to transmit to manycore hardware
 * @param[in] type    Is this packet a request or response packet?
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_manycore_packet_tx(hb_mc_manycore_t *mc,
                             hb_mc_packet_t *packet,
                             hb_mc_fifo_tx_t type,
                             long timeout)
{
        switch (type) {
        case HB_MC_FIFO_TX_RSP:
                return hb_mc_manycore_response_tx(mc, &packet->response, timeout);
        case HB_MC_FIFO_TX_REQ:
                return hb_mc_manycore_request_tx(mc, &packet->request, timeout);
        }
}

/**
 * Receive a packet from manycore hardware
 * @param[in] mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] packet A packet into which data should be read
 * @param[in] type   Is this packet a request or response packet?
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_manycore_packet_rx(hb_mc_manycore_t *mc,
                             hb_mc_packet_t *packet,
                             hb_mc_fifo_rx_t type,
                             long timeout)
{
        switch (type) {
        case HB_MC_FIFO_RX_RSP:
                return hb_mc_manycore_response_rx(mc, &packet->response, timeout);
        case HB_MC_FIFO_RX_REQ:
                return hb_mc_manycore_request_rx(mc, &packet->request, timeout);
        }
}

/////////////////////////////
// Packet Helper Functions //
/////////////////////////////

static bool hb_mc_manycore_dst_npa_is_valid(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa)
{
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        hb_mc_dimension_t dim = hb_mc_config_get_dimension_network(cfg);

        if (hb_mc_npa_get_x(npa) >= hb_mc_dimension_get_x(dim)) {
                char npa_str[256];
                manycore_pr_err(mc, "%s: %s is not a valid destination\n",
                                __func__,
                                hb_mc_npa_to_string(npa, npa_str, sizeof(npa_str)));
                return false;
        }

        if (hb_mc_npa_get_y(npa) >= hb_mc_dimension_get_y(dim)) {
                char npa_str[256];
                manycore_pr_err(mc, "%s: %s is not a valid destination\n",
                                hb_mc_npa_to_string(npa, npa_str, sizeof(npa_str)));
                return false;
        }

        return true;
}

static int hb_mc_manycore_format_request_packet(hb_mc_manycore_t *mc, hb_mc_request_packet_t *pkt, const hb_mc_npa_t *npa)
{
        hb_mc_coordinate_t host_coordinate = hb_mc_manycore_get_host_coordinate(mc);

        if (!hb_mc_manycore_dst_npa_is_valid(mc, npa))
                return HB_MC_INVALID;

        hb_mc_request_packet_set_x_dst(pkt, hb_mc_npa_get_x(npa));
        hb_mc_request_packet_set_y_dst(pkt, hb_mc_npa_get_y(npa));
        hb_mc_request_packet_set_x_src(pkt, hb_mc_coordinate_get_x(host_coordinate));
        hb_mc_request_packet_set_y_src(pkt, hb_mc_coordinate_get_y(host_coordinate));
        hb_mc_request_packet_set_addr(pkt, hb_mc_npa_get_epa(npa) >> 2);
        return 0;
}

static int hb_mc_manycore_format_store_request_packet(hb_mc_manycore_t *mc,
                                                      hb_mc_request_packet_t *pkt,
                                                      const hb_mc_npa_t *npa)
{
        int r;

        if ((r = hb_mc_manycore_format_request_packet(mc, pkt, npa)) != 0)
                return r;

        hb_mc_request_packet_set_op(pkt, HB_MC_PACKET_OP_REMOTE_STORE);

        return 0;
}

static int hb_mc_manycore_format_load_request_packet(hb_mc_manycore_t *mc,
                                                     hb_mc_request_packet_t *pkt,
                                                     const hb_mc_npa_t *npa)
{
        int r;

        if ((r = hb_mc_manycore_format_request_packet(mc, pkt, npa)) != 0)
                return r;

        hb_mc_request_packet_set_data(pkt, HB_MC_PACKET_PAYLOAD_REMOTE_LOAD);
        hb_mc_request_packet_set_op(pkt, HB_MC_PACKET_OP_REMOTE_LOAD);

        return 0;
}

static int hb_mc_manycore_format_cache_op_request_packet(hb_mc_manycore_t *mc,
                                                         hb_mc_request_packet_t *pkt,
                                                         const hb_mc_npa_t *npa,
                                                         hb_mc_packet_cache_op_t opcode)
{
        int r;

        if ((r = hb_mc_manycore_format_request_packet(mc, pkt, npa)) != 0)
                return r;

        hb_mc_request_packet_set_op(pkt, HB_MC_PACKET_OP_CACHE_OP);
        hb_mc_request_packet_set_cache_op(pkt, opcode);

        return 0;
}


/************************/
/* Cache Operations API */
/************************/

/**
 * Apply cache operation to NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t (must map to DRAM) - start of the range to invalidate
 * @param[in]  sz     The size of the range to invalidate in bytes
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
static
int hb_mc_manycore_vcache_apply_to_npa(hb_mc_manycore_t *mc,
                                       const hb_mc_npa_t *npa,
                                       hb_mc_packet_cache_op_t cache_op)
{
        int err;
        hb_mc_request_packet_t pkt;

        if ((err = hb_mc_manycore_format_cache_op_request_packet(mc, &pkt, npa, cache_op)))
                return err;

        err = hb_mc_manycore_request_tx(mc, &pkt, -1);
        if (err != HB_MC_SUCCESS) {
                manycore_pr_err(mc, "%s: Failed to send request packet: %s\n",
                                __func__, hb_mc_strerror(err));
        }

        return HB_MC_SUCCESS;
}

/**
 * Apply cache operation to a range of NPAs
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t (must map to DRAM) - start of the range to invalidate
 * @param[in]  sz     The size of the range to invalidate in bytes
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
static int hb_mc_manycore_vcache_apply_to_npa_range(hb_mc_manycore_t *mc,
                                                    const hb_mc_npa_t *npa,
                                                    size_t range_sz,
                                                    hb_mc_packet_cache_op_t cache_op)
{
        hb_mc_epa_t epa = hb_mc_npa_get_epa(npa);
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        ssize_t sz = static_cast<ssize_t>(range_sz);
        ssize_t bsize = static_cast<ssize_t>(hb_mc_config_get_vcache_block_size(cfg));
        int err;

        // align npa to closest cache line
        sz += (epa & (bsize - 1));
        epa &= -bsize;

        // until we've applied op the entire range...
        while (sz > 0) {
                // apply op to line address
                hb_mc_npa_t line_npa = *npa;
                hb_mc_npa_set_epa(&line_npa, epa);

                err = hb_mc_manycore_vcache_apply_to_npa(mc, npa, cache_op);
                if (err != HB_MC_SUCCESS)
                        return err;

                // next line
                sz -= std::min(sz, bsize);
                epa += bsize;
        }

        return HB_MC_SUCCESS;
}

/**
 * Invalidate a range of manycore DRAM addresses.
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t (must map to DRAM) - start of the range to invalidate
 * @param[in]  sz     The size of the range to invalidate in bytes
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_manycore_vcache_invalidate_npa_range(hb_mc_manycore_t *mc,
                                               const hb_mc_npa_t *npa,
                                               size_t sz)
{
        return hb_mc_manycore_vcache_apply_to_npa_range(mc, npa, sz,
                                                        HB_MC_PACKET_CACHE_OP_AINV);
}

/**
 * Flush a range of manycore DRAM addresses.
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t (must map to DRAM) - start of the range to flush
 * @param[in]  sz     The size of the range to flush in bytes
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_manycore_vcache_flush_npa_range(hb_mc_manycore_t *mc,
                                          const hb_mc_npa_t *npa,
                                          size_t sz)
{
        int err;
        err = hb_mc_manycore_vcache_apply_to_npa_range(mc, npa, sz,
                                                       HB_MC_PACKET_CACHE_OP_AFL);
        if (err != HB_MC_SUCCESS)
                return err;

        // read a single word from cache - when it completes, assume flush is done
        uint32_t dummy;
        return hb_mc_manycore_read32(mc, npa, &dummy);
}

int hb_mc_manycore_vcache_flush_tag(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa)
{
        int err;
        hb_mc_request_packet_t pkt;

        err = hb_mc_manycore_format_cache_op_request_packet(mc, &pkt, npa, HB_MC_PACKET_CACHE_OP_TAGFL);
        if (err != HB_MC_SUCCESS)
                return err;

        return hb_mc_manycore_request_tx(mc, &pkt, -1);
}

template <typename ApplyFunction>
static int hb_mc_manycore_apply_to_vcache(hb_mc_manycore_t *mc, ApplyFunction apply_function)
{
        hb_mc_epa_t ways = hb_mc_vcache_num_ways(mc);
        hb_mc_epa_t sets = hb_mc_vcache_num_sets(mc);
        hb_mc_epa_t caches = hb_mc_vcache_num_caches(mc);
        int err;

        for (hb_mc_epa_t way_id = 0; way_id < ways; way_id++) {
                for (hb_mc_epa_t set_id = 0; set_id < sets; set_id++) {
                        for (hb_mc_epa_t cache_id = 0; cache_id < caches; cache_id++) {
                                // build the address for the way
                                hb_mc_npa_t way_addr = hb_mc_vcache_way_npa(mc, cache_id, set_id, way_id);
                                // apply
                                err = apply_function(mc, &way_addr);
                                if (err != HB_MC_SUCCESS)
                                        return err;
                        }
                }
        }
        return HB_MC_SUCCESS;
}

/**
 * Invalidate entire victim cache.
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 */
int hb_mc_manycore_invalidate_vcache(hb_mc_manycore_t *mc)
{
        return hb_mc_manycore_apply_to_vcache(mc, [](hb_mc_manycore_t *mc, const hb_mc_npa_t *way_addr) {
                        // write way_id (no valid bit)
                        char npa_str [256];
                        manycore_pr_dbg(mc, "Invalidating vcache tag @ %s\n",
                                        hb_mc_npa_to_string(way_addr, npa_str, sizeof(npa_str)));

                        return hb_mc_manycore_write32(mc, way_addr, 0);
                });
}


/**
 * Validate entire victim cache.
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 */
int hb_mc_manycore_validate_vcache(hb_mc_manycore_t *mc)
{
        return hb_mc_manycore_apply_to_vcache(mc, [](hb_mc_manycore_t *mc, const hb_mc_npa_t *way_addr) {
                        char npa_str[256];
                        uint32_t tag = HB_MC_VCACHE_VALID | hb_mc_vcache_way(mc, hb_mc_npa_get_epa(way_addr));
                        manycore_pr_dbg(mc, "Validating vcache tag @ %s with tag = 0x%08" PRIx32 "\n",
                                        hb_mc_npa_to_string(way_addr, npa_str, sizeof(npa_str)), tag);

                        // write the way_id or'd with the valid bit
                        return hb_mc_manycore_write32(mc, way_addr, tag);
                });
}

/**
 * Flush entire victim cache.
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 */
int hb_mc_manycore_flush_vcache(hb_mc_manycore_t *mc)
{
        int err = hb_mc_manycore_apply_to_vcache(mc, [](hb_mc_manycore_t *mc, const hb_mc_npa_t *way_addr) {
                        // flush tag
                        char npa_str[256];
                        manycore_pr_dbg(mc, "Flushing vcach tag @ %s\n",
                                        hb_mc_npa_to_string(way_addr, npa_str, sizeof(npa_str)));
                        return hb_mc_manycore_vcache_flush_tag(mc, way_addr);
                });

        if (err != HB_MC_SUCCESS)
                return err;

        // read a word from each cache
        for (hb_mc_epa_t cache_id = 0; cache_id < hb_mc_vcache_num_caches(mc); cache_id++) {
                hb_mc_npa_t way_addr = hb_mc_vcache_way_npa(mc, cache_id, 0, 0);
                hb_mc_npa_set_epa(&way_addr, 0);
                uint32_t dummy;
                err = hb_mc_manycore_read32(mc, &way_addr, &dummy);
                if (err != HB_MC_SUCCESS)
                        return err;
        }
        return HB_MC_SUCCESS;
}


/////////////
// DMA API //
/////////////

/**
 * Check if NPA is in DRAM.
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t
 * @return One if the NPA maps to DRAM - Zero otherwise.
 */
int hb_mc_manycore_npa_is_dram(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa)
{
    const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
    return hb_mc_config_is_dram_y(cfg, hb_mc_npa_get_y(npa));
}

#if defined(DRAM_HACK_WRITE) || defined(DRAM_HACK_READ)
/**
 * Given an NPA that maps to DRAM, return a buffer that holds the data for that address.
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t - must be an L2 cache coordinate
 * @param[in]  sz     The number of bytes to write to manycore hardware - used for sanity check
 * @param[out] buffer The valid buffer
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int hb_mc_manycore_npa_to_buffer_cosim_only(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, size_t sz,
                                                   unsigned char **buffer)
{
    using namespace bsg_test_dram_channel;

    /*
      Our system supports having multiple caches per memory channel.
      Currently, we do this by splitting the channels evenly into even 'banks' for each cache.

      IF THE ADDRESS MAPPING SCHEME FROM CACHES TO DRAM CHANGES THIS FUNCTION WILL BREAK!!!!!

      As of the time of this writing we are in the process of designing the memory system.
      So take note...
     */

    /*
      Get system parameters for performing the address mapping
     */
    const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
    unsigned long caches = hb_mc_vcache_num_caches(mc);
    unsigned long channels = hb_mc_config_get_dram_channels(cfg);
    unsigned long caches_per_channel = caches/channels;

    manycore_pr_dbg(mc, "%s: caches = %lu, channels = %lu, caches_per_channel = %lu\n",
                    __func__, caches, channels, caches_per_channel);

    /*
      Figure out which memory channel and bank this NPA maps to.
     */
    hb_mc_idx_t cache_id = hb_mc_config_get_dram_id(cfg, hb_mc_npa_get_xy(npa)); // which cache
    parameter_t id = cache_id / caches_per_channel; // which channel
    parameter_t bank = cache_id % caches_per_channel; // which bank within channel

    /*
      Use the backdoor to our non-synthesizable memory.
     */
    Memory *memory = bsg_test_dram_channel_get_memory(id);
    parameter_t bank_size = memory->_data.size()/caches_per_channel;

    hb_mc_epa_t epa = hb_mc_npa_get_epa(npa);

    char npa_str[256];

    if (memory == nullptr) {
        manycore_pr_err(mc, " %s: Could not get the memory for endpoint at %s\n",
                        __func__, hb_mc_npa_to_string(npa, npa_str, sizeof(npa_str)));

        return HB_MC_FAIL;
    }

    address_t addr = bank*bank_size + epa;

    manycore_pr_dbg(mc, "%s: Mapped %s to Channel %2lu, Address 0x%08lx\n",
                    __func__, hb_mc_npa_to_string(npa, npa_str, sizeof(npa_str)), id, addr);

    /*
      Don't overflow memory if you can help it.
    */
    assert(addr + sz <= memory->_data.size());

    *buffer = &memory->_data[addr];

    return HB_MC_SUCCESS;
}

/**
 * Write memory out to manycore DRAM via C++ backdoor
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t - must be an L2 cache coordinate
 * @param[in]  data   A buffer to be written out manycore hardware
 * @param[in]  sz     The number of bytes to write to manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int hb_mc_manycore_write_dram_cosim_only(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa,
                                                const void *data, size_t sz)
{

    unsigned char *membuffer;
    int err = hb_mc_manycore_npa_to_buffer_cosim_only(mc, npa, sz, &membuffer);
    if (err != HB_MC_SUCCESS)
        return err;

    char npa_str[256];

    manycore_pr_dbg(mc, "%s: Writing %3zu bytes to %s\n",
                    __func__, sz, hb_mc_npa_to_string(npa, npa_str, sizeof(npa_str)));

    memcpy(reinterpret_cast<void*>(membuffer), data, sz);

    return HB_MC_SUCCESS;
}


/**
 * Read memory from manycore DRAM via C++ backdoor
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t - must be an L2 cache coordinate
 * @param[in]  data   A host buffer to be read into from manycore hardware
 * @param[in]  sz     The number of bytes to read from manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int hb_mc_manycore_read_dram_cosim_only(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa,
                                               void *data, size_t sz)
{

    unsigned char *membuffer;
    int err = hb_mc_manycore_npa_to_buffer_cosim_only(mc, npa, sz, &membuffer);
    if (err != HB_MC_SUCCESS)
        return err;

    char npa_str[256];

    manycore_pr_dbg(mc, "%s: Reading %3zu bytes from %s\n",
                    __func__, sz, hb_mc_npa_to_string(npa, npa_str, sizeof(npa_str)));

    memcpy(data, reinterpret_cast<void*>(membuffer), sz);

    return HB_MC_SUCCESS;
}

#endif


/**
 * Check if DMA writing is supported.
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return One if DMA writing is supported - Zero otherwise.
 */
int hb_mc_manycore_supports_dma_write(const hb_mc_manycore *mc)
{
#if defined(COSIM) && defined(DRAM_HACK_WRITE)
        return 1;
#else
        return 0;
#endif
}

/**
 * Check if DMA reading is supported.
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return One if DMA reading is supported - Zero otherwise.
 */
int hb_mc_manycore_supports_dma_read(const hb_mc_manycore *mc)
{
#if defined(COSIM) && defined(DRAM_HACK_READ)
        return 1;
#else
        return 0;
#endif
}

/**
 * Write memory via DMA to manycore DRAM starting at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t (must map to DRAM)
 * @param[in]  data   A buffer to be written out manycore hardware
 * @param[in]  sz     The number of bytes to write to manycore hardware
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 *
 * This function is used to write to HammerBlade DRAM directly via DMA.
 * Any data in the cache that becomes stale will be invalidated - this function is 'safe' in that respect.
 *
 * However, invalidating every address might be expensive - and perhaps unnecessary if you 'know'
 * that the memory being written is guaranteed to be un-cached.
 * See hb_mc_manycore_dma_write_no_cache_ainv() for an unsafe version of this function.
 *
 * This function is not supported on all HammerBlade platforms.
 * Please check the return code for HB_MC_NOIMPL.
 */
int hb_mc_manycore_dma_write(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa,
                             const void *data, size_t sz)
{
        int err;

        err = hb_mc_manycore_dma_write_no_cache_ainv(mc, npa, data, sz);
        if (err != HB_MC_SUCCESS)
                return err;

        return hb_mc_manycore_vcache_invalidate_npa_range(mc, npa, sz);
}

/**
 * Write memory via DMA to manycore DRAM starting at a given NPA - unsafe
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t (must map to DRAM)
 * @param[in]  data   A buffer to be written out manycore hardware
 * @param[in]  sz     The number of bytes to write to manycore hardware
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 *
 * This function is used to write to HammerBlade DRAM directly via DMA.
 * Stale data may remain in the cache - this function is unsafe in that respect.
 * This function is not supported on all HammerBlade platforms.
 * Please check the return code for HB_MC_NOIMPL.
 */
int hb_mc_manycore_dma_write_no_cache_ainv(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa,
                                           const void *data, size_t sz)
{
        int err;

#if defined(COSIM) && defined(DRAM_HACK_WRITE)
        // is dram?
        if (!hb_mc_manycore_npa_is_dram(mc, npa))
                return HB_MC_INVALID;


        err = hb_mc_manycore_write_dram_cosim_only(mc, npa, data, sz);
        if (err != HB_MC_SUCCESS)
                return err;

        return HB_MC_SUCCESS;
#else
        return HB_MC_NOIMPL;
#endif
}


/**
 * Read memory via DMA from manycore DRAM starting at a given NPA - unsafe
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t (must map to DRAM)
 * @param[in]  data   A buffer into which data will be read
 * @param[in]  sz     The number of bytes to read from manycore hardware
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 *
 * This function is used to read from HammerBlade DRAM directly via DMA.
 * Cached data for this memory range migth not be flushed - this function is 'unsafe' in that respect.
 *
 * This function is not supported on all HammerBlade platforms.
 * Please check the return code for HB_MC_NOIMPL.
 */
int hb_mc_manycore_dma_read_no_cache_afl(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa,
                                         void *data, size_t sz)
{
        int err;
#if defined(COSIM) && defined(DRAM_HACK_READ)
        // is dram?
        if (!hb_mc_manycore_npa_is_dram(mc, npa))
                return HB_MC_INVALID;

        return hb_mc_manycore_read_dram_cosim_only(mc, npa, data, sz);
#else
        return HB_MC_NOIMPL;
#endif
}

/**
 * Read memory via DMA from manycore DRAM starting at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t (must map to DRAM)
 * @param[in]  data   A buffer into which data will be read
 * @param[in]  sz     The number of bytes to read from manycore hardware
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 *
 * This function is used to read from HammerBlade DRAM directly via DMA.
 * Any cached data for this memory range will be flushed - this function is 'safe' in that respect.
 *
 * However, sending a flush packet for every address in this range might be expensive -
 * and perhaps unnecessary if you 'know'  the data is uncached.
 * See hb_mc_manycore_dma_read_no_cache_afl() for an unsafe alternative to this function.
 *
 * This function is not supported on all HammerBlade platforms.
 * Please check the return code for HB_MC_NOIMPL.
 */
int hb_mc_manycore_dma_read(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa,
                            void *data, size_t sz)
{
        int err;
#if defined(COSIM) && defined(DRAM_HACK_READ)
        if (!hb_mc_manycore_npa_is_dram(mc, npa))
                return HB_MC_INVALID;

        err = hb_mc_manycore_vcache_flush_npa_range(mc, npa, sz);
        if (err != HB_MC_SUCCESS)
                return err;

        return hb_mc_manycore_dma_read_no_cache_afl(mc, npa, data, sz);
#else
        return HB_MC_NOIMPL;
#endif
}

////////////////
// Memory API //
////////////////

/**
 * Checks alignment of an epa based on data size in bytes.
 * @param[in] epa  epa address
 * @param[in] sz   data size in bytes.
 * @return         HB_MC_SUCCESS if npa is aligned and HB_MC_UNALIGNED if not,
 *                 and HB_MC_INVALID otherwise.
 */
static inline int hb_mc_manycore_epa_check_alignment(const hb_mc_epa_t *epa, size_t sz)
{
        switch (sz) {
        case 4:
                if (*epa & 0x3)
                        return HB_MC_UNALIGNED;
                break;
        case 2:
                if (*epa & 0x1)
                        return HB_MC_UNALIGNED;
                break;
        case 1:
                break;
        default:
                return HB_MC_INVALID;
        }
        return HB_MC_SUCCESS;
}

/* send a read request and don't wait for the return packet */
static int hb_mc_manycore_send_read_rqst(hb_mc_manycore_t *mc,
                                         const hb_mc_npa_t *npa, size_t sz,
                                         uint32_t id = 0)
{
        hb_mc_packet_t rqst;
        int err;

        /* format the request packet */
        err = hb_mc_manycore_format_load_request_packet(mc, &rqst.request, npa);
        if (err != HB_MC_SUCCESS) {
                manycore_pr_err(mc, "%s: Failed to format load request packet: %s\n",
                                __func__, hb_mc_strerror(err));
                return err;
        }

        hb_mc_epa_t epa = hb_mc_npa_get_epa(npa);

        // Check for 4-byte, 2-byte or 1-byte alignment based on size
        err = hb_mc_manycore_epa_check_alignment(&epa, sz);
        if (err != HB_MC_SUCCESS)
                return err;

        // mark request with id
        hb_mc_request_packet_set_load_id(&rqst.request, id);
        int shift = hb_mc_npa_get_epa(npa) & 0x3;
        /* set the byte mask */
        switch (sz) {
        case 4:
                hb_mc_request_packet_set_mask(&rqst.request, HB_MC_PACKET_REQUEST_MASK_WORD);
                break;
        case 2:
                hb_mc_request_packet_set_mask(&rqst.request,
                                              static_cast<hb_mc_packet_mask_t>(
                                                      HB_MC_PACKET_REQUEST_MASK_SHORT << shift));
                break;
        case 1:
                hb_mc_request_packet_set_mask(&rqst.request, static_cast<hb_mc_packet_mask_t>(
                                                      HB_MC_PACKET_REQUEST_MASK_BYTE << shift));
                break;
        default:
                return HB_MC_INVALID;
        }

        /* transmit the request to the hardware */
        manycore_pr_dbg(mc, "Sending %d-byte read request to NPA "
                        "(x: %d, y: %d, 0x%08" PRIx32 ")\n",
                        sz,
                        hb_mc_npa_get_x(npa),
                        hb_mc_npa_get_y(npa),
                        hb_mc_npa_get_epa(npa));

        err = hb_mc_manycore_request_tx(mc, &rqst.request, -1);
        if (err == HB_MC_BUSY)
                return err; // omit the error message if just busy

        if (err != HB_MC_SUCCESS) {
                manycore_pr_err(mc, "%s: Failed to send request packet: %s\n",
                                __func__, hb_mc_strerror(err));
                return err;
        }

        return HB_MC_SUCCESS;
}

/* read a response packet for a read request to an npa */
static int hb_mc_manycore_recv_read_rsp(hb_mc_manycore_t *mc,
                                        uint32_t *vp,
                                        uint32_t *id = nullptr)
{
        hb_mc_packet_t rsp;
        int err;

        /* receive a packet from the hardware */
        err = hb_mc_manycore_response_rx(mc, &rsp.response, -1);
        if (err != HB_MC_SUCCESS) {
                manycore_pr_err(mc, "%s: Failed to read response packet: %s\n",
                                __func__, hb_mc_strerror(err));
                return err;
        }

        /* read data from packet */
        *vp = hb_mc_response_packet_get_data(&rsp.response);

        if (id != nullptr)
                *id = hb_mc_response_packet_get_load_id(&rsp.response);

        return HB_MC_SUCCESS;
}

template<typename UINT>
static UINT hb_mc_manycore_mask_load_data(const hb_mc_npa_t *npa, uint32_t load_data)
{
        int shift = CHAR_BIT * (hb_mc_npa_get_epa(npa) & 0x3);
        uint32_t result;

        /* make sure this template is being used only as intended */
        static_assert(std::is_unsigned<UINT>::value,
                      "hb_mc_manycore_mask_load_data: UINT must be uint8_t, uint16_t, or uint32_t");

        static_assert(std::is_integral<UINT>::value,
                      "hb_mc_manycore_mask_load_data: UINT must be uint8_t, uint16_t, or uint32_t");

        static_assert(sizeof(UINT) == 1 || sizeof(UINT) == 2 || sizeof(UINT) == 4,
                      "hb_mc_manycore_mask_load_data: UINT must be uint8_t, uint16_t, or uint32_t");

        if (sizeof(UINT) == 4) {
                result = load_data;
        } else if (sizeof(UINT) == 2) {
                result = (load_data >> shift) & 0xFFFF;
        } else if (sizeof(UINT) == 1) {
                result = (load_data >> shift) & 0xFF;
        }

        return static_cast<UINT>(result);
}

/* read from a memory address on the manycore */
template <typename UINT>
static int hb_mc_manycore_read(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, UINT *vp)
{
        int err;

        /* send load request */
        err = hb_mc_manycore_send_read_rqst(mc, npa, sizeof(UINT));
        if (err != HB_MC_SUCCESS)
                return err;

        /* read back response */
        uint32_t load_data;
        err = hb_mc_manycore_recv_read_rsp(mc, &load_data);
        if (err != HB_MC_SUCCESS)
                return err;

        /* mask off unused bits */
        *vp = hb_mc_manycore_mask_load_data<UINT>(npa, load_data);
        return HB_MC_SUCCESS;
}

/* write to a memory address on the manycore */
static int hb_mc_manycore_write(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, const void *vp, size_t sz)
{
        int err;
        hb_mc_packet_t rqst;

        /* format the packet */
        err = hb_mc_manycore_format_store_request_packet(mc, &rqst.request, npa);
        if (err != HB_MC_SUCCESS)
                return err;

        hb_mc_epa_t epa = hb_mc_npa_get_epa(npa);

        // Check for 4-byte, 2-byte or 1-byte alignment based on size
        err = hb_mc_manycore_epa_check_alignment(&epa, sz);
        if (err != HB_MC_SUCCESS)
                return err;

        int mask_shift = epa & 0x3;
        int data_shift = CHAR_BIT * mask_shift;
        /* set data and size */
        switch (sz) {
        case 4:
                hb_mc_request_packet_set_data(&rqst.request, *(const uint32_t*)vp);
                hb_mc_request_packet_set_mask(&rqst.request, HB_MC_PACKET_REQUEST_MASK_WORD);
                break;
        case 2:
                hb_mc_request_packet_set_data(&rqst.request, static_cast<uint32_t>(*(const uint16_t*)vp) << data_shift);
                hb_mc_request_packet_set_mask(&rqst.request, static_cast<hb_mc_packet_mask_t>(
                                                      HB_MC_PACKET_REQUEST_MASK_SHORT << mask_shift));
                break;
        case 1:
                hb_mc_request_packet_set_data(&rqst.request, static_cast<uint32_t>(*(const  uint8_t*)vp) << data_shift);
                hb_mc_request_packet_set_mask(&rqst.request, static_cast<hb_mc_packet_mask_t>(
                                                      HB_MC_PACKET_REQUEST_MASK_BYTE << mask_shift));
                break;
        default:
                return HB_MC_INVALID;
        }

        /* transmit the request */
        manycore_pr_dbg(mc, "Sending %d-byte write request to NPA "
                        "(x: %d, y: %d, 0x%08x) (data = 0x%08" PRIx32 ")\n",
                        sz,
                        hb_mc_npa_get_x(npa),
                        hb_mc_npa_get_y(npa),
                        hb_mc_npa_get_epa(npa),
                        hb_mc_request_packet_get_data(&rqst.request));

        return hb_mc_manycore_request_tx(mc, &rqst.request, -1);
}

/* checks that the arguments of read/write_mem are supported */
static int hb_mc_manycore_read_write_mem_check_args(hb_mc_manycore_t *mc,
                                                    const char *caller_name,
                                                    const void *data, size_t sz)
{
        uintptr_t ptr = (uintptr_t)data;

        if (ptr & 0x3) {
                manycore_pr_err(mc, "%s: Input 'data' = %p: "
                                "only 32-bit aligned data is supported\n",
                                caller_name, data);
                return HB_MC_NOIMPL;
        }

        if (sz & 0x3) {
                manycore_pr_err(mc, "%s: Input 'sz' = %zu: "
                                "only multiples of 4 are supported\n",
                                caller_name, sz);
                return HB_MC_NOIMPL;
        }
        return HB_MC_SUCCESS;
}

/**
 * Write memory out to manycore hardware starting at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t
 * @param[in]  data   A buffer to be written out manycore hardware
 * @param[in]  sz     The number of bytes to write to manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_write_mem(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa,
                             const void *data, size_t sz)
{
        int err;

        err = hb_mc_manycore_read_write_mem_check_args(mc, __func__, data, sz);
        if (err != HB_MC_SUCCESS)
                return err;

        // This pair of matching function calls changes the clock period of the
        // manycore during data transfer to accelerate simulation
#ifdef COSIM
        sv_set_virtual_dip_switch(0, 1);
#endif

        const uint32_t *words = (const uint32_t*)data;
        size_t n_words = sz >> 2;
        hb_mc_npa_t addr = *npa;

        /* send store requests one word at a time */
        for (size_t i = 0; i < n_words; i++) {

                /* do we have capacity for another request? */
                err = hb_mc_manycore_update_requests(mc);
                if (err != HB_MC_SUCCESS)
                        return err;

                err = hb_mc_manycore_write(mc, &addr, &words[i], 4);
                if (err != HB_MC_SUCCESS) {
                        manycore_pr_err(mc, "%s: Failed to send write request: %s\n",
                                        __func__, hb_mc_strerror(err));
                        return err;
                }

                err = hb_mc_manycore_incr_host_requests(mc);
                if (err != HB_MC_SUCCESS)
                        return err;

                // Increment EPA by 4:
                hb_mc_npa_set_epa(&addr, hb_mc_npa_get_epa(&addr) + 4);
        }

        err = hb_mc_manycore_host_request_fence(mc);
        if (err != HB_MC_SUCCESS)
                return err;


#ifdef COSIM
        sv_set_virtual_dip_switch(0, 0);
#endif
        return HB_MC_SUCCESS;
}

/**
 * Set memory to a given value starting at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t
 * @param[in]  val    Value to be written out
 * @param[in]  sz     The number of bytes to write to manycore hardware
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_manycore_memset(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa,
                          uint8_t val, size_t sz)
{
        int err;

        err = hb_mc_manycore_read_write_mem_check_args(mc, __func__, NULL, sz);
        if (err != HB_MC_SUCCESS)
                return err;

        const uint32_t word = (val << 24) | (val << 16) | (val << 8) | val;
        size_t n_words = sz >> 2;
        hb_mc_npa_t addr = *npa;

#ifdef COSIM
        sv_set_virtual_dip_switch(0, 1);
#endif

        /* send store requests one word at a time */
        for (size_t i = 0; i < n_words; i++) {

                /* do we have capacity for another request? */
                err = hb_mc_manycore_update_requests(mc);
                if (err != HB_MC_SUCCESS)
                        return err;

                err = hb_mc_manycore_write(mc, &addr, &word, 4);
                if (err != HB_MC_SUCCESS) {
                        manycore_pr_err(mc, "%s: Failed to send write request: %s\n",
                                        __func__, hb_mc_strerror(err));
                        return err;
                }

                err = hb_mc_manycore_incr_host_requests(mc);
                if (err != HB_MC_SUCCESS)
                        return err;

                // increment EPA by 1: (EPA's address words)
                hb_mc_npa_set_epa(&addr, hb_mc_npa_get_epa(&addr) + sizeof(uint32_t));
        }

        err = hb_mc_manycore_host_request_fence(mc);
        if (err != HB_MC_SUCCESS)
                return err;

#ifdef COSIM
        sv_set_virtual_dip_switch(0, 0);
#endif

        return HB_MC_SUCCESS;
}

/**
 * Perform #cnt loads from a series of NPAs and return results in an associative container #data.
 * After returning success, #data[i] shall be the data read from the NPA given by #npa(i)
 * for i >= 0 and i < cnt.
 *
 * @tparam UINT               The unsigned integer type for data loads.
 * @tparam UINTV              An associative container of UNT words (indexed by i).
 * @tparam NPA_OF_I_FUNCTION  Returns an NPA given an index i.
 *
 * @param[in]  mc    A manycore instance.
 * @param[in]  npa   A function that takes an index i and returns an NPA.
 * @param[out] data  A mutable associative container by which load data is returned.
 * @param[in]  cnt   The number of loads to perform.
 *
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
template <typename UINT, typename UINTV, typename NPA_OF_I_FUNCTION>
static int hb_mc_manycore_read_mem_internal(hb_mc_manycore_t *mc,
                                            NPA_OF_I_FUNCTION npa,
                                            UINTV & data, size_t cnt)
{
        size_t rsp_i = 0, rqst_i = 0;
        uint32_t occupancy;
        unsigned n_ids;
        int err;

        /* cap the number of load ids to the maximum number of pending requests */
        err = hb_mc_manycore_get_remote_load_cap(mc, &n_ids);
        if (err != HB_MC_SUCCESS)
                return err;

#ifdef COSIM
        sv_set_virtual_dip_switch(0, 1);
#endif

        /* track requests and responses with ids and id_to_rsp_i */
        std::stack <uint32_t, std::vector<uint32_t> > ids;
        for (int i = n_ids - 1; i >= 0; i--)
                ids.push(static_cast<uint32_t>(i));

        int id_to_rsp_i [n_ids];
        hb_mc_npa_t id_to_npa[n_ids];

        /* until we've received all responses... */
        while (rsp_i < cnt) {

                /* try to request as many words as we have left */
                while (rqst_i < cnt) {
                        // get the NPA of the next load address
                        hb_mc_npa_t rqst_addr = npa(rqst_i);

                        // if we're out of load ids, break to start reading requests
                        if (ids.empty())
                                break;

                        // get an available load id for this load request
                        uint32_t rqst_load_id = ids.top();

                        // save which request this is
                        id_to_rsp_i[rqst_load_id] = rqst_i;
                        id_to_npa[rqst_load_id] = rqst_addr;

                        // send a load request
                        err = hb_mc_manycore_send_read_rqst(mc, &rqst_addr, sizeof(UINT),
                                                            rqst_load_id);
                        if (err == HB_MC_SUCCESS) {
                                // success; increment succesful requests and pop the load id
                                rqst_i++;
                                ids.pop();
                        } else if (err == HB_MC_BUSY) {
                                // if we're busy, break to start reading requests
                                break;
                        } else {
                                // we've hit some other error: abort with an error message
                                manycore_pr_err(mc, "%s: Failed to send read request: %s\n",
                                                __func__, hb_mc_strerror(err));
                                return err;
                        }
                }

                /* read all available response packets */
                while (rsp_i < rqst_i) {
                        /* read a response and write it back to the location marked by load_id */
                        uint32_t read_data, load_id;
                        err = hb_mc_manycore_recv_read_rsp(mc, &read_data, &load_id);
                        if (err != HB_MC_SUCCESS) {
                                manycore_pr_err(mc, "%s: Failed to receive read response: %s\n",
                                                __func__, hb_mc_strerror(err));
                                return err;
                        }

                        manycore_pr_dbg(mc, "%s: Received response for load_id = %" PRIu32 "\n",
                                        __func__, load_id);

                        // this should never happen unless something is messed up in hardware
                        if (load_id >= n_ids) {
                                manycore_pr_err(mc, "%s: Bad load id = %" PRIu32 "\n",
                                                __func__, load_id);
                                return HB_MC_FAIL;
                        }

                        // write 'read_data' back to the correct location
                        data[id_to_rsp_i[load_id]] = hb_mc_manycore_mask_load_data<UINT>(&id_to_npa[load_id], read_data);

                        // increment succesful responses
                        rsp_i++;

                        // push the load id onto the stack so we can use it again
                        ids.push(load_id);
                }
        }
#ifdef COSIM
        sv_set_virtual_dip_switch(0, 0);
#endif

        return HB_MC_SUCCESS;
}

/**
 * Read memory from a vector of NPAs
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A vector of valid hb_mc_npa_t of length <= #words
 * @param[out] data   A word vector into which data will be read
 * @param[in]  words  The number of words to read from manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_read_mem_scatter_gather(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa,
                                           uint32_t *data, size_t words)
{
        /* ith NPA => npa[i] */
        struct npa_function {
                const hb_mc_npa_t *npa;
                npa_function(const hb_mc_npa_t *npa) : npa(npa) {}
                hb_mc_npa_t operator()(size_t i) { return npa[i]; }
        };

        return hb_mc_manycore_read_mem_internal<uint32_t>(mc, npa_function(npa), data, words);
}

/**
 * Read memory from manycore hardware starting at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t
 * @param[out] data   A buffer into which data will be read
 * @param[in]  sz     The number of bytes to read from manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_read_mem(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa,
                            void *data, size_t sz)
{
        int err;

        err = hb_mc_manycore_read_write_mem_check_args(mc, __func__, data, sz);
        if (err != HB_MC_SUCCESS)
                return err;

        uint32_t *words = static_cast<uint32_t*>(data);
        size_t n_words = sz >> 2;

        /* ith NPA => first NPA + i words */
        struct npa_function {
                const hb_mc_npa_t *npa;
                npa_function(const hb_mc_npa_t *npa) : npa(npa) {}
                hb_mc_npa_t operator()(size_t i) {
                        return hb_mc_npa_from_x_y(hb_mc_npa_get_x(npa),
                                                  hb_mc_npa_get_y(npa),
                                                  hb_mc_npa_get_epa(npa) +
                                                  i*sizeof(uint32_t));
                }
        };

        return hb_mc_manycore_read_mem_internal<uint32_t>(mc, npa_function(npa), words, n_words);
}

/**
 * Read one byte from manycore hardware at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t (note that this is a byte-level address)
 * @param[out] vp     A byte to be set to the data read
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_read8(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, uint8_t *vp)
{
        return hb_mc_manycore_read(mc, npa, vp);
}

/**
 * Read a 16-bit half-word from manycore hardware at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t aligned to a two byte boundary (note that this is a byte-level address)
 * @param[out] vp     A half-word to be set to the data read
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_read16(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, uint16_t *vp)
{
        return hb_mc_manycore_read(mc, npa, vp);
}

/**
 * Read a 32-bit word from manycore hardware at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t aligned to a four byte boundary (note that this is a byte-level address)
 * @param[out] vp     A word to be set to the data read
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_read32(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, uint32_t *vp)
{
        return hb_mc_manycore_read(mc, npa, vp);
}

/**
 * Write one byte to manycore hardware at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t
 * @param[in]  v      A byte value to be written out
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_write8(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, uint8_t v)
{
        return hb_mc_manycore_write(mc, npa, &v, 1);
}

/**
 * Write a 16-bit half-word to manycore hardware at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t aligned to a two byte boundary
 * @param[in]  v      A half-word value to be written out
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_write16(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, uint16_t v)
{
        return hb_mc_manycore_write(mc, npa, &v, 2);
}

/**
 * Write a 32-bit word to manycore hardware at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t aligned to a four byte boundary
 * @param[in]  v      A word value to be written out
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_write32(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, uint32_t v)
{
        return hb_mc_manycore_write(mc, npa, &v, 4);
}


/**
 * Enable DRAM mode on the manycore instance.
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return One if DRAM is enabled. Zero otherwise.
 */
int hb_mc_manycore_enable_dram(hb_mc_manycore_t *mc)
{
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        /* for each tile */
        hb_mc_idx_t
                n_rows = hb_mc_dimension_get_y(hb_mc_config_get_dimension_vcore(cfg)),
                n_cols = hb_mc_dimension_get_x(hb_mc_config_get_dimension_vcore(cfg));

        for (hb_mc_idx_t row = 0; row < n_rows; row++) {
                for (hb_mc_idx_t col = 0; col < n_cols; col++) {
                        hb_mc_idx_t x = col + hb_mc_config_get_vcore_base_x(cfg);
                        hb_mc_idx_t y = row + hb_mc_config_get_vcore_base_y(cfg);
                        hb_mc_coordinate_t tile = hb_mc_coordinate(x,y);
                        int err = hb_mc_tile_set_dram_enabled(mc, &tile);
                        if (err != HB_MC_SUCCESS)
                                return err;
                }
        }
        mc->dram_enabled = 1;
        return HB_MC_SUCCESS;
}

/**
 * Disable DRAM mode on the manycore instance.
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return One if DRAM is enabled. Zero otherwise.
 */
int hb_mc_manycore_disable_dram(hb_mc_manycore_t *mc)
{
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        /* for each tile */
        hb_mc_idx_t
                n_rows = hb_mc_dimension_get_y(hb_mc_config_get_dimension_vcore(cfg)),
                n_cols = hb_mc_dimension_get_x(hb_mc_config_get_dimension_vcore(cfg));

        for (hb_mc_idx_t row = 0; row < n_rows; row++) {
                for (hb_mc_idx_t col = 0; col < n_cols; col++) {
                        hb_mc_idx_t x = col + hb_mc_config_get_vcore_base_x(cfg);
                        hb_mc_idx_t y = row + hb_mc_config_get_vcore_base_y(cfg);
                        hb_mc_coordinate_t tile = hb_mc_coordinate(x,y);
                        int err = hb_mc_tile_clear_dram_enabled(mc, &tile);
                        if (err != HB_MC_SUCCESS)
                                return err;
                }
        }
        mc->dram_enabled = 0;
        return HB_MC_SUCCESS;
}
