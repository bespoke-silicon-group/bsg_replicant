#include <bsg_manycore_platform.h>
#include <bsg_manycore_mmio.h>
#include <bsg_manycore_config.h>
#include <bsg_manycore_printing.h>

#include <cstring>

#ifndef COSIM
#include <fpga_pci.h>
#include <fpga_mgmt.h>
#else
#include <svdpi.h>
#include <fpga_pci_sv.h>
#include <utils/sh_dpi_tasks.h>
#endif

/* these are convenience macros that are only good for one line prints */
#define platform_pr_dbg(m, fmt, ...)                    \
        bsg_pr_dbg("%s: " fmt, m->name, ##__VA_ARGS__)

#define platform_pr_err(m, fmt, ...)                    \
        bsg_pr_err("%s: " fmt, m->name, ##__VA_ARGS__)

#define platform_pr_warn(m, fmt, ...)                   \
        bsg_pr_warn("%s: " fmt, m->name, ##__VA_ARGS__)

#define platform_pr_info(m, fmt, ...)                   \
        bsg_pr_info("%s: " fmt, m->name, ##__VA_ARGS__)

#define array_size(x)                           \
        (sizeof(x)/sizeof(x[0]))


typedef struct hb_mc_platform_t {
        const char *name;
#ifdef COSIM
        svScope scope;
#endif
        int transmit_vacancy;    //!< Software copy of the transmit vacancy register
        pci_bar_handle_t handle; //!< pci bar handle
        hb_mc_manycore_id_t id;  //!< which manycore instance is this
        uintptr_t      mmio;     //!< pointer to memory mapped io (F1-specific)
} hb_mc_platform_t;


/* initialize manycore MMIO */
static int hb_mc_platform_mmio_init(hb_mc_platform_t *pl,
                                    hb_mc_manycore_id_t id)
{

        int pf_id = FPGA_APP_PF, write_combine = 0, bar_id = APP_PF_BAR0;
        int r = HB_MC_FAIL, err;

        // all IDs except 0 are unused at the moment
        if (id != 0) {
                platform_pr_err(pl, "Failed to init MMIO: invalid ID\n");
                return HB_MC_INVALID;
        }

        if ((err = fpga_pci_attach(id, pf_id, bar_id, write_combine, &pl->handle)) != 0) {
                platform_pr_err(pl, "Failed to init MMIO: %s\n", FPGA_ERR2STR(err));
                platform_pr_err(pl, "Are you running with sudo?\n");
                return r;
        }

#if !defined(COSIM) // on F1
        // it is not clear to me where 0x4000 comes from...
        // map in the base address register to our address space
        if ((err = fpga_pci_get_address(pl->handle, 0, 0x4000, (void**)&pl->mmio)) != 0) {
                platform_pr_err(pl, "Failed to init MMIO: %s\n", FPGA_ERR2STR(err));
                goto cleanup;
        }
#else
        pl->mmio = (uintptr_t)nullptr;
#endif
        pl->id = id;
        r = HB_MC_SUCCESS;
        platform_pr_dbg(pl, "%s: pl->mmio = 0x%" PRIxPTR "\n", __func__, pl->mmio);
        goto done;

 cleanup:
        fpga_pci_detach(pl->handle);
        pl->handle = PCI_BAR_HANDLE_INIT;
 done:
        return r;
}

/* cleanup manycore MMIO */
static void hb_mc_platform_mmio_cleanup(hb_mc_platform_t *pl)
{
        int err;

        if (pl->handle == PCI_BAR_HANDLE_INIT)
                return;

        if ((err = fpga_pci_detach(pl->handle)) != 0)
                platform_pr_err(pl, "Failed to cleanup MMIO: %s\n", FPGA_ERR2STR(err));

        pl->handle = PCI_BAR_HANDLE_INIT;
        pl->mmio = (uintptr_t)nullptr;
        pl->id = 0;
        return;
}

/************/
/* MMIO API */
/************/
/**
 * Reads data for MMIO by actualling doing loads
 */
static int  hb_mc_platform_mmio_read_mmio(hb_mc_platform_t *pl, 
                                          uintptr_t offset,
                                          void *vp, size_t sz)
{
        unsigned char *addr = reinterpret_cast<unsigned char *>(pl->mmio);
        uint32_t tmp;

        if (addr == nullptr) {
                platform_pr_err(pl, "%s: Failed: MMIO not initialized", __func__);
                return HB_MC_UNINITIALIZED;
        }

        // check that the address is aligned to a four byte boundar
        if (offset % 4) {
                platform_pr_err(pl, "%s: Failed: 0x%" PRIxPTR " "
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
                platform_pr_err(pl, "%s: Failed: invalid load size (%zu)\n", __func__, sz);
                return HB_MC_INVALID;
        }

        return HB_MC_SUCCESS;
}
/**
 * Reads data for MMIO instead by using PCI ops (used in COSIM)
 */
static int hb_mc_platform_mmio_read_pci(hb_mc_platform_t *pl,
                                        uintptr_t offset,
                                        void *vp, size_t sz)
{
        uint32_t val;
        int err;

        if ((err = fpga_pci_peek(pl->handle, offset, &val)) != 0) {
                platform_pr_err(pl, "%s: Failed: %s\n", __func__, FPGA_ERR2STR(err));
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
                platform_pr_err(pl, "%s: Failed: invalid load size (%zu)\n", __func__, sz);
                return HB_MC_INVALID;
        }
        return HB_MC_SUCCESS;
}

/**
 * Writes data for MMIO instead by  PCI ops (used in COSIM)
 */
static int hb_mc_platform_mmio_write_pci(hb_mc_platform_t *pl, uintptr_t offset,
                                         void *vp, size_t sz)
{
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
                platform_pr_err(pl, "%s: Failed: invalid store size (%zu)\n", __func__, sz);
                return HB_MC_INVALID;
        }

        err = fpga_pci_poke(pl->handle, offset, val);
        if (err != 0) {
                platform_pr_err(pl, "%s: Failed: %s\n", __func__, FPGA_ERR2STR(err));
                return HB_MC_FAIL;
        }
        return HB_MC_SUCCESS;
}


static int hb_mc_platform_mmio_read(hb_mc_platform_t *pl, uintptr_t offset,
                                    void *vp, size_t sz)
{
#if !defined(COSIM)
        return hb_mc_platform_mmio_read_mmio(pl, offset, vp, sz);
#else
        return hb_mc_platform_mmio_read_pci(pl, offset, vp, sz);
#endif
}

/**
 * Writes data for MMIO by direct stores
 */
static int hb_mc_platform_mmio_write_mmio(hb_mc_platform_t *pl, uintptr_t offset,
                                          void *vp, size_t sz)
{
        unsigned char *addr = reinterpret_cast<unsigned char *>(pl->mmio);
        uint32_t tmp;

        if (addr == nullptr) {
                platform_pr_err(pl, "%s: Failed: MMIO not initialized", __func__);
                return HB_MC_UNINITIALIZED;
        }

        // check that the address is aligned to a four byte boundary
        if (offset % 4) {
                platform_pr_err(pl, "%s: Failed: 0x%" PRIxPTR " "
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
                platform_pr_err(pl, "%s: Failed: invalid load size (%zu)\n", __func__, sz);
                return HB_MC_INVALID;
        }

        *(volatile uint32_t *)addr = tmp;

        return HB_MC_SUCCESS;
}
 
static int hb_mc_platform_mmio_write(hb_mc_platform_t *pl, uintptr_t offset,
                                     void *vp, size_t sz)
{
#if !defined(COSIM)
        return hb_mc_platform_mmio_write_mmio(pl, offset, vp, sz);
#else
        return hb_mc_platform_mmio_write_pci(pl, offset, vp, sz);
#endif
}

/**
 * Read one byte from manycore hardware at a given AXI Address
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  offset An  offset into the manycore's MMIO address space
 * @param[out] vp     A byte to be set to the data read
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int hb_mc_platform_mmio_read8(hb_mc_platform_t *pl, uintptr_t offset, uint8_t *vp)
{
        return hb_mc_platform_mmio_read(pl, offset, (void*)vp, 1);
}

/**
 * Read a 16-bit half-word from manycore hardware at a given AXI Address
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  offset An  offset into the manycore's MMIO address space
 * @param[out] vp     A half-word to be set to the data read
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int hb_mc_platform_mmio_read16(hb_mc_platform_t *pl, uintptr_t offset, uint16_t *vp)
{
        return hb_mc_platform_mmio_read(pl, offset, (void*)vp, 2);
}

/**
 * Read a 32-bit word from manycore hardware at a given AXI Address
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  offset An  offset into the manycore's MMIO address space
 * @param[out] vp     A word to be set to the data read
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_platform_mmio_read32(hb_mc_platform_t *pl, uintptr_t offset, uint32_t *vp)
{
        return hb_mc_platform_mmio_read(pl, offset, (void*)vp, 4);
}

/**
 * Write one byte to manycore hardware at a given AXI Address
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  offset An  offset into the manycore's MMIO address space
 * @param[in]  v      A byte value to be written out
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */

int hb_mc_platform_mmio_write8(hb_mc_platform_t *pl, uintptr_t offset, uint8_t v)
{
        return hb_mc_platform_mmio_write(pl, offset, (void*)&v, 1);
}

/**
 * Write a 16-bit half-word to manycore hardware at a given AXI Address
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  offset An  offset into the manycore's MMIO address space
 * @param[in]  v      A half-word value to be written out
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */

int hb_mc_platform_mmio_write16(hb_mc_platform_t *pl, uintptr_t offset, uint16_t v)
{
        return hb_mc_platform_mmio_write(pl, offset, (void*)&v, 2);
}

/**
 * Write a 32-bit word to manycore hardware at a given AXI Address
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  offset An  offset into the manycore's MMIO address space
 * @param[in]  v      A word value to be written out
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int hb_mc_platform_mmio_write32(hb_mc_platform_t *pl, uintptr_t offset, uint32_t v)
{
        return hb_mc_platform_mmio_write(pl, offset, (void*)&v, 4);
}

// ****************************************************************************
// FIFO INTERFACE
// ****************************************************************************

/* get the number of unread packets in a FIFO (rx only) */
static int hb_mc_platform_rx_fifo_get_occupancy(hb_mc_platform_t *pl,
                                                hb_mc_fifo_rx_t type,
                                                uint32_t *occupancy)
{
        uint32_t val, occ;
        uintptr_t occupancy_addr = hb_mc_mmio_fifo_get_addr(type, HB_MC_MMIO_FIFO_RX_OCCUPANCY_OFFSET);
        const char *typestr = hb_mc_fifo_rx_to_string(type);
        int err;
        if (type == HB_MC_FIFO_RX_RSP) {
                platform_pr_err(pl, "RX FIFO Occupancy register %s is disabled!\n", typestr);
                occupancy = NULL;
                return HB_MC_FAIL;
        }
        else {
                err = hb_mc_platform_mmio_read32(pl, occupancy_addr, &val);
                if (err != HB_MC_SUCCESS) {
                        platform_pr_err(pl, "Failed to get %s occupancy\n", typestr);
                        return err;
                }

                // All packets recieved packets should have an integral occupancy that
                // is determined by the Packet Bit-Width / FIFO Bit-Width
                occ = ((sizeof(hb_mc_packet_t) * 8))/HB_MC_MMIO_FIFO_DATA_WIDTH;
                if((occ < ((sizeof(hb_mc_packet_t) * 8))/HB_MC_MMIO_FIFO_DATA_WIDTH) && (val % occ != 0)) {
                        platform_pr_err(pl, "Invalid occupancy: Non-integral packet"
                                        " received from %s\n", typestr);
                        return HB_MC_FAIL;
                }

                *occupancy = (val / occ);
                return HB_MC_SUCCESS;
        }

}

/* read all unread packets from a fifo (rx only) */
static int hb_mc_platform_drain(hb_mc_manycore_t *mc,
                                hb_mc_platform_t *pl,
                                hb_mc_fifo_rx_t type)
{
        const char *typestr = hb_mc_fifo_rx_to_string(type);
        hb_mc_request_packet_t recv;
        uint32_t occupancy;
        int rc;

        for (int drains = 0; drains < 20; drains++) {
                /* first check how many unread packets are currently in the FIFO */
                rc = hb_mc_platform_rx_fifo_get_occupancy(pl, type, &occupancy);
                if (rc != HB_MC_SUCCESS)
                        return rc;

                /* break if occupancy is zero */
                if (occupancy == 0)
                        break;

                /* Read stale packets from fifo */
                for (unsigned i = 0; i < occupancy; i++){
                        rc = hb_mc_platform_receive(mc, (hb_mc_packet_t*) &recv, type, -1);
                        if (rc != HB_MC_SUCCESS) {
                                platform_pr_err(pl, "%s: Failed to read packet from %s fifo\n",
                                                __func__, typestr);
                                return HB_MC_FAIL;
                        }

                        platform_pr_dbg(pl,
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
        rc = hb_mc_platform_rx_fifo_get_occupancy(pl, type, &occupancy);
        if (rc != HB_MC_SUCCESS)
                return HB_MC_FAIL;

        /* fail if new packets have arrived */
        if (occupancy > 0){
                platform_pr_err(pl, "%s: Failed to drain %s fifo: new packets generated\n",
                                __func__, type);
                return HB_MC_FAIL;
        }

        return HB_MC_SUCCESS;
}

#define HB_MC_FIFO_ELS_PER_PACKET (((sizeof(hb_mc_packet_t) * 8))/HB_MC_MMIO_FIFO_DATA_WIDTH)
/* get the number of remaining packets in a host tx FIFO */
static int hb_mc_platform_get_transmit_vacancy(hb_mc_manycore_t *mc,
                                               hb_mc_fifo_tx_t type,
                                               int *vacancy)
{
        hb_mc_platform_t *pl = reinterpret_cast<hb_mc_platform_t *>(mc->platform);

        uint32_t vac;
        uintptr_t vacancy_addr = hb_mc_mmio_fifo_get_addr(type, HB_MC_MMIO_FIFO_TX_VACANCY_OFFSET);
        const char *typestr = hb_mc_fifo_tx_to_string(type);
        int err;

        if (type == HB_MC_FIFO_TX_RSP) {
                platform_pr_err(pl, "TX Response Not Supported!\n", typestr);
                return HB_MC_NOIMPL;
        }
        else {
                err = hb_mc_platform_mmio_read32(pl, vacancy_addr, &vac);
                if (err != HB_MC_SUCCESS) {
                        platform_pr_err(pl, "Failed to get %s vacancy\n", typestr);
                        return err;
                }

                // All packets recieved packets should have an integral vacancy that
                // is determined by the Packet Bit-Width / FIFO Bit-Width
                if(vac % HB_MC_FIFO_ELS_PER_PACKET) {
                        platform_pr_err(pl, "Invalid vacancy: Non-integral packet"
                                        " detected in %s\n", typestr);
                        return HB_MC_FAIL;
                }
                *vacancy = (vac / HB_MC_FIFO_ELS_PER_PACKET);
                if(*vacancy < 0){
                        platform_pr_err(pl, "Invalid vacancy: Negative vacancy %d"
                                        " detected in %s\n", vacancy, typestr);
                        return HB_MC_FAIL;
                }
        }
        return HB_MC_SUCCESS;
}

static int hb_mc_platform_fifos_init(hb_mc_manycore_t *mc,
                                     hb_mc_platform_t *pl)
{
        int rc;

        /* Drain the Manycore-To-Host (RX) Request FIFO */
        rc = hb_mc_platform_drain(mc, pl, HB_MC_FIFO_RX_REQ);
        if (rc != HB_MC_SUCCESS)
                return rc;

        // Initialize the transmit vacancy
        rc = hb_mc_platform_get_transmit_vacancy(mc, HB_MC_FIFO_TX_REQ, &pl->transmit_vacancy);
        if (rc != HB_MC_SUCCESS)
                return rc;
                
        return HB_MC_SUCCESS;
}

static void hb_mc_platform_fifos_cleanup(hb_mc_manycore_t *mc, 
                                         hb_mc_platform_t *pl)
{
        pl->transmit_vacancy = 0;

        /* Drain the Manycore-To-Host (RX) Request FIFO */
        hb_mc_platform_drain(mc, pl, HB_MC_FIFO_RX_REQ);
}



/**
 * Transmit a request packet to manycore hardware
 * @param[in] mc      A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] request A request packet to transmit to manycore hardware
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_transmit(hb_mc_manycore_t *mc,
                                  hb_mc_packet_t *packet,
                                  hb_mc_fifo_tx_t type,
                                  long timeout){

        hb_mc_platform_t *pl = reinterpret_cast<hb_mc_platform_t *>(mc->platform);

        const char *typestr = hb_mc_fifo_tx_to_string(type);
        uintptr_t data_addr;
        int err;

        if (timeout != -1) {
                platform_pr_err(pl, "%s: Only a timeout value of -1 is supported\n",
                                __func__);
                return HB_MC_INVALID;
        }

        // get the address of the transmit data register TDR
        data_addr = hb_mc_mmio_fifo_get_addr(type, HB_MC_MMIO_FIFO_TX_DATA_OFFSET);


        while(pl->transmit_vacancy == 0){
                hb_mc_platform_get_transmit_vacancy(mc, HB_MC_FIFO_TX_REQ, &pl->transmit_vacancy);
        }

        pl->transmit_vacancy--;

        // transmit the data one word at a time
        for (unsigned i = 0; i < array_size(packet->words); i++) {
                err = hb_mc_platform_mmio_write32(pl, data_addr, packet->words[i]);
                if (err != HB_MC_SUCCESS) {
                        return err;
                }
        }

        return HB_MC_SUCCESS;
}

/**
 * Receive a packet from manycore hardware
 * @param[in] mc       A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] response A packet into which data should be read
 * @param[in] timeout  A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_receive(hb_mc_manycore_t *mc,
                           hb_mc_packet_t *packet,
                           hb_mc_fifo_rx_t type,
                           long timeout){
        hb_mc_platform_t *pl = reinterpret_cast<hb_mc_platform_t *>(mc->platform);

        const char *typestr = hb_mc_fifo_rx_to_string(type);
        uintptr_t data_addr;
        uint32_t occupancy;
        int err;

        if (timeout != -1) {
                platform_pr_err(pl, "%s: Only a timeout value of -1 is supported\n",
                                __func__);
                return HB_MC_INVALID;
        }

        data_addr = hb_mc_mmio_fifo_get_addr(type, HB_MC_MMIO_FIFO_RX_DATA_OFFSET);

        if (type == HB_MC_FIFO_RX_REQ) {
                /* wait for a packet */
                do {
                        err = hb_mc_platform_rx_fifo_get_occupancy(pl, type, &occupancy);
                        if (err != HB_MC_SUCCESS) {
                                platform_pr_err(pl, "%s: Failed to get %s FIFO occupancy while waiting for packet: %s\n",
                                                __func__, typestr, hb_mc_strerror(err));
                                return err;
                        }

                } while (occupancy < 1);  // this is packet occupancy, not word occupancy!
        }

        /* read in the packet one word at a time */
        for (unsigned i = 0; i < array_size(packet->words); i++) {
                err = hb_mc_platform_mmio_read32(pl, data_addr, &packet->words[i]);
                if (err != HB_MC_SUCCESS) {
                        platform_pr_err(pl, "%s: Failed read data from %s FIFO: %s\n",
                                        __func__, typestr, hb_mc_strerror(err));
                        return err;
                }
        }
        return HB_MC_SUCCESS;
}

/**
 * Read the configuration register at an index
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  idx    Configuration register index to access
 * @param[out] config Configuration value at index
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_get_config_at(hb_mc_manycore_t *mc, 
                                 unsigned int idx,
                                 hb_mc_config_raw_t *config)
{
        int err;
        uintptr_t addr;
        hb_mc_platform_t *pl = reinterpret_cast<hb_mc_platform_t *>(mc->platform); 

        addr = hb_mc_config_id_to_addr(HB_MC_MMIO_ROM_BASE,
                                       (hb_mc_config_id_t) idx);

        if(idx >= HB_MC_CONFIG_MAX){
                return HB_MC_INVALID;
        }

        err = hb_mc_platform_mmio_read32(pl, addr, config);
        if (err != HB_MC_SUCCESS) {
                platform_pr_err(pl, "%s: Failed to read config word %d from ROM\n",
                                __func__, idx);
                return err;
        }

        return HB_MC_SUCCESS;
}

/**
 * Read the number of remaining manycore network credits
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[out] credits The number of remaining credits
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
static int hb_mc_platform_get_credits(hb_mc_manycore_t *mc, 
                                      int *credits,
                                      long timeout)
{
        uint64_t addr;
        uint32_t val;
        int err;
        hb_mc_platform_t *pl = reinterpret_cast<hb_mc_platform_t *>(mc->platform); 

        if (timeout != -1) {
                platform_pr_err(pl, "%s: Only a timeout value of -1 is supported\n",
                                __func__);
                return HB_MC_NOIMPL;
        }

        addr = hb_mc_mmio_out_credits_get_addr();
        err = hb_mc_platform_mmio_read32(pl, addr, &val);
        if (err != HB_MC_SUCCESS) {
                platform_pr_err(pl, "%s: Failed to read endpoint out credits: %s\n",
                                __func__, hb_mc_strerror(err));
                return err;
        }
        *credits = val;
        return HB_MC_SUCCESS;
}



static int hb_mc_platform_dpi_init(hb_mc_platform_t *pl)
{
#ifdef COSIM
        svScope scope;
        scope = svGetScopeFromName("tb");
        svSetScope(scope);
        pl->scope = scope;
#endif
        return HB_MC_SUCCESS;
}

static void hb_mc_platform_dpi_cleanup(hb_mc_platform_t *pl)
{
#ifdef COSIM
        pl->scope = nullptr;
#endif
        return;
}

/**
 * Clean up the runtime platform
 * @param[in] mc    A manycore to clean up
 */
void hb_mc_platform_cleanup(hb_mc_manycore_t *mc)
{
        hb_mc_platform_t *pl = reinterpret_cast<hb_mc_platform_t *>(mc->platform); 

        hb_mc_platform_fifos_cleanup(mc, pl);

        hb_mc_platform_mmio_cleanup(pl);

        hb_mc_platform_dpi_cleanup(pl);

        pl->name = nullptr;

        delete pl;

        mc->platform = nullptr;

        return;
}

/**
 * Initialize the runtime platform
 * @param[in] mc    A manycore to initialize
 * @param[in] id    ID which selects the physical hardware from which this manycore is configured
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_platform_init(hb_mc_manycore_t *mc,
                        hb_mc_manycore_id_t id){
        // check if platform is already initialized
        if (mc->platform)
                return HB_MC_INITIALIZED_TWICE;

        int r = HB_MC_FAIL, err;

        hb_mc_platform_t *pl = new hb_mc_platform_t();

        if (!pl) {
                platform_pr_err(pl, "%s failed: %m\n", __func__);
                return HB_MC_NOMEM;
        }

        pl->name = mc->name;

        // initialize simulation
        if ((err = hb_mc_platform_dpi_init(pl)) != HB_MC_SUCCESS) {
                delete pl;
                return err;
        }

        // initialize manycore for MMIO
        if ((err = hb_mc_platform_mmio_init(pl, id)) != HB_MC_SUCCESS){
                hb_mc_platform_dpi_cleanup(pl);
                delete pl;
                return err;
        }

        mc->platform = pl;

        // initialize FIFOs
        if ((err = hb_mc_platform_fifos_init(mc, pl)) != HB_MC_SUCCESS){
                mc->platform = nullptr;
                hb_mc_platform_mmio_cleanup(pl);
                hb_mc_platform_dpi_cleanup(pl);
                delete pl;
                return err;
        }

        return HB_MC_SUCCESS;
}


/**
 * Stall until the all requests (and responses) have reached their destination.
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_fence(hb_mc_manycore_t *mc,
                         long timeout)
{
        uint32_t max_vacancy;
        uint32_t max_credits;

        int vacancy;
        int credits;
        int err;

        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        const hb_mc_platform_t *pl = reinterpret_cast<hb_mc_platform_t *>(mc->platform); 

        if (timeout != -1) {
                platform_pr_err(pl, "%s: Only a timeout value of -1 is supported\n",
                                __func__);
                return HB_MC_NOIMPL;
        }

        max_vacancy = hb_mc_config_get_transmit_vacancy_max(cfg);
        max_credits = hb_mc_config_get_io_endpoint_max_out_credits(cfg);

        // wait until out credts are fully resumed, and the tx fifo vacancy equals to host credits
        while ((vacancy != max_vacancy) | (credits != max_credits)) {
                err = hb_mc_platform_get_transmit_vacancy(mc, HB_MC_FIFO_TX_REQ, &vacancy);
                if (err != HB_MC_SUCCESS)
                        return err;
                err = hb_mc_platform_get_credits(mc, &credits, -1);
                if (err != HB_MC_SUCCESS)
                        return err;
        }

        return HB_MC_SUCCESS;
}
