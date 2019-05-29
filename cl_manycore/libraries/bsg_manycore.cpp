#include <bsg_manycore.h>
#include <bsg_manycore_driver.h>
#include <bsg_manycore_mmio.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_tile.h>

#ifndef COSIM
#include <fpga_pci.h>
#include <fpga_mgmt.h>
#else
#include <fpga_pci_sv.h>
#include <utils/sh_dpi_tasks.h>
#endif

#ifdef __cplusplus
#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <climits>
#include <cstdbool>
#else
#include <inttypes>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>
#endif

#include <vector>

#define array_size(x)                           \
        (sizeof(x)/sizeof(x[0]))

#define sarray_size(x)				\
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

///////////////////////////
// FIFO Helper Functions //
///////////////////////////

static int hb_mc_manycore_fifo_get_isr_bit(hb_mc_manycore_t *mc, hb_mc_direction_t dir, uint32_t bit, uint32_t *v)
{
        uintptr_t isr_addr = hb_mc_mmio_fifo_get_reg_addr(dir, HB_MC_MMIO_FIFO_ISR_OFFSET);
        uint32_t tmp;
        int err;

        err = hb_mc_manycore_mmio_read32(mc, isr_addr, &tmp);
        if (err != HB_MC_SUCCESS) {
                manycore_pr_err(mc, "Failed to read bit in %s ISR register: %s\n",
                                hb_mc_direction_to_string(dir), hb_mc_strerror(err));
                return err;
        }

        *v = (tmp >> bit) & 1;
        return HB_MC_SUCCESS;
}

static int hb_mc_manycore_fifo_clear_isr_bit(hb_mc_manycore_t *mc, hb_mc_direction_t dir, uint32_t bit)
{
        uintptr_t isr_addr = hb_mc_mmio_fifo_get_reg_addr(dir, HB_MC_MMIO_FIFO_ISR_OFFSET);
        int err;

        err = hb_mc_manycore_mmio_write32(mc, isr_addr, (1<<bit));
        if (err != HB_MC_SUCCESS) {
                manycore_pr_err(mc, "Failed to set bit in %s ISR register: %s\n",
                                hb_mc_direction_to_string(dir), hb_mc_strerror(err));
                return err;
        }

        return HB_MC_SUCCESS;
}

static int hb_mc_manycore_tx_fifo_get_vacancy(hb_mc_manycore_t *mc,
					      hb_mc_fifo_tx_t type,
					      uint32_t *vacancy)
{
        const char *typestr = hb_mc_fifo_tx_to_string(type);
        uintptr_t vacancy_addr = hb_mc_mmio_fifo_get_reg_addr(type, HB_MC_MMIO_FIFO_TX_VACANCY_OFFSET);
        int err;

        err = hb_mc_manycore_mmio_read32(mc, vacancy_addr, vacancy);
        if (err != HB_MC_SUCCESS) {
                manycore_pr_err(mc, "Failed to get %s vacancy\n", typestr);
                return err;
        }

        return HB_MC_SUCCESS;
}


/* get the number of unread packets in a FIFO (rx only) */
static int hb_mc_manycore_rx_fifo_get_occupancy(hb_mc_manycore_t *mc,
                                                hb_mc_fifo_rx_t type,
                                                uint32_t *occupancy)
{
        uintptr_t occupancy_addr = hb_mc_mmio_fifo_get_reg_addr(type, HB_MC_MMIO_FIFO_RX_OCCUPANCY_OFFSET);
        const char *typestr = hb_mc_fifo_rx_to_string(type);
        int err;

        err = hb_mc_manycore_mmio_read32(mc, occupancy_addr, occupancy);
        if (err != HB_MC_SUCCESS) {
                manycore_pr_err(mc, "Failed to get %s occupancy\n", typestr);
                return err;
        }

        return HB_MC_SUCCESS;
}

/* read all unread packets from a fifo (rx only) */
static int hb_mc_manycore_rx_fifo_drain(hb_mc_manycore_t *mc, hb_mc_fifo_rx_t type)
{
        const char *typestr = hb_mc_fifo_rx_to_string(type);
        hb_mc_request_packet_t recv;
        uint32_t occupancy;
        int rc;

        /* first check how many unread packets are currently in the FIFO */
        rc = hb_mc_manycore_rx_fifo_get_occupancy(mc, type, &occupancy);
        if (rc != HB_MC_SUCCESS)
                return rc;

	/* Read stale packets from fifo */
	for (unsigned i = 0; i < occupancy; i++){
                rc = hb_mc_manycore_packet_rx(mc, (hb_mc_packet_t*) &recv, type, -1);
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

static int hb_mc_manycore_fifo_clear_isr(hb_mc_manycore_t *mc, hb_mc_direction_t fifo)
{
        uintptr_t isr_addr = hb_mc_mmio_fifo_get_reg_addr(fifo, HB_MC_MMIO_FIFO_ISR_OFFSET);
        int err;

        /* ISR bits are write-one-to-clear */
        err = hb_mc_manycore_mmio_write32(mc, isr_addr, 0xFFFFFFFF);
        if (err != HB_MC_SUCCESS) {
                manycore_pr_err(mc, "Failed clear ISR\n");
                return err;
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

/* enables a fifo for mc */
static int hb_mc_fifo_enable(hb_mc_manycore_t *mc, hb_mc_direction_t fifo)
{
        uintptr_t ier_addr = hb_mc_mmio_fifo_get_reg_addr(fifo, HB_MC_MMIO_FIFO_IER_OFFSET);
        const char *fifo_name = hb_mc_direction_to_string(fifo);

        /* enable the transmit complete interrupt status bit */
        int err = hb_mc_manycore_mmio_write32(mc, ier_addr, 1 << HB_MC_MMIO_FIFO_IXR_TC_BIT);
        if (err != HB_MC_SUCCESS) {
                manycore_pr_err(mc, "Failed to initialize %s fifo\n", fifo_name);
                return err;
        }

	manycore_pr_dbg(mc, "Enabled fifo (%s)\n", fifo_name);
        return HB_MC_SUCCESS;
}

static int hb_mc_manycore_init_fifos(hb_mc_manycore_t *mc)
{
        int rc;

        /* enable fifos */
        rc = hb_mc_fifo_enable(mc, HB_MC_MMIO_FIFO_TO_DEVICE);
        if (rc != HB_MC_SUCCESS)
                return rc;

        rc = hb_mc_fifo_enable(mc, HB_MC_MMIO_FIFO_TO_HOST);
        if (rc != HB_MC_SUCCESS)
                return rc;

        /* drain rx FIFOs */
        rc = hb_mc_manycore_rx_fifo_drain(mc, HB_MC_FIFO_RX_REQ);
        if (rc != HB_MC_SUCCESS)
                return rc;

        rc = hb_mc_manycore_rx_fifo_drain(mc, HB_MC_FIFO_RX_RSP);
        if (rc != HB_MC_SUCCESS)
                return rc;

        /* clear interrupts registers */
        rc = hb_mc_manycore_fifo_clear_isr(mc, HB_MC_MMIO_FIFO_TO_DEVICE);
        if (rc != HB_MC_SUCCESS)
                return rc;

        rc = hb_mc_manycore_fifo_clear_isr(mc, HB_MC_MMIO_FIFO_TO_HOST);
        if (rc != HB_MC_SUCCESS)
                return rc;

        return HB_MC_SUCCESS;
}

static void hb_mc_manycore_cleanup_fifos(hb_mc_manycore_t *mc)
{
        /* drain rx FIFOs */
        hb_mc_manycore_rx_fifo_drain(mc, HB_MC_FIFO_RX_REQ);
        hb_mc_manycore_rx_fifo_drain(mc, HB_MC_FIFO_RX_RSP);
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

        r = HB_MC_SUCCESS;
        goto done;

cleanup:
	r = err;
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
void hb_mc_manycore_exit(hb_mc_manycore_t *mc)
{
        hb_mc_manycore_cleanup_fifos(mc);
        hb_mc_manycore_cleanup_mmio(mc);
        hb_mc_manycore_cleanup_private_data(mc);
        free((void*)mc->name);
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
                return HB_MC_INVALID;
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
 * Read the number of remaining available host credits
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_FAIL if an error occured. Number of remaining host credits otherwise
 */
int hb_mc_manycore_get_host_credits(hb_mc_manycore_t *mc)
{
        uint64_t addr;
        uint32_t value;
        int err;
        addr = hb_mc_mmio_credits_get_reg_addr(HB_MC_MMIO_CREDITS_HOST_OFFSET);
        err = hb_mc_manycore_mmio_read32(mc, addr, &value);
        if (err != HB_MC_SUCCESS) {
                manycore_pr_err(mc, "%s: Failed to read Host Credits Register: %s\n",
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
                return HB_MC_INVALID;
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

int hb_mc_manycore_packet_tx(hb_mc_manycore_t *mc,
			     hb_mc_packet_t *packet,
			     hb_mc_fifo_tx_t type,
			     long timeout) {
        const char *typestr = hb_mc_fifo_tx_to_string(type);
        uintptr_t data_addr, len_addr;
        hb_mc_direction_t dir;
        uint32_t vacancy, tx_complete;
        int err;

        if (timeout != -1) {
                manycore_pr_err(mc, "%s: Only a timeout value of -1 is supported\n",
                                __func__);
                return HB_MC_INVALID;
        }

        // get the address of the data and length registers
        data_addr = hb_mc_mmio_fifo_get_reg_addr(type, HB_MC_MMIO_FIFO_TX_DATA_OFFSET);
        len_addr  = hb_mc_mmio_fifo_get_reg_addr(type, HB_MC_MMIO_FIFO_TX_LENGTH_OFFSET);

        // get the direction
        dir = hb_mc_get_tx_direction(type);

        // get vacancy
        err = hb_mc_manycore_tx_fifo_get_vacancy(mc, type, &vacancy);
        if (err != HB_MC_SUCCESS) {
                manycore_pr_err(mc, "%s: Failed to read %s FIFO vacancy: %s\n",
                                __func__, typestr, hb_mc_strerror(err));
                return err;
        }

        if (vacancy < array_size(packet->words)) {
                manycore_pr_err(mc, "%s: FIFO %s has vacancy less than a unit packet size\n",
                                __func__, typestr);
                return HB_MC_FAIL;
        }

        // clear the Transmit Complete bit
        err = hb_mc_manycore_fifo_clear_isr_bit(mc, dir, HB_MC_MMIO_FIFO_IXR_TC_BIT);
        if (err != HB_MC_SUCCESS) {
                manycore_pr_err(mc, "%s: Failed to clear TX-Complete bit for FIFO %s "
				"(direction = %s): %s\n",
                                __func__,
				typestr,
				hb_mc_direction_to_string(dir), hb_mc_strerror(err));
                return err;
        }

        // transmit the data one word at a time
	for (unsigned i = 0; i < array_size(packet->words); i++) {
 		err = hb_mc_manycore_mmio_write32(mc, data_addr, packet->words[i]);
		if (err != HB_MC_SUCCESS) {
			manycore_pr_err(mc, "%s: Failed to transmit word %d via %s FIFO: %s\n",
					__func__, i, typestr, hb_mc_strerror(err));
			return err;
		}
	}

	do { //	wait until transmit is complete: continuously write a packet length until done
		err = hb_mc_manycore_mmio_write32(mc, len_addr, sizeof(*packet));
		if (err != HB_MC_SUCCESS) {
			manycore_pr_err(mc, "%s: Failed to write length to FIFO %s: %s\n",
					__func__, typestr, hb_mc_strerror(err));
			return err;
		}

		err = hb_mc_manycore_fifo_get_isr_bit(mc, dir, HB_MC_MMIO_FIFO_IXR_TC_BIT, &tx_complete);
		if (err != HB_MC_SUCCESS) {
			manycore_pr_err(mc, "%s: Failed to read TX-Complete bit for FIFO %s "
					"(direction = %s):"
					"%s\n",
					__func__,
					typestr,
					hb_mc_direction_to_string(dir), hb_mc_strerror(err));
			return err;
		}
	} while (!tx_complete);

	// clear the Transmit Complete bit
	err = hb_mc_manycore_fifo_clear_isr_bit(mc, dir, HB_MC_MMIO_FIFO_IXR_TC_BIT);
	if (err != HB_MC_SUCCESS) {
		manycore_pr_err(mc, "%s: Failed to clear TX-Complete bit for FIFO %s "
				"(direction = %s): %s\n",
				__func__,
				typestr,
				hb_mc_direction_to_string(dir), hb_mc_strerror(err));
		return err;
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

int hb_mc_manycore_packet_rx(hb_mc_manycore_t *mc,
			     hb_mc_packet_t *packet,
			     hb_mc_fifo_rx_t type,
			     long timeout)
{
        const char *typestr = hb_mc_fifo_rx_to_string(type);
        uintptr_t length_addr, data_addr;
        uint32_t occupancy, length;
        int err;

        if (timeout != -1) {
                manycore_pr_err(mc, "%s: Only a timeout value of -1 is supported\n",
                                __func__);
                return HB_MC_INVALID;
        }

        length_addr = hb_mc_mmio_fifo_get_reg_addr(type, HB_MC_MMIO_FIFO_RX_LENGTH_OFFSET);
        data_addr   = hb_mc_mmio_fifo_get_reg_addr(type, HB_MC_MMIO_FIFO_RX_DATA_OFFSET);

        /* wait for a packet */
        do {
                err = hb_mc_manycore_rx_fifo_get_occupancy(mc, type, &occupancy);
                if (err != HB_MC_SUCCESS) {
                        manycore_pr_err(mc, "%s: Failed to get %s FIFO occupancy while waiting for packet: %s\n",
                                        __func__, typestr, hb_mc_strerror(err));
                        return err;
                }

        } while (occupancy < 1);

        /* get FIFO length */
        err = hb_mc_manycore_mmio_read32(mc, length_addr, &length);
        if (err != HB_MC_SUCCESS) {
                manycore_pr_err(mc, "%s: Failed to read %s FIFO length register: %s\n",
                                __func__, typestr, hb_mc_strerror(err));
                return err;
        }

        if (length != sizeof(*packet)) {
                manycore_pr_err(mc, "%s: Read bad length %" PRId32 " from %s FIFO length register\n",
                                __func__, length, typestr);
                return HB_MC_FAIL;
        }

        manycore_pr_dbg(mc, "%s: From %s FIFO: Read the receive length register "
			"@ 0x%08" PRIxPTR " to be %" PRIu32 "\n",
                        __func__, typestr, length_addr, length);

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

/////////////////////////////
// Packet Helper Functions //
/////////////////////////////

static int hb_mc_manycore_format_request_packet(hb_mc_manycore_t *mc, hb_mc_request_packet_t *pkt, const hb_mc_npa_t *npa)
{
        hb_mc_coordinate_t host_coordinate = hb_mc_manycore_get_host_coordinate(mc);

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

        hb_mc_request_packet_set_op(pkt, HB_MC_PACKET_OP_REMOTE_LOAD);

        return 0;
}

////////////////
// Memory API //
////////////////

/* send a read request and don't wait for the return packet */
static int hb_mc_manycore_send_read_rqst(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, size_t sz)
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

	/* set the byte mask */
	switch (sz) {
	case 4:
		hb_mc_request_packet_set_mask(&rqst.request, HB_MC_PACKET_REQUEST_MASK_WORD);
		break;
	case 2:
	case 1:
		manycore_pr_err(mc, "%zu byte reads are not yet supported\n", sz);
		return HB_MC_NOIMPL;
	default:
		return HB_MC_INVALID;
	}

	/* transmit the request to the hardware */
	manycore_pr_dbg(mc, "Sending %d-byte read request to NPA "
			"(x: %d, y: %d, 0x%x)\n",
			sz,
			hb_mc_npa_get_x(npa),
			hb_mc_npa_get_y(npa),
			hb_mc_npa_get_epa(npa));
	err = hb_mc_manycore_packet_tx(mc, &rqst, HB_MC_FIFO_TX_REQ, -1);
	if (err != HB_MC_SUCCESS) {
		manycore_pr_err(mc, "%s: Failed to send request packet: %s\n",
				__func__, hb_mc_strerror(err));
		return err;
	}

	return HB_MC_SUCCESS;
}

/* read a response packet for a read request to an npa */
static int hb_mc_manycore_recv_read_rsp(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, void *vp, size_t sz)
{
	hb_mc_packet_t rsp;
	int err;

	/* receive a packet from the hardware */
	err = hb_mc_manycore_packet_rx(mc, &rsp, HB_MC_FIFO_RX_RSP, -1);
	if (err != HB_MC_SUCCESS) {
		manycore_pr_err(mc, "%s: Failed to read response packet: %s\n",
				__func__, hb_mc_strerror(err));
		return err;
	}

	/* check that the npa matches? */

	/* read data from packet */
	switch (sz) {
	case 4:
		*(uint32_t*)vp = hb_mc_response_packet_get_data(&rsp.response);
		break;
	case 2:
	case 1:
		manycore_pr_err(mc, "%zu byte reads are not yet supported\n", sz);
		return HB_MC_NOIMPL;
	default:
		return HB_MC_INVALID;
	}

	return HB_MC_SUCCESS;
}
/* read from a memory address on the manycore */
static int hb_mc_manycore_read(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, void *vp, size_t sz)
{
	int err;

	err = hb_mc_manycore_send_read_rqst(mc, npa, sz);
	if (err != HB_MC_SUCCESS)
		return err;

	return hb_mc_manycore_recv_read_rsp(mc, npa, vp, sz);
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

	/* set data and size */
	switch (sz) {
	case 4:
		hb_mc_request_packet_set_data(&rqst.request, *(const uint32_t*)vp);
		hb_mc_request_packet_set_mask(&rqst.request, HB_MC_PACKET_REQUEST_MASK_WORD);
		break;
	case 2:
	case 1:
		manycore_pr_err(mc, "%zu byte writes are not yet supported\n", sz);
		return HB_MC_NOIMPL;
	default:
		return HB_MC_INVALID;
	}

	/* transmit the request */
	manycore_pr_dbg(mc, "Sending %d-byte write request to NPA "
			"(x: %d, y: %d, 0x%08x)\n",
			sz,
			hb_mc_npa_get_x(npa),
			hb_mc_npa_get_y(npa),
			hb_mc_npa_get_epa(npa));
	return hb_mc_manycore_packet_tx(mc, &rqst, HB_MC_FIFO_TX_REQ, -1);
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

	const uint32_t *words = (const uint32_t*)data;
	size_t n_words = sz >> 2;
	hb_mc_npa_t addr = *npa;

	/* send store requests one word at a time */
	for (size_t i = 0; i < n_words; i++) {
		
		err = hb_mc_manycore_write(mc, &addr, &words[i], 4);
		if (err != HB_MC_SUCCESS) {
			manycore_pr_err(mc, "%s: Failed to send write request: %s\n",
					__func__, hb_mc_strerror(err));
			return err;
		}

		// Increment EPA by 4:
		hb_mc_npa_set_epa(&addr, hb_mc_npa_get_epa(&addr) + 4);
	}

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

	/* send store requests one word at a time */
	for (size_t i = 0; i < n_words; i++) {
		
		err = hb_mc_manycore_write(mc, &addr, &word, 4);
		if (err != HB_MC_SUCCESS) {
			manycore_pr_err(mc, "%s: Failed to send write request: %s\n",
					__func__, hb_mc_strerror(err));
			return err;
		}

		// increment EPA by 1: (EPA's address words)
		hb_mc_npa_set_epa(&addr, hb_mc_npa_get_epa(&addr) + sizeof(uint32_t));
	}

	return HB_MC_SUCCESS;
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

	uint32_t *words = (uint32_t*)data;
	size_t n_words = sz >> 2;
	hb_mc_npa_t  addr = *npa;

	/* we batch our read requests here instead of completing each read one at a time */

	/* send a read request for each word */
	for (size_t i = 0; i < n_words; i++) {
		
		err = hb_mc_manycore_send_read_rqst(mc, &addr, 4);
		if (err != HB_MC_SUCCESS) {
			manycore_pr_err(mc, "%s: Failed to send read request: %s\n",
					__func__, hb_mc_strerror(err));
			return err;
		}

		// increment EPA by 4:
		hb_mc_npa_set_epa(&addr, hb_mc_npa_get_epa(&addr) +sizeof(uint32_t));
	}

	/* now receive a packet for each word */
	addr = *npa;
	for (size_t i = 0; i < n_words; i++) {
                // increment EPA by 1: (EPA's address words)
		hb_mc_npa_set_epa(&addr, hb_mc_npa_get_epa(&addr)+1);
		
		err = hb_mc_manycore_recv_read_rsp(mc, &addr, &words[i], 4);
		if (err != HB_MC_SUCCESS) {
			manycore_pr_err(mc, "%s: Failed to receive read response: %s\n",
					__func__, hb_mc_strerror(err));
			return err;
		}
	}

	return HB_MC_SUCCESS;
}

/**
 * Read one byte from manycore hardware at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t
 * @param[out] vp     A byte to be set to the data read
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_read8(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, uint8_t *vp)
{
	return hb_mc_manycore_read(mc, npa, vp, 1);
}

/**
 * Read a 16-bit half-word from manycore hardware at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t aligned to a two byte boundary
 * @param[out] vp     A half-word to be set to the data read
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_read16(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, uint16_t *vp)
{
	return hb_mc_manycore_read(mc, npa, vp, 2);
}

/**
 * Read a 32-bit word from manycore hardware at a given NPA
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t aligned to a four byte boundary
 * @param[out] vp     A word to be set to the data read
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_manycore_read32(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, uint32_t *vp)
{
	return hb_mc_manycore_read(mc, npa, vp, 4);
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
