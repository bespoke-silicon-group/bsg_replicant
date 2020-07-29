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
#include <bsg_manycore_profiler.hpp>
#include <bsg_manycore_tracer.hpp>

#include <bsg_manycore_simulator.hpp>

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

#define manycore_pr_warn(mc, fmt, ...)                          \
        bsg_pr_warn("%s: " fmt, mc->name, ##__VA_ARGS__)

#define manycore_pr_info(mc, fmt, ...)                          \
        bsg_pr_info("%s: " fmt, mc->name, ##__VA_ARGS__)

typedef struct hb_mc_platform_t {
        SimulationWrapper *top;
        bsg_nonsynth_dpi::dpi_manycore<HB_MC_CONFIG_MAX> *dpi;
        hb_mc_manycore_id_t id;
        bsg_nonsynth_dpi::dpi_cycle_counter<uint64_t> *ctr;
        hb_mc_profiler_t prof;
        hb_mc_tracer_t tracer;
} hb_mc_platform_t;

/* read all unread packets from a fifo (rx only) */
int hb_mc_platform_drain(hb_mc_manycore_t *mc, hb_mc_fifo_rx_t type)
{

        int err, drains = 0;
        hb_mc_platform_t *platform = reinterpret_cast<hb_mc_platform_t *>(mc->platform); 
        SimulationWrapper *top = platform->top;
        __m128i *pkt;

        hb_mc_config_raw_t cap;
        // Configuration hasn't been initialized yet...
        hb_mc_platform_get_config_at(mc, HB_MC_CONFIG_IO_REMOTE_LOAD_CAP, &cap);

        do {
                top->eval();

                switch(type){
                case HB_MC_FIFO_RX_REQ:
                        err = platform->dpi->rx_req(*pkt);
                        break;
                case HB_MC_FIFO_RX_RSP:
                        err = platform->dpi->rx_rsp(*pkt);
                        break;
                default:
                        manycore_pr_err(mc, "%s: Unknown packet type\n", __func__);
                        return HB_MC_NOIMPL;
                }

                // Only increment on valid packet
                if (err == BSG_NONSYNTH_DPI_SUCCESS)
                        drains ++;
        } while ((err == BSG_NONSYNTH_DPI_NOT_WINDOW  // Not in window
                  || err == BSG_NONSYNTH_DPI_SUCCESS) // Still got a packet
                 && (drains <= cap)); // Still haven't drained the FIFO capacity
        
        if(drains == cap){
                manycore_pr_err(mc, "%s: Failed to drain fifo %s\n", __func__,
                                hb_mc_fifo_rx_to_string(type));
                return HB_MC_FAIL;
        }

        return HB_MC_SUCCESS;
}

static int hb_mc_platform_dpi_init(hb_mc_platform_t *platform, std::string hierarchy)
{
        svScope scope;
        int credits = 0, err;
        SimulationWrapper *top = platform->top;

        top->eval();

        platform->dpi = new bsg_nonsynth_dpi::dpi_manycore<HB_MC_CONFIG_MAX>(hierarchy + ".mc_dpi");
        platform->ctr = new bsg_nonsynth_dpi::dpi_cycle_counter<uint64_t>(hierarchy + ".ctr");

        return HB_MC_SUCCESS;
}

static void hb_mc_platform_dpi_cleanup(hb_mc_platform_t *platform)
{
        bsg_nonsynth_dpi::dpi_manycore<HB_MC_CONFIG_MAX> *dpi;
        bsg_nonsynth_dpi::dpi_cycle_counter<uint64_t> *ctr;

        dpi = platform->dpi;
        ctr = platform->ctr;

        delete dpi;
        delete ctr;

        return;
}

// These track active manycore machine IDs, and top-level
// instantiations.
static std::set<hb_mc_manycore_id_t> active_ids;
static std::map<hb_mc_manycore_id_t,SimulationWrapper*> machines;

/**
 * Clean up the runtime platform
 * @param[in] mc    A manycore to clean up
 */
void hb_mc_platform_cleanup(hb_mc_manycore_t *mc)
{
        hb_mc_platform_t *platform = reinterpret_cast<hb_mc_platform_t *>(mc->platform); 

        hb_mc_tracer_cleanup(&(platform->tracer));

        hb_mc_profiler_cleanup(&(platform->prof));

        hb_mc_platform_dpi_cleanup(platform);

        // Remove the key
        auto key = active_ids.find(platform->id);
        active_ids.erase(key);

        platform->id = 0;

        // Ideally, we would clean up each platform in
        // platforms. However, we can't guarantee that someone won't
        // call init again, so we'll live with the memory leak for
        // now. It's a rare case, since few people call init/cleanup
        // and then continue their program indefinitley.

        return;
}

/**
 * Initialize the runtime platform
 * @param[in] mc    A manycore to initialize
 * @param[in] id    ID which selects the physical hardware from which this manycore is configured
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_platform_init(hb_mc_manycore_t *mc, hb_mc_manycore_id_t id)
{

        int r = HB_MC_FAIL, err;
        
        hb_mc_platform_t *platform = new hb_mc_platform_t;
        std::string hierarchy;
        hb_mc_idx_t x, y;
        hb_mc_config_raw_t rd;

        // check if mc is already initialized
        if (mc->platform)
                return HB_MC_INITIALIZED_TWICE;

        if (id != 0) {
                manycore_pr_err(mc, "Failed to init platform: invalid ID\n");
                return HB_MC_INVALID;
        }

        // Check if the ID has already been initialized
        if(active_ids.find(id) != active_ids.end()){
                manycore_pr_err(mc, "Already initialized ID\n");
                return HB_MC_INVALID;
        }

        active_ids.insert(id);
        platform->id = id;

        // Instantiate the top-level platform simulation and put it in
        // the map. If it has already been instantiated, don't
        // instantiate it again.
        auto m = machines.find(id);
        if(m == machines.end()){
                machines[id] = new SimulationWrapper();
        }
        platform->top = machines[id];

        hierarchy = machines[id]->getRoot();
        mc->platform = reinterpret_cast<void *>(platform);

        // initialize simulation
        err = hb_mc_platform_dpi_init(platform, hierarchy);
        if (err != HB_MC_SUCCESS){
                delete platform;
                return err;
        }

        std::string profiler = hierarchy;
        profiler += ".network.manycore";
        hb_mc_platform_get_config_at(mc, HB_MC_CONFIG_DEVICE_DIM_X, &rd);
        x = rd;
        hb_mc_platform_get_config_at(mc, HB_MC_CONFIG_DEVICE_DIM_Y, &rd);
        y = rd;
        err = hb_mc_profiler_init(&(platform->prof), x, y, profiler);
        if (err != HB_MC_SUCCESS){
                hb_mc_platform_dpi_cleanup(platform);
                delete platform;
                return err;
        }

        err = hb_mc_tracer_init(&(platform->tracer), hierarchy);
        if (err != HB_MC_SUCCESS){
                hb_mc_profiler_cleanup(&(platform->prof));
                hb_mc_platform_dpi_cleanup(platform);
                delete platform;
                return err;
        }

        err = hb_mc_platform_drain(mc, HB_MC_FIFO_RX_REQ);
        if (err != HB_MC_SUCCESS){
                hb_mc_tracer_cleanup(&(platform->tracer));
                hb_mc_profiler_cleanup(&(platform->prof));
                hb_mc_platform_dpi_cleanup(platform);
                delete platform;
                return err;
        }

        hb_mc_platform_drain(mc, HB_MC_FIFO_RX_RSP);
        if (err != HB_MC_SUCCESS){
                hb_mc_tracer_cleanup(&(platform->tracer));
                hb_mc_profiler_cleanup(&(platform->prof));
                hb_mc_platform_dpi_cleanup(platform);
                delete platform;
                return err;
        }
        // Enable assertions for two-state simulators
        platform->top->assertOn(true);
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
        hb_mc_platform_t *platform = reinterpret_cast<hb_mc_platform_t *>(mc->platform); 
        SimulationWrapper *top = platform->top;
        __m128i *pkt = reinterpret_cast<__m128i*>(packet);
        const char *typestr = hb_mc_fifo_tx_to_string(type);

        int err;
        if (timeout != -1) {
                manycore_pr_err(mc, "%s: Only a timeout value of -1 is supported\n",
                                __func__);
                return HB_MC_INVALID;
        }

        if (type == HB_MC_FIFO_TX_RSP) {
                manycore_pr_err(mc, "TX Response Not Supported!\n", typestr);
                return HB_MC_NOIMPL;
        }


        do {
                top->eval();
                err = platform->dpi->tx_req(*pkt);
        } while (err != BSG_NONSYNTH_DPI_SUCCESS &&
                 (err == BSG_NONSYNTH_DPI_NO_CREDITS || 
                  err == BSG_NONSYNTH_DPI_NOT_WINDOW ||
                  err == BSG_NONSYNTH_DPI_NOT_READY    ));

        if(err != BSG_NONSYNTH_DPI_SUCCESS){
                manycore_pr_err(mc, "%s: Failed to transmit packet: %s\n",
                                __func__, bsg_nonsynth_dpi_strerror(err));
                return HB_MC_INVALID;
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

        int err;
        hb_mc_platform_t *platform = reinterpret_cast<hb_mc_platform_t *>(mc->platform); 
        SimulationWrapper *top = platform->top;
        __m128i *pkt = reinterpret_cast<__m128i*>(packet);

        if (timeout != -1) {
                manycore_pr_err(mc, "%s: Only a timeout value of -1 is supported\n",
                                __func__);
                return HB_MC_INVALID;
        }

        do {
                top->eval();

                switch(type){
                case HB_MC_FIFO_RX_REQ:
                        err = platform->dpi->rx_req(*pkt);
                        break;
                case HB_MC_FIFO_RX_RSP:
                        err = platform->dpi->rx_rsp(*pkt);
                        break;
                default:
                        manycore_pr_err(mc, "%s: Unknown packet type\n", __func__);
                        return HB_MC_NOIMPL;
                }

        } while (err != BSG_NONSYNTH_DPI_SUCCESS &&
                 (err == BSG_NONSYNTH_DPI_NOT_WINDOW ||
                  err == BSG_NONSYNTH_DPI_NOT_VALID));

        if(err != BSG_NONSYNTH_DPI_SUCCESS){
                manycore_pr_err(mc, "%s: Failed to receive packet: %s\n",
                                __func__, bsg_nonsynth_dpi_strerror(err));
                return HB_MC_INVALID;
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
        hb_mc_platform_t *platform = reinterpret_cast<hb_mc_platform_t *>(mc->platform); 

        if(idx < HB_MC_CONFIG_MAX){
                *config = platform->dpi->config[idx];
                return HB_MC_SUCCESS;
        }

        return HB_MC_INVALID;
}

/**
 * Read the number of remaining credits of host
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[out] credits The number of remaining credits
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_get_credits(hb_mc_manycore_t *mc, int *credits, long timeout){
        int res;
        hb_mc_platform_t *platform = reinterpret_cast<hb_mc_platform_t *>(mc->platform); 
        SimulationWrapper *top = platform->top;
        if (timeout != -1) {
                manycore_pr_err(mc, "%s: Only a timeout value of -1 is supported\n",
                                __func__);
                return HB_MC_NOIMPL;
        }

        do {
                top->eval();
                res = platform->dpi->get_credits(*credits);
        } while(res == BSG_NONSYNTH_DPI_NOT_WINDOW);

        if(res != BSG_NONSYNTH_DPI_SUCCESS){
                manycore_pr_err(mc, "%s: Unexpected return value.\n",
                                __func__);
                return HB_MC_INVALID;
        }

        if(*credits < 0){
                manycore_pr_err(mc, "%s: Invalid credit value. Must be non-negative\n",
                                __func__, *credits);
                return HB_MC_INVALID;
        }
        
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
        int credits, err = HB_MC_SUCCESS;
        bool isvacant;
        uint32_t max_credits;
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        hb_mc_platform_t *platform = reinterpret_cast<hb_mc_platform_t *>(mc->platform); 

        max_credits = hb_mc_config_get_io_endpoint_max_out_credits(cfg);

        if (timeout != -1) {
                manycore_pr_err(mc, "%s: Only a timeout value of -1 is supported\n",
                                __func__);
                return HB_MC_NOIMPL;
        }

        do {
                err = hb_mc_platform_get_credits(mc, &credits, timeout);
                platform->dpi->tx_is_vacant(isvacant);
        } while(err == HB_MC_SUCCESS && !(credits == max_credits && isvacant));

        return err;
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
        hb_mc_platform_t *platform = reinterpret_cast<hb_mc_platform_t *>(mc->platform); 

        platform->ctr->read(*time);

        return HB_MC_SUCCESS;
}

/**
 * Get the number of instructions executed for a certain class of instructions
 * @param[in] mc    A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] itype An enum defining the class of instructions to query.
 * @param[out] count The number of instructions executed in the queried class.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_get_icount(hb_mc_manycore_t *mc, bsg_instr_type_e itype, int *count){
         hb_mc_platform_t *platform = reinterpret_cast<hb_mc_platform_t *>(mc->platform);

         return hb_mc_profiler_get_icount(platform->prof, itype, count);

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
