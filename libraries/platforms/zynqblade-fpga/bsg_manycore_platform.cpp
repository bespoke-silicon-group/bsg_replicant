
#include <bsg_manycore.h>
#include <bsg_manycore_platform.h>

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
#define GP0_RD_CREDIT_COUNT      (GP0_RD_MC_RSP_FIFO_CTR   + 0x4  )
#define GP0_RD_ROM_DATA          (GP0_RD_CREDIT_COUNT      + 0x4  )

// GP0 Write Memory Map
#define GP0_WR_CSR_SYS_RESETN     GP0_RD_CSR_SYS_RESETN
#define GP0_WR_CSR_TAG_BITBANG    GP0_RD_CSR_TAG_BITBANG
#define GP0_WR_CSR_DRAM_INITED    GP0_RD_CSR_DRAM_INITED
#define GP0_WR_CSR_DRAM_BASE      GP0_RD_CSR_DRAM_BASE
#define GP0_WR_CSR_ROM_ADDR       GP0_RD_CSR_ROM_ADDR
#define GP0_WR_EP_REQ_FIFO_DATA  (GP0_WR_CSR_DRAM_BASE    + 0x4)
#define GP0_WR_EP_RSP_FIFO_DATA  (GP0_WR_EP_REQ_FIFO_DATA + 0x4)

#define TAG_NUM_CLIENTS 16
#define TAG_MAX_LEN 1
#define TAG_CLIENT_MC_RESET_ID 0
#define TAG_CLIENT_MC_RESET_WIDTH 1

#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <pthread.h>
#include <time.h>
#include <queue>
#include <unistd.h>
#include <bitset>

#include "bp_zynq_pl.h"
#include "bsg_printing.h"
#include "bsg_tag_bitbang.h"
#include "bsg_argparse.h"

#define array_size(x)                           \
        (sizeof(x)/sizeof(x[0]))

/**
 * This file defines the interface that runtime platforms provide to
 * BSG Manycore and CUDA Lite Runtime libraries. This interface
 * provides methods for initialization and cleanup, transmit and
 * receive, fence, and configuration.
 *
 * To support a new platform, define these functions in a new
 * bsg_manycore_platform.cpp file.
 *
 * Editing this file should be VERY rare. Do not add methods to this
 * interface without SERIOUSLY considering the implications.
 *
 */

bp_zynq_pl *zpl;

/**
 * Clean up the runtime platform
 * @param[in] mc    A manycore to clean up
 */
void hb_mc_platform_cleanup(hb_mc_manycore_t *mc)
{
	// Does nothing
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
	bsg_pr_info("ABCD\n");
	zpl = new bp_zynq_pl(NULL, NULL);

  	////bsg_tag_bitbang *btb = new bsg_tag_bitbang(zpl, GP0_WR_CSR_TAG_BITBANG, TAG_NUM_CLIENTS, TAG_MAX_LEN);
  	////bsg_tag_client *mc_reset_client = new bsg_tag_client(TAG_CLIENT_MC_RESET_ID, TAG_CLIENT_MC_RESET_WIDTH);

  	////// Reset the bsg tag master
  	////btb->reset_master();
  	////// Reset bsg client0
  	////btb->reset_client(mc_reset_client);
  	////// Set bsg client0 to 1 (assert BP reset)
  	////btb->set_client(mc_reset_client, 0x1);
  	////// Set bsg client0 to 0 (deassert BP reset)
  	////btb->set_client(mc_reset_client, 0x0);

  	////// We need some additional toggles for data to propagate through
  	////btb->idle(50);
  	////// Deassert the active-low system reset as we finish initializing the whole system
  	////zpl->axil_write(GP0_WR_CSR_SYS_RESETN, 0x1, 0xF);

	return HB_MC_SUCCESS;
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
			    long timeout)
{
        if (timeout != -1) {
                return HB_MC_INVALID;
        }

	// transmit the data one word at a time
	for (unsigned i = 0; i < array_size(packet->words); i++) {
		while(zpl->axil_read(GP0_RD_MC_REQ_FIFO_CTR) == 0);
        	zpl->axil_write(GP0_WR_EP_REQ_FIFO_DATA, packet->words[i], 0xf);
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
			   long timeout)
{
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
	return HB_MC_SUCCESS;
}

/**
 * Stall until the all requests (and responses) have reached their destination.
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_fence(hb_mc_manycore_t *mc, long timeout)
{
	return HB_MC_SUCCESS;
}

/**
 * Signal the hardware to start a bulk transfer over the network
 *
 * Hooks that the platform may implement to perform special
 * initialization and cleanup tasks for assisting in doing
 * packet-based bulk transfers from the host and device.
 *
 * Unless otherwise needed, most platforms can simply return HB_MC_SUCCESS
 *
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_start_bulk_transfer(hb_mc_manycore_t *mc)
{
	return HB_MC_SUCCESS;
}

/**
 * Signal the hardware to end a bulk transfer over the network
 *
 * Hooks that the platform may implement to perform special
 * initialization and cleanup tasks for assisting in doing
 * packet-based bulk transfers from the host and device.
 *
 * Unless otherwise needed, most platforms can simply return HB_MC_SUCCESS
 *
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_finish_bulk_transfer(hb_mc_manycore_t *mc)
{
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
	return HB_MC_SUCCESS;
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
	return HB_MC_SUCCESS;
}

/**
 * Enable trace file generation (vanilla_operation_trace.csv)
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_trace_enable(hb_mc_manycore_t *mc)
{
	return HB_MC_SUCCESS;
}

/**
 * Disable trace file generation (vanilla_operation_trace.csv)
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_trace_disable(hb_mc_manycore_t *mc)
{
	return HB_MC_SUCCESS;
}

/**
 * Enable log file generation (vanilla.log)
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_log_enable(hb_mc_manycore_t *mc)
{
	return HB_MC_SUCCESS;
}

/**
 * Disable log file generation (vanilla.log)
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_log_disable(hb_mc_manycore_t *mc)
{
	return HB_MC_SUCCESS;
}

/**
 * Block until chip reset has completed.
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */        
int hb_mc_platform_wait_reset_done(hb_mc_manycore_t *mc)
{
	return HB_MC_SUCCESS;
}


