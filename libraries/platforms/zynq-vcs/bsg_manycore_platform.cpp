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
#define GP0_ADDR_WIDTH 6
#define GP0_ADDR_BASE  0x43C00000
#define GP0_DATA_WIDTH 32
#define GP0_HIER_BASE                           \
    TOP_NAME.axil0

#ifdef GP1_ENABLE
#undef GP1_ENABLE
#endif

#include <bp_zynq_pl.h>

#include <bsg_nonsynth_dpi_errno.hpp>
#include <bsg_nonsynth_dpi_manycore.hpp>
#include <bsg_nonsynth_dpi_cycle_counter.hpp>
#include <bsg_nonsynth_dpi_clock_gen.hpp>

#include <cstring>
#include <set>
#include <map>
#include <xmmintrin.h>

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

typedef struct hb_mc_platform_t {
    bp_zynq_pl *zynq_pl;
    hb_mc_manycore_id_t id;
    hb_mc_tracer_t tracer;
    //bp_zynq_pl *axi_shell;
} hb_mc_platform_t;

/* read all unread packets from a fifo (rx only) */
int hb_mc_platform_drain(hb_mc_manycore_t *mc, hb_mc_fifo_rx_t type)
{
    return HB_MC_FAIL;
}

// These track active manycore machine IDs, and top-level
// instantiations.
static std::set<hb_mc_manycore_id_t> active_ids;


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
        
    mc->platform = reinterpret_cast<void*>(plt);    
    return HB_MC_SUCCESS;
}

/**
 * Clean up the runtime platform
 * @param[in] mc    A manycore to clean up
 */
void hb_mc_platform_cleanup(hb_mc_manycore_t *mc)
{
    delete reinterpret_cast<hb_mc_platform_t*>(mc->platform);
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
    return HB_MC_FAIL;
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

    return HB_MC_FAIL;
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
    return HB_MC_FAIL;
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
    return HB_MC_FAIL;
}

/**
 * Signal the hardware to start a bulk transfer over the network
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_start_bulk_transfer(hb_mc_manycore_t *mc)
{
    return HB_MC_FAIL;
}

/**
 * Signal the hardware to end a bulk transfer over the network
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_finish_bulk_transfer(hb_mc_manycore_t *mc)
{
    return HB_MC_FAIL;
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
    return HB_MC_FAIL;
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
    return HB_MC_FAIL;
}

