// Copyright (c) 2020, University of Washington All rights reserved.
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
#define DEBUG
#include <bsg_manycore_platform.h>
#include <bsg_manycore.h>
#include <bsg_manycore_config.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_tracer.hpp>

#ifndef _STRINGIFY
#define _STRINGIFY(x) #x
#endif
#ifndef STRINGIFY
#define STRINGIFY(x) _STRINGIFY(x)
#endif

#define GP0_ENABLE 1
#define GP0_ADDR_WIDTH 9
#define GP0_ADDR_BASE  0x43C00000
#define GP0_DATA_WIDTH 32
#define GP0_HIER_BASE                           \
    TOP_NAME.axil0

#ifdef GP1_ENABLE
#undef GP1_ENABLE
#endif
#include <bp_zynq_pl.h>

#define GP0_RESET_DONE_ADDR                     \
    (GP0_ADDR_BASE + 0x28)

#define GP0_ROM_ADDR_BASE \
    (GP0_ADDR_BASE + 0x2C)

#include <bsg_nonsynth_dpi_errno.hpp>
#include <bsg_nonsynth_dpi_manycore.hpp>
#include <bsg_nonsynth_dpi_cycle_counter.hpp>
#include <bsg_nonsynth_dpi_clock_gen.hpp>

#include <cstring>
#include <set>
#include <map>
#include <xmmintrin.h>
#include <memory>

/* these are convenience macros that are only good for one line prints */
#define manycore_pr_dbg(mc, fmt, ...)                   \
    bsg_pr_dbg("%s: " fmt, mc->name, ##__VA_ARGS__)

#define manycore_pr_err(mc, fmt, ...)                   \
    bsg_pr_err("%s: " fmt, mc->name, ##__VA_ARGS__)

#define manycore_pr_warn(mc, fmt, ...)                  \
    bsg_pr_warn("%s: " fmt, mc->name, ##__VA_ARGS__)

#define manycore_pr_info(mc, fmt, ...)                  \
    bsg_pr_info("%s: " fmt, mc->name, ##__VA_ARGS__)

#define CALL(stmt)                                      \
    do {                                                \
        int __r = stmt;                                 \
        if (__r != HB_MC_SUCCESS) {                     \
            bsg_pr_err("%s: %d: '%s' failed: %s\n"      \
                       , __FILE__                       \
                       , __LINE__                       \
                       , #stmt                          \
                       , hb_mc_strerror(__r));          \
            return __r;                                 \
        }                                               \
    } while (0)

#define CALL_CATCH(stmt, err, label)                    \
    do {                                                \
        int __r = stmt;                                 \
        if (__r != HB_MC_SUCCESS) {                     \
            bsg_pr_err("%s: %d: '%s' failed: %s\n"      \
                       , __FILE__                       \
                       , __LINE__                       \
                       , #stmt                          \
                       , hb_mc_strerror(__r));          \
            err = __r;                                  \
            goto label;                                 \
        }                                               \
    } while (0)

typedef enum fifo_direction {
    TX,
    RX
} fifo_direction_t;

static const char *fifo_direction_to_string(fifo_direction_t dir)
{
    switch (dir) {
    case TX: return "TX";
    case RX: return "RX";
    default: return "unknown";
    }
}

template <typename pkt_t>
const char *packet_type_to_string();

template<>
const char *packet_type_to_string<hb_mc_request_packet_t>()
{
    return "req";
}

template<>
const char *packet_type_to_string<hb_mc_response_packet_t>()
{
    return "rsp";
}

template <fifo_direction_t dir, typename pkt_t>
class fifo {
public:
    fifo(const std::string &name
         , unsigned int gp_axil_base_addr
         , unsigned int data_off
         , unsigned int count_off) :
        _data_addr(gp_axil_base_addr+data_off)
        ,_count_addr(gp_axil_base_addr+count_off)
        ,_name(name) {
        bsg_pr_dbg("fifo(): %s-%s: %s: data address 0x%08x = <gp_axil@0x%08x> + 0x%08x\n"
                   , fifo_direction_to_string(dir)
                   , packet_type_to_string<pkt_t>()
                   , this->name().c_str()
                   , this->data_addr()
                   , gp_axil_base_addr
                   , data_off);

        bsg_pr_dbg("fifo(): %s-%s: %s: count address 0x%08x = <gp_axil@0x%08x> + 0x%08x\n"
                   , fifo_direction_to_string(dir)
                   , packet_type_to_string<pkt_t>()
                   , this->name().c_str()
                   , this->count_addr()
                   , gp_axil_base_addr
                   , count_off);
        
    }
    const std::string & name() const { return _name; }
    unsigned int data_addr()  const { return _data_addr; }
    unsigned int count_addr() const { return _count_addr; }
private:
    unsigned int _data_addr;
    unsigned int _count_addr;
    std::string  _name;
};


using fifo_tx_req_t = fifo<TX,hb_mc_request_packet_t>;
using fifo_tx_rsp_t = fifo<TX,hb_mc_response_packet_t>;
using fifo_rx_req_t = fifo<RX,hb_mc_request_packet_t>;
using fifo_rx_rsp_t = fifo<RX,hb_mc_response_packet_t>;

typedef struct hb_mc_platform_t {
    bp_zynq_pl *zynq_pl;
    hb_mc_manycore_id_t id;
    hb_mc_tracer_t tracer;
    std::unique_ptr<fifo_tx_req_t>  tx_req;
    std::unique_ptr<fifo_tx_rsp_t>  tx_rsp;    
    std::unique_ptr<fifo_rx_req_t>  rx_req;
    std::unique_ptr<fifo_rx_rsp_t>  rx_rsp;
} hb_mc_platform_t;


template <typename pkt_t>
int fifo_tx(bp_zynq_pl *pl, const fifo<TX, pkt_t> &fifo, pkt_t *pkt)
{
    int  n = sizeof(pkt_t)/sizeof(int32_t);
    int *data = reinterpret_cast<int*>(pkt);
    int  written = 0;

    // until complete packet is written
    while (written < n) {
        // check fifo available words
        int avail = pl->axil_read(fifo.count_addr());
        // write available words
        for (int k = 0; k < avail; k++)
            pl->axil_write(fifo.data_addr(), data[written++], 0xf);
    }

    return HB_MC_SUCCESS;
}

template <typename pkt_t>
int fifo_rx(bp_zynq_pl *pl, const fifo<RX, pkt_t> &fifo, pkt_t *pkt)
{
    int n = sizeof(pkt_t)/sizeof(int32_t);
    int *data = reinterpret_cast<int*>(pkt);
    int read = 0;

    // until complete packet is read
    while (read < n) {
        // check fifo available words
        int avail = pl->axil_read(fifo.count_addr());
        avail = std::min(avail, n-read);
        // read available words
        for (int k = 0; k < avail; k++) {
            data[read++] = pl->axil_read(fifo.data_addr());
        }
    }

    return HB_MC_SUCCESS;
}
/* read all unread packets from a fifo (rx only) */
int hb_mc_platform_drain(hb_mc_manycore_t *mc, hb_mc_fifo_rx_t type)
{
    return HB_MC_FAIL;
}

// These track active manycore machine IDs, and top-level
// instantiations.
static std::set<hb_mc_manycore_id_t> active_ids;

/**
 * Initializes FIFOs
 */
static int platform_fifos_init(hb_mc_platform_t *plt)
{

    // initialize
    plt->tx_req = std::unique_ptr<fifo_tx_req_t>(new fifo_tx_req_t("host-req", GP0_ADDR_BASE, 0x10, 0x20));    
    plt->tx_rsp = std::unique_ptr<fifo_tx_rsp_t>(new fifo_tx_rsp_t("host-rsp", GP0_ADDR_BASE, 0x14, 0x24));    
    plt->rx_req = std::unique_ptr<fifo_rx_req_t>(new fifo_rx_req_t("manycore-req", GP0_ADDR_BASE, 0x10, 0x18));
    plt->rx_rsp = std::unique_ptr<fifo_rx_rsp_t>(new fifo_rx_rsp_t("manycore-rsp", GP0_ADDR_BASE, 0x14, 0x1C));
    return HB_MC_SUCCESS;
}

/**
 * Initialize the runtime platform
 * @param[in] mc    A manycore to initialize
 * @param[in] id    ID which selects the physical hardware from which this manycore is configured
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_platform_init(hb_mc_manycore_t *mc, hb_mc_manycore_id_t id)
{
    hb_mc_platform_t *plt = new hb_mc_platform_t;
    bp_zynq_pl *zynq_pl = new bp_zynq_pl(0, nullptr);
    int err = HB_MC_FAIL;
    
    plt->zynq_pl = zynq_pl;
    mc->platform = reinterpret_cast<void*>(plt);

    // initialize fifos
    CALL_CATCH(platform_fifos_init(plt), err, failed);
    return HB_MC_SUCCESS;
    
failed:
    delete zynq_pl;
    delete plt;
    mc->platform = nullptr;
    return err;
}

/**
 * Clean up the runtime platform
 * @param[in] mc    A manycore to clean up
 */
void hb_mc_platform_cleanup(hb_mc_manycore_t *mc)
{
    hb_mc_platform_t *plt = reinterpret_cast<hb_mc_platform_t*>(mc->platform);
    delete plt->zynq_pl;
    delete plt;
    return;
}

/**
 * Transmit a packet to manycore hardware
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
    hb_mc_platform_t *plt = reinterpret_cast<hb_mc_platform_t*>(mc->platform);

    switch (type) {
    case HB_MC_FIFO_TX_REQ:
        return fifo_tx(plt->zynq_pl, *plt->tx_req, &packet->request);
    case HB_MC_FIFO_TX_RSP:
        return fifo_tx(plt->zynq_pl, *plt->tx_rsp, &packet->response);
    default:
        return HB_MC_FAIL;
    }
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
    hb_mc_platform_t *plt = reinterpret_cast<hb_mc_platform_t*>(mc->platform);

    switch (type) {
    case HB_MC_FIFO_RX_REQ:
        return fifo_rx(plt->zynq_pl, *plt->rx_req, &packet->request);
    case HB_MC_FIFO_RX_RSP:
        return fifo_rx(plt->zynq_pl, *plt->rx_rsp, &packet->response);
    default:
        return HB_MC_FAIL;
    }
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
    hb_mc_platform_t *plt = reinterpret_cast<hb_mc_platform_t*>(mc->platform);
    bp_zynq_pl *pl = plt->zynq_pl;
    int val = pl->axil_read(GP0_ROM_ADDR_BASE + sizeof(int)*idx);
    *config = static_cast<hb_mc_config_raw_t>(val);
    return HB_MC_SUCCESS;
}

/**
 * Read the count of credits currently in use
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[out] credits The number of consumed credits
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_get_credits_used(hb_mc_manycore_t *mc, int *credits, long timeout)
{
    return HB_MC_FAIL;
}

/**
 * Read the maximum number of credits available to the host
 * @param[in] mc       A manycore instance initialized with hb_mc_manycore_init()
 * @param[out] credits Maximum number of credits available
 * @param[in] timeout  A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_get_credits_max(hb_mc_manycore_t *mc, int *credits, long timeout)
{

    return HB_MC_FAIL;
}

/**
 * Stall until the all requests (and responses) have reached their destination.
 * @param[in] mc      A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_fence(hb_mc_manycore_t *mc, long timeout)
{
    return HB_MC_SUCCESS;
}

/**
 * Signal the hardware to start a bulk transfer over the network
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_start_bulk_transfer(hb_mc_manycore_t *mc)
{
    return HB_MC_SUCCESS;
}

/**
 * Signal the hardware to end a bulk transfer over the network
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
    return HB_MC_NOIMPL;
}

/**
 * Enable trace file generation (vanilla_operation_trace.csv)
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_trace_enable(hb_mc_manycore_t *mc){
    hb_mc_platform_t *pl = reinterpret_cast<hb_mc_platform_t *>(mc->platform);
    return hb_mc_tracer_trace_enable(pl->tracer);
}

/**
 * Disable trace file generation (vanilla_operation_trace.csv)
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_trace_disable(hb_mc_manycore_t *mc){
    hb_mc_platform_t *pl = reinterpret_cast<hb_mc_platform_t *>(mc->platform);
    return hb_mc_tracer_trace_disable(pl->tracer);
}

/**
 * Enable log file generation (vanilla.log)
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_log_enable(hb_mc_manycore_t *mc){
    hb_mc_platform_t *pl = reinterpret_cast<hb_mc_platform_t *>(mc->platform);
    return hb_mc_tracer_log_enable(pl->tracer);
}

/**
 * Disable log file generation (vanilla.log)
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_log_disable(hb_mc_manycore_t *mc){
    hb_mc_platform_t *pl = reinterpret_cast<hb_mc_platform_t *>(mc->platform);
    return hb_mc_tracer_log_disable(pl->tracer);
}

/**
 * Check if chip reset has completed.
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_wait_reset_done(hb_mc_manycore_t *mc)
{
    hb_mc_platform_t *plt = reinterpret_cast<hb_mc_platform_t*>(mc->platform);
    bp_zynq_pl *pl = plt->zynq_pl;

    int done = pl->axil_read(GP0_RESET_DONE_ADDR);

    // print every some thousand of attempts
    // quit after some even more thousand of attemps
    // this loop should not need to execute more than a few times
    int iter = 0, interval = 100, quit = interval * 10;
    while (done != 1) {
        if (iter % interval == 0)
            manycore_pr_dbg(mc, "%s: calling eval()\n", __func__);

        pl->tick();

        if (iter % interval == 0)
            manycore_pr_dbg(mc, "%s: read %u\n", __func__, static_cast<unsigned>(done));

        done = pl->axil_read(GP0_RESET_DONE_ADDR);
        iter++;

        if (iter >= quit) {
            manycore_pr_err(mc, "%s: reset_done low after %d cycles\n", __func__, iter);
            return HB_MC_TIMEOUT;
        }
    }

    manycore_pr_dbg(mc, "%s: read reset_done after %d cycles\n"
                    , __func__
                    , iter);

    return HB_MC_SUCCESS;
}

