#include <bsg_manycore_platform.h>
#include <bsg_manycore_mmio.h>
#include <bsg_manycore_config.h>
#include <bsg_manycore_coordinate.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_profiler.hpp>
#include <bsg_manycore_tracer.hpp>

#include <cstring>
#include <set>

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
        int transmit_vacancy;    //!< Software copy of the transmit vacancy register
        int handle; //!< pci bar handle
        hb_mc_manycore_id_t id;  //!< which manycore instance is this
        hb_mc_mmio_t      mmio;  //!< pointer to memory mapped io (F1-specific)
        hb_mc_profiler_t  prof;  //!< Profiler Implementation
        hb_mc_tracer_t    tracer; //!< Tracer Implementation
} hb_mc_platform_t;


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
                err = hb_mc_mmio_read32(pl->mmio, occupancy_addr, &val);
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
                err = hb_mc_mmio_read32(pl->mmio, vacancy_addr, &vac);
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
                err = hb_mc_mmio_write32(pl->mmio, data_addr, packet->words[i]);
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
                err = hb_mc_mmio_read32(pl->mmio, data_addr, &packet->words[i]);
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

        err = hb_mc_mmio_read32(pl->mmio, addr, config);
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
        err = hb_mc_mmio_read32(pl->mmio, addr, &val);
        if (err != HB_MC_SUCCESS) {
                platform_pr_err(pl, "%s: Failed to read endpoint out credits: %s\n",
                                __func__, hb_mc_strerror(err));
                return err;
        }
        *credits = val;
        return HB_MC_SUCCESS;
}



// This track active manycore machine IDs
static std::set<hb_mc_manycore_id_t> active_ids;

/**
 * Clean up the runtime platform
 * @param[in] mc    A manycore to clean up
 */
void hb_mc_platform_cleanup(hb_mc_manycore_t *mc)
{
        hb_mc_platform_t *pl = reinterpret_cast<hb_mc_platform_t *>(mc->platform); 

        hb_mc_tracer_cleanup(&(pl->tracer));

        hb_mc_profiler_cleanup(&(pl->prof));

        hb_mc_platform_fifos_cleanup(mc, pl);

        hb_mc_mmio_cleanup(&pl->mmio, &pl->handle);

        pl->name = nullptr;

        // Remove the key
        auto key = active_ids.find(pl->id);
        active_ids.erase(key);

        pl->id = 0;

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
        std::string hierarchy = "tb.card.fpga.CL";
        hb_mc_idx_t x, y;
        hb_mc_config_raw_t rd;

        hb_mc_platform_t *pl = new hb_mc_platform_t();

        if (!pl) {
                platform_pr_err(pl, "%s failed: %m\n", __func__);
                return HB_MC_NOMEM;
        }

        pl->name = mc->name;

        // Check if the ID has already been initialized
        if(active_ids.find(id) != active_ids.end()){
                platform_pr_err(pl, "Already initialized ID\n");
                return HB_MC_INVALID;
        }

        active_ids.insert(id);

        pl->id = id;

        // initialize manycore for MMIO
        if ((err = hb_mc_mmio_init(&pl->mmio, (int*)&pl->handle, id)) != HB_MC_SUCCESS){
                delete pl;
                active_ids.erase(active_ids.find(pl->id));
                return err;
        }

        mc->platform = pl;

        // initialize FIFOs
        if ((err = hb_mc_platform_fifos_init(mc, pl)) != HB_MC_SUCCESS){
                mc->platform = nullptr;
                hb_mc_mmio_cleanup(&pl->mmio, &pl->handle);
                active_ids.erase(active_ids.find(pl->id));
                delete pl;
                return err;
        }

        std::string profiler = hierarchy + ".network.manycore_wrapper.manycore";
        hb_mc_platform_get_config_at(mc, HB_MC_CONFIG_DEVICE_DIM_X, &rd);
        x = rd;
        hb_mc_platform_get_config_at(mc, HB_MC_CONFIG_DEVICE_DIM_Y, &rd);
        y = rd;
        err = hb_mc_profiler_init(&(pl->prof), x, y, profiler);
        // This feature MIGHT not be implemented, so if it doesn't
        // work, just ignore.
        if (err != HB_MC_SUCCESS && err != HB_MC_NOIMPL){
                hb_mc_platform_fifos_cleanup(mc, pl);
                mc->platform = nullptr;
                hb_mc_mmio_cleanup(&pl->mmio, &pl->handle);
                active_ids.erase(active_ids.find(pl->id));
                delete pl;
                return err;
        }

        err = hb_mc_tracer_init(&(pl->tracer), hierarchy);
        // This feature MIGHT not be implemented, so if it doesn't
        // work, just ignore.
        if (err != HB_MC_SUCCESS && err != HB_MC_NOIMPL){
                hb_mc_profiler_cleanup(&(pl->prof));
                hb_mc_platform_fifos_cleanup(mc, pl);
                mc->platform = nullptr;
                hb_mc_mmio_cleanup(&pl->mmio, &pl->handle);
                active_ids.erase(active_ids.find(pl->id));
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


/**
 * Get the current cycle counter of the Manycore Platform
 *
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[out] time   The current counter value.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_platform_get_cycle(hb_mc_manycore_t *mc, uint64_t *time)
{
        const hb_mc_platform_t *pl = reinterpret_cast<hb_mc_platform_t *>(mc->platform); 

        int err;
        uint64_t retval;
        uint32_t lo, hi;

        if(time == nullptr){
                platform_pr_err(pl, "%s: Nullptr provided as argument time\n",
                                __func__);
                return HB_MC_INVALID;
        }

        err = hb_mc_mmio_read32(pl->mmio, HB_MC_MMIO_CYCLE_CTR_LO_OFFSET, &lo);
        if (err != HB_MC_SUCCESS) {
                platform_pr_err(pl, "%s: Failed to read LOW bits of cycle counter: %s\n",
                                __func__, hb_mc_strerror(err));
                return err;
        }

        err = hb_mc_mmio_read32(pl->mmio, HB_MC_MMIO_CYCLE_CTR_HI_OFFSET, &hi);
        if (err != HB_MC_SUCCESS) {
                platform_pr_err(pl, "%s: Failed to read high bits of cycle counter: %s\n",
                                __func__, hb_mc_strerror(err));
                return err;
        }

        retval = (static_cast<uint64_t>(lo) |  (static_cast<uint64_t>(hi) << 32));
        
        *time = retval;
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
        hb_mc_platform_t *pl = reinterpret_cast<hb_mc_platform_t *>(mc->platform);

        return hb_mc_profiler_get_icount(pl->prof, itype, count);
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
