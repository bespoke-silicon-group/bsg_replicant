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

#define GP0_ENABLE
#define GP0_ADDR_BASE 0x400000000
#define GP0_ADDR_WIDTH 10
#define GP0_DATA_WIDTH 32
#define GP0_HIER_BASE ""

#define _BSG_PRINTING_H
#include <bsg_zynq_pl.h>
#include <bsg_tag_bitbang.h>

#include <bsg_manycore_request_packet.h>
#include <bsg_manycore_response_packet.h>


// GP0 Read Memory Map
#define GP0_RD_CSR_SYS_RESETN    (GP0_ADDR_BASE                   )
#define GP0_RD_CSR_TAG_BITBANG   (GP0_RD_CSR_SYS_RESETN    + 0x4  )
#define GP0_RD_CSR_DRAM_INITED   (GP0_RD_CSR_TAG_BITBANG   + 0x4  )
#define GP0_RD_CSR_DRAM_BASE     (GP0_RD_CSR_DRAM_INITED   + 0x4  )
#define GP0_RD_CSR_ROM_ADDR      (GP0_RD_CSR_DRAM_BASE     + 0x4  )
#define GP0_RD_MC_REQ_FIFO_DATA  (GP0_RD_CSR_ROM_ADDR      + 0x4  )
#define GP0_RD_MC_RSP_FIFO_DATA  (GP0_RD_MC_REQ_FIFO_DATA  + 0x4  )
#define GP0_RD_MC_REQ_FIFO_CTR   (GP0_RD_MC_RSP_FIFO_DATA  + 0x4  )
#define GP0_RD_MC_RSP_FIFO_CTR   (GP0_RD_MC_REQ_FIFO_CTR   + 0x4  )
#define GP0_RD_EP_REQ_FIFO_CTR   (GP0_RD_MC_RSP_FIFO_CTR   + 0x4  )
#define GP0_RD_EP_RSP_FIFO_CTR   (GP0_RD_EP_REQ_FIFO_CTR   + 0x4  )
#define GP0_RD_CREDIT_COUNT      (GP0_RD_EP_RSP_FIFO_CTR   + 0x4  )
#define GP0_RD_ROM_DATA          (GP0_RD_CREDIT_COUNT      + 0x4  )

// GP0 Write Memory Map
#define GP0_WR_CSR_SYS_RESETN     GP0_RD_CSR_SYS_RESETN
#define GP0_WR_CSR_TAG_BITBANG    GP0_RD_CSR_TAG_BITBANG
#define GP0_WR_CSR_DRAM_INITED    GP0_RD_CSR_DRAM_INITED
#define GP0_WR_CSR_DRAM_BASE      GP0_RD_CSR_DRAM_BASE
#define GP0_WR_CSR_ROM_ADDR       GP0_RD_CSR_ROM_ADDR
#define GP0_WR_EP_REQ_FIFO_DATA  (GP0_WR_CSR_ROM_ADDR + 0x4)
#define GP0_WR_EP_RSP_FIFO_DATA  (GP0_WR_EP_REQ_FIFO_DATA + 0x4)

#define TAG_NUM_CLIENTS 16
#define TAG_MAX_LEN 1
#define TAG_CLIENT_MC_RESET_ID 0
#define TAG_CLIENT_MC_RESET_WIDTH 1

/////////////////////////////////////////////
// Globals
/////////////////////////////////////////////
bsg_zynq_pl *zpl;

/////////////////////////////////////////////
// Convenience Functions
/////////////////////////////////////////////

inline void send_mc_request_packet(bsg_zynq_pl *zpl, hb_mc_request_packet_t *packet) {
  int axil_len = sizeof(hb_mc_request_packet_t) / 4;

  uint32_t *pkt_data = reinterpret_cast<uint32_t *>(packet);
  for (int i = 0; i < axil_len; i++) {
    while (!zpl->axil_read(GP0_RD_EP_REQ_FIFO_CTR));
    zpl->axil_write(GP0_WR_EP_REQ_FIFO_DATA, pkt_data[i], 0xf);
  }
}

inline void recv_mc_response_packet(bsg_zynq_pl *zpl, hb_mc_response_packet_t *packet) {
  int axil_len = sizeof(hb_mc_response_packet_t) / 4;

  uint32_t *pkt_data = reinterpret_cast<uint32_t *>(packet);
  for (int i = 0; i < axil_len; i++) {
    while (!zpl->axil_read(GP0_RD_MC_RSP_FIFO_CTR));
    pkt_data[i] = zpl->axil_read(GP0_RD_MC_RSP_FIFO_DATA);
  }
}

inline void recv_mc_request_packet(bsg_zynq_pl *zpl, hb_mc_request_packet_t *packet) {
  int axil_len = sizeof(hb_mc_request_packet_t) / 4;

  uint32_t *pkt_data = reinterpret_cast<uint32_t *>(packet);
  for (int i = 0; i < axil_len; i++) {
    while (!zpl->axil_read(GP0_RD_MC_REQ_FIFO_CTR));
    pkt_data[i] = zpl->axil_read(GP0_RD_MC_REQ_FIFO_DATA);
  }
}

inline void send_mc_write(bsg_zynq_pl *zpl, uint8_t x, uint8_t y, uint32_t epa, int32_t data) {
  hb_mc_request_packet_t req_pkt;

  req_pkt.op_v2   = 2; // SW
  req_pkt.reg_id  = 0xff; // unused
  req_pkt.payload = data;
  req_pkt.x_src   = BSG_MANYCORE_MACHINE_HOST_COORD_X;
  req_pkt.y_src   = BSG_MANYCORE_MACHINE_HOST_COORD_Y;
  req_pkt.x_dst   = x;
  req_pkt.y_dst   = y;
  req_pkt.addr    = epa >> 2;

  send_mc_request_packet(zpl, &req_pkt);
}

inline int32_t send_mc_read(bsg_zynq_pl *zpl, uint8_t x, uint8_t y, uint32_t epa) {
  hb_mc_request_packet_t req_pkt;

  req_pkt.op_v2   = 0; // LD
  req_pkt.reg_id  = 0xff; // unused
  req_pkt.payload = 0; // Ignore payload
  req_pkt.x_src   = BSG_MANYCORE_MACHINE_HOST_COORD_X;
  req_pkt.y_src   = BSG_MANYCORE_MACHINE_HOST_COORD_Y;
  req_pkt.x_dst   = x;
  req_pkt.y_dst   = y;
  req_pkt.addr    = epa >> 2;

  send_mc_request_packet(zpl, &req_pkt);

  hb_mc_response_packet_t resp_pkt;
  recv_mc_response_packet(zpl, &resp_pkt);

  return resp_pkt.data;
}

/////////////////////////////////////////////
// Platform Functions
/////////////////////////////////////////////

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
                            long timeout)
{
        bsg_zynq_pl *zpl = reinterpret_cast<bsg_zynq_pl *>(mc->platform);

        const char *typestr = hb_mc_fifo_tx_to_string(type);

        int err;
        if (timeout != -1) {
                bsg_pr_err("%s: Only a timeout value of -1 is supported\n",
                                __func__);
                return HB_MC_INVALID;
        }

        if (type == HB_MC_FIFO_TX_RSP) {
                bsg_pr_err("TX Response Not Supported!\n", typestr);
                return HB_MC_NOIMPL;
        }

	send_mc_request_packet(zpl, (hb_mc_request_packet_t *)packet);

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
                           long timeout)
{
        bsg_zynq_pl *zpl = reinterpret_cast<bsg_zynq_pl *>(mc->platform);

        if (timeout != -1) {
                bsg_pr_err("%s: Only a timeout value of -1 is supported\n",
                                __func__);
                return HB_MC_INVALID;
        }

	switch (type) {
		case HB_MC_FIFO_RX_REQ:
			recv_mc_request_packet(zpl, (hb_mc_request_packet_t *)packet);
			break;
		case HB_MC_FIFO_RX_RSP:
			recv_mc_response_packet(zpl, (hb_mc_response_packet_t *)packet);
		default:
			bsg_pr_err("%s: Unknown packet type\n", __func__);
			return HB_MC_NOIMPL;
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
	bsg_zynq_pl *zpl = reinterpret_cast<bsg_zynq_pl *>(mc->platform);

	if (idx < HB_MC_CONFIG_MAX) {
		zpl->axil_write(GP0_WR_CSR_ROM_ADDR, idx, 0xf);
		*config = zpl->axil_read(GP0_RD_ROM_DATA);
        	return HB_MC_SUCCESS;
	}

	return HB_MC_INVALID;
}

/**
 * Clean up the runtime platform
 * @param[in] mc    A manycore to clean up
 */
void hb_mc_platform_cleanup(hb_mc_manycore_t *mc)
{
	bsg_zynq_pl *zpl = reinterpret_cast<bsg_zynq_pl *>(mc->platform);

	delete zpl;

        return;
}

/**
 * Initialize the runtime platform
 * @param[in] mc    A manycore to initialize
 * @param[in] id    ID which selects the physical hardware from which this manycore is configured
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_platform_init(hb_mc_manycore_t *mc,
                        hb_mc_manycore_id_t id)
{
	int err;
	zpl = new bsg_zynq_pl(0, NULL);

	if (mc->platform)
		return HB_MC_INITIALIZED_TWICE;

        if (id != 0) {
                return HB_MC_INVALID;
        }

	mc->platform = reinterpret_cast<void *>(zpl);

	bsg_tag_bitbang *btb = new bsg_tag_bitbang(zpl, GP0_WR_CSR_TAG_BITBANG, TAG_NUM_CLIENTS, TAG_MAX_LEN);
	bsg_tag_client *mc_reset_client = new bsg_tag_client(TAG_CLIENT_MC_RESET_ID, TAG_CLIENT_MC_RESET_WIDTH);

	// Reset the bsg tag master
	btb->reset_master();
	// Reset bsg client0
	btb->reset_client(mc_reset_client);
	// Set bsg client0 to 1 (assert BP reset)
	btb->set_client(mc_reset_client, 0x1);
	// Set bsg client0 to 0 (deassert BP reset)
	btb->set_client(mc_reset_client, 0x0);
	
	// We need some additional toggles for data to propagate through
	btb->idle(50);
	// Deassert the active-low system reset as we finish initializing the whole system
	zpl->axil_write(GP0_WR_CSR_SYS_RESETN, 0x1, 0xF);

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
        bsg_zynq_pl *zpl = reinterpret_cast<bsg_zynq_pl *>(mc->platform);

        return HB_MC_SUCCESS;
}

/**
 * Signal the hardware to start a bulk transfer over the network
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_start_bulk_transfer(hb_mc_manycore_t *mc)
{
        bsg_zynq_pl *zpl = reinterpret_cast<bsg_zynq_pl *>(mc->platform);

        return HB_MC_SUCCESS;
}

/**
 * Signal the hardware to end a bulk transfer over the network
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_finish_bulk_transfer(hb_mc_manycore_t *mc)
{
        bsg_zynq_pl *zpl = reinterpret_cast<bsg_zynq_pl *>(mc->platform);

        return HB_MC_SUCCESS;
}

/**
 * Get the current cycle counter of the Manycore Platform
 *
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[out] time   The current counter value.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_get_cycle(hb_mc_manycore_t *mc, uint64_t *time)
{
        bsg_zynq_pl *zpl = reinterpret_cast<bsg_zynq_pl *>(mc->platform);

	return HB_MC_NOIMPL;
}

/**
 * Get the number of instructions executed for a certain class of instructions
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] itype An enum defining the class of instructions to query.
 * @param[out] count The number of instructions executed in the queried class.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_get_icount(hb_mc_manycore_t *mc, bsg_instr_type_e itype, int *count)
{
        bsg_zynq_pl *zpl = reinterpret_cast<bsg_zynq_pl *>(mc->platform);

        return HB_MC_NOIMPL;
}

/**
 * Enable trace file generation (vanilla_operation_trace.csv)
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_trace_enable(hb_mc_manycore_t *mc)
{
        bsg_zynq_pl *zpl = reinterpret_cast<bsg_zynq_pl *>(mc->platform);

        return HB_MC_NOIMPL;
}

/**
 * Disable trace file generation (vanilla_operation_trace.csv)
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_trace_disable(hb_mc_manycore_t *mc)
{
        bsg_zynq_pl *zpl = reinterpret_cast<bsg_zynq_pl *>(mc->platform);

        return HB_MC_NOIMPL;
}

/**
 * Enable log file generation (vanilla.log)
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_log_enable(hb_mc_manycore_t *mc)
{
        bsg_zynq_pl *zpl = reinterpret_cast<bsg_zynq_pl *>(mc->platform);

        return HB_MC_NOIMPL;
}

/**
 * Disable log file generation (vanilla.log)
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_log_disable(hb_mc_manycore_t *mc)
{
        bsg_zynq_pl *zpl = reinterpret_cast<bsg_zynq_pl *>(mc->platform);

        return HB_MC_NOIMPL;
}

/**
 * Check if chip reset has completed.
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_wait_reset_done(hb_mc_manycore_t *mc)
{
        bsg_zynq_pl *zpl = reinterpret_cast<bsg_zynq_pl *>(mc->platform);

        return HB_MC_SUCCESS;
}

