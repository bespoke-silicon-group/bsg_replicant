#include <bsg_manycore_platform.h>
#include <bsg_manycore_machine.h>
#include <bsg_manycore_config.h>
#include <bsg_manycore_coordinate.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_profiler.hpp>
#include <bsg_manycore_tracer.hpp>

#include <cstring>
#include <set>

/////////////////////////////////////////////
// Header Definitions
/////////////////////////////////////////////

#include <bsg_manycore_request_packet.h>
#include <bsg_manycore_response_packet.h>

/////////////////////////////////////////////
// Globals
/////////////////////////////////////////////

#define HOST_BASE_ADDRESS 0x100000
#define HOST_PUTCHAR_REG  (HOST_BASE_ADDRESS + 0x1000)
#define HOST_GETCHAR_REG  (HOST_BASE_ADDRESS + 0x2000)
#define HOST_BOOTROM_REG  (HOST_BASE_ADDRESS + 0x3000)

class bp_mc_link_t {
    private:
                 uint64_t  fifo_base_addr        = (uint64_t) 0x500000;
        volatile uint64_t *bp_req_fifo_data_addr = (volatile uint64_t *) (fifo_base_addr + 0x1000);
        volatile int      *bp_req_fifo_ctr_addr  = (volatile int *) (fifo_base_addr + 0x2000);
        volatile uint64_t *mc_rsp_fifo_data_addr = (volatile uint64_t *) (fifo_base_addr + 0x3000);
        volatile int      *mc_rsp_fifo_ctr_addr  = (volatile int *) (fifo_base_addr + 0x4000);
        volatile uint64_t *mc_req_fifo_data_addr = (volatile uint64_t *) (fifo_base_addr + 0x5000);
        volatile int      *mc_req_fifo_ctr_addr  = (volatile int *) (fifo_base_addr + 0x6000);
        volatile uint64_t *bp_rsp_fifo_data_addr = (volatile uint64_t *) (fifo_base_addr + 0x7000);
        volatile int      *bp_rsp_fifo_ctr_addr  = (volatile int *) (fifo_base_addr + 0x8000);
        volatile int      *endpoint_credits_addr = (volatile int *) (fifo_base_addr + 0x9000);

        int try_write_bp_request_fifo(uint64_t data);
        int try_write_bp_response_fifo(uint64_t data);
        int try_read_mc_response_fifo(uint64_t *data);
        int try_read_mc_request_fifo(uint64_t *data);

    public:
        int tx_fifo_req(hb_mc_request_packet_t *packet);
        int tx_fifo_rsp(hb_mc_response_packet_t *packet);
        int rx_fifo_req(hb_mc_request_packet_t *packet);
        int rx_fifo_rsp(hb_mc_response_packet_t *packet);
        int mmio_read(uint64_t address, int32_t *data);
        int mmio_write(uint64_t address, int32_t data, uint8_t mask);
        int fifo_fence();
        int fifo_drain();
};

int bp_mc_link_t::try_write_bp_request_fifo(uint64_t data)
{
    int ctr = *bp_req_fifo_ctr_addr;

    if (ctr == 0) {
        return -1;
    }

    *bp_req_fifo_data_addr = data;

    return 0;
}

int bp_mc_link_t::try_write_bp_response_fifo(uint64_t data)
{
    int ctr = *bp_rsp_fifo_ctr_addr;

    *bp_rsp_fifo_data_addr = data;

    return 0;
}

int bp_mc_link_t::try_read_mc_response_fifo(uint64_t *data)
{
    int ctr = *mc_rsp_fifo_ctr_addr;

    if (ctr == 0) {
        return -1;
    }

    *data = *mc_rsp_fifo_data_addr;

    return 0;
}

int bp_mc_link_t::try_read_mc_request_fifo(uint64_t *data)
{
    int ctr = *mc_req_fifo_ctr_addr;

    if (ctr == 0) {
        return -1;
    }

    *data = *mc_req_fifo_data_addr;

    return 0;
}

int bp_mc_link_t::tx_fifo_req(hb_mc_request_packet_t *packet)
{
    int packet_len = sizeof(hb_mc_request_packet_t) / 8;

    uint64_t *pkt_data = reinterpret_cast<uint64_t *>(packet);

    bsg_pr_dbg("Transmit (%x,%x)[%x] <- [%x]\n", packet->y_dst, packet->x_dst, packet->addr, packet->payload);

    int i = 0;
    int rc;
    while (i < packet_len) {
        rc = try_write_bp_request_fifo(pkt_data[i]);

        if (rc == 0) {
            i++;
        }
    }

    return HB_MC_SUCCESS;
}

int bp_mc_link_t::tx_fifo_rsp(hb_mc_response_packet_t *packet)
{
    int packet_len = sizeof(hb_mc_response_packet_t) / 8;

    uint64_t *pkt_data = reinterpret_cast<uint64_t *>(packet);

    int i = 0;
    int rc;
    while (i < packet_len) {
        rc = try_write_bp_response_fifo(pkt_data[i]);

        if (rc == 0) {
            i++;
        }
    }

    return HB_MC_SUCCESS;
}

int bp_mc_link_t::rx_fifo_req(hb_mc_request_packet_t *packet)
{
    int packet_len = sizeof(hb_mc_request_packet_t) / 8;

    uint64_t *pkt_data = reinterpret_cast<uint64_t *>(packet);

    int i = 0;
    int rc;
    while (i < packet_len) {
        rc = try_read_mc_request_fifo(&pkt_data[i]);

        if (rc == 0) {
            i++;
        }
    }

    return HB_MC_SUCCESS;
}

int bp_mc_link_t::rx_fifo_rsp(hb_mc_response_packet_t *packet)
{
    int packet_len = sizeof(hb_mc_response_packet_t) / 8;

    uint64_t *pkt_data = reinterpret_cast<uint64_t *>(packet);

    int i = 0;
    int rc;
    while (i < packet_len) {
        rc = try_read_mc_response_fifo(&pkt_data[i]);

        if (rc == 0) {
            i++;
        }
    }

    return HB_MC_SUCCESS;
}

int bp_mc_link_t::mmio_read(uint64_t address, int32_t *data) {
    int32_t *ptr = (int32_t *) address;

    *data = *ptr;

    return HB_MC_SUCCESS;
}

int bp_mc_link_t::mmio_write(uint64_t address, int32_t data, uint8_t mask) {
    if (mask != 0xf) {
        return HB_MC_INVALID;
    }

    int32_t *ptr = (int32_t *) address;
    *ptr = data;

    return HB_MC_SUCCESS;
}

int bp_mc_link_t::fifo_fence() {
    while (*endpoint_credits_addr > 0);

    return HB_MC_SUCCESS;
}

int bp_mc_link_t::fifo_drain() {
    int unused;
    int i = 0;
    bsg_pr_info("Draining Request Fifo\n");
    while (*mc_req_fifo_ctr_addr > 0) {
        unused = *mc_req_fifo_data_addr;
    }

    i = 0;
    bsg_pr_info("Draining Response Fifo\n");
    while (*mc_rsp_fifo_ctr_addr > 0) {
        unused = *mc_rsp_fifo_data_addr;
    }

    return HB_MC_SUCCESS;
}

/////////////////////////////////////////////
// Convenience Functions
/////////////////////////////////////////////

/////////////////////////////////////////////
// Platform Functions
/////////////////////////////////////////////

/**
 * Transmit a request packet to manycore hardware
 * @param[in] mc      A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] request A request packet to transmit to manycore hardware
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_NOIMPL on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_transmit(hb_mc_manycore_t *mc,
        hb_mc_packet_t *packet,
        hb_mc_fifo_tx_t type,
        long timeout)
{
    bp_mc_link_t *mcl = reinterpret_cast<bp_mc_link_t *>(mc->platform);

    if (timeout != -1) {
            return HB_MC_INVALID;
    }

    switch (type) {
        case HB_MC_FIFO_TX_REQ: return mcl->tx_fifo_req((hb_mc_request_packet_t *)packet);
        case HB_MC_FIFO_TX_RSP: return mcl->tx_fifo_rsp((hb_mc_response_packet_t *)packet);
    }
    bsg_pr_info("failed to transmit...\n");

    return HB_MC_NOIMPL;
}

/**
 * Receive a packet from manycore hardware
 * @param[in] mc       A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] response A packet into which data should be read
 * @param[in] timeout  A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_NOIMPL on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_receive(hb_mc_manycore_t *mc,
        hb_mc_packet_t *packet,
        hb_mc_fifo_rx_t type,
        long timeout)
{
    bp_mc_link_t *mcl = reinterpret_cast<bp_mc_link_t *>(mc->platform);

    if (timeout != -1) {
            return HB_MC_INVALID;
    }

    switch (type) {
        case HB_MC_FIFO_RX_REQ: return mcl->rx_fifo_req((hb_mc_request_packet_t *)packet);
        case HB_MC_FIFO_RX_RSP: return mcl->rx_fifo_rsp((hb_mc_response_packet_t *)packet);
    }

    return HB_MC_NOIMPL;
}

/**
 * Read the configuration register at an index
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  idx    Configuration register index to access
 * @param[out] config Configuration value at index
 * @return HB_MC_NOIMPL on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_get_config_at(hb_mc_manycore_t *mc,
        unsigned int idx,
        hb_mc_config_raw_t *config)
{
    bp_mc_link_t *mcl = reinterpret_cast<bp_mc_link_t *>(mc->platform);

    if (idx < HB_MC_CONFIG_MAX) {
        int config_address = hb_mc_config_id_to_addr(HOST_BOOTROM_REG, (hb_mc_config_id_t) idx);
        int config_data;
        int rc = mcl->mmio_read(config_address, &config_data);
        if (rc == HB_MC_SUCCESS) {
            *config = config_data;
        }

        return rc;
    }

    return HB_MC_INVALID;
}

/**
 * Clean up the runtime platform
 * @param[in] mc    A manycore to clean up
 */
void hb_mc_platform_cleanup(hb_mc_manycore_t *mc)
{
    bp_mc_link_t *mcl = reinterpret_cast<bp_mc_link_t *>(mc->platform);

    return;
}

/**
 * Initialize the runtime platform
 * @param[in] mc    A manycore to initialize
 * @param[in] id    ID which selects the physical hardware from which this manycore is configured
 * @return HB_MC_FAIL if an error occured. HB_MC_NOIMPL otherwise.
 */
int hb_mc_platform_init(hb_mc_manycore_t *mc,
        hb_mc_manycore_id_t id)
{
    int err;
    bp_mc_link_t *mcl = new bp_mc_link_t;

    if (mc->platform)
        return HB_MC_INITIALIZED_TWICE;

    if (id != 0) {
        return HB_MC_INVALID;
    }

    mc->platform = reinterpret_cast<void *>(mcl);

    mcl->fifo_drain();

    return HB_MC_SUCCESS;
}


/**
 * Stall until the all requests (and responses) have reached their destination.
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_NOIMPL on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_fence(hb_mc_manycore_t *mc,
        long timeout)
{
    bp_mc_link_t *mcl = reinterpret_cast<bp_mc_link_t *>(mc->platform);

    mcl->fifo_fence();

    return HB_MC_SUCCESS;
}

/**
 * Signal the hardware to start a bulk transfer over the network
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_NOIMPL on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_start_bulk_transfer(hb_mc_manycore_t *mc)
{
    bp_mc_link_t *mcl = reinterpret_cast<bp_mc_link_t *>(mc->platform);

    return HB_MC_SUCCESS;
}

/**
 * Signal the hardware to end a bulk transfer over the network
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_NOIMPL on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_finish_bulk_transfer(hb_mc_manycore_t *mc)
{
    bp_mc_link_t *mcl = reinterpret_cast<bp_mc_link_t *>(mc->platform);

    return HB_MC_SUCCESS;
}

/**
 * Get the current cycle counter of the Manycore Platform
 *
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[out] time   The current counter value.
 * @return HB_MC_NOIMPL on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_get_cycle(hb_mc_manycore_t *mc, uint64_t *time)
{
    bp_mc_link_t *mcl = reinterpret_cast<bp_mc_link_t *>(mc->platform);

    return HB_MC_NOIMPL;
}

/**
 * Get the number of instructions executed for a certain class of instructions
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] itype An enum defining the class of instructions to query.
 * @param[out] count The number of instructions executed in the queried class.
 * @return HB_MC_NOIMPL on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_get_icount(hb_mc_manycore_t *mc, bsg_instr_type_e itype, int *count)
{
    bp_mc_link_t *mcl = reinterpret_cast<bp_mc_link_t *>(mc->platform);

    return HB_MC_NOIMPL;
}

/**
 * Enable trace file generation (vanilla_operation_trace.csv)
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_NOIMPL on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_trace_enable(hb_mc_manycore_t *mc)
{
    bp_mc_link_t *mcl = reinterpret_cast<bp_mc_link_t *>(mc->platform);

    return HB_MC_NOIMPL;
}

/**
 * Disable trace file generation (vanilla_operation_trace.csv)
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_NOIMPL on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_trace_disable(hb_mc_manycore_t *mc)
{
    bp_mc_link_t *mcl = reinterpret_cast<bp_mc_link_t *>(mc->platform);

    return HB_MC_NOIMPL;
}

/**
 * Enable log file generation (vanilla.log)
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_NOIMPL on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_log_enable(hb_mc_manycore_t *mc)
{
    bp_mc_link_t *mcl = reinterpret_cast<bp_mc_link_t *>(mc->platform);

    return HB_MC_NOIMPL;
}

/**
 * Disable log file generation (vanilla.log)
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_NOIMPL on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_log_disable(hb_mc_manycore_t *mc)
{
    bp_mc_link_t *mcl = reinterpret_cast<bp_mc_link_t *>(mc->platform);

    return HB_MC_NOIMPL;
}

/**
 * Check if chip reset has completed.
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_NOIMPL on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_wait_reset_done(hb_mc_manycore_t *mc)
{
    bp_mc_link_t *mcl = reinterpret_cast<bp_mc_link_t *>(mc->platform);

    return HB_MC_SUCCESS;
}

