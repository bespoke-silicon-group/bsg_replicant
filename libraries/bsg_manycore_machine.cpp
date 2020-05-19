#include <bsg_manycore_machine.h>
#include <bsg_manycore.h>
#include <bsg_manycore_config.h>
#include <bsg_manycore_printing.h>

#include <verilated.h>
#include <verilated_vcd_c.h>
#include <Vmanycore_tb_top.h>

#include <bsg_nonsynth_dpi_errno.hpp>
#include <bsg_nonsynth_dpi_manycore.hpp>
#include <bsg_nonsynth_dpi_clock_gen.hpp>

#include <cstring>
#include <xmmintrin.h>

/* these are conveniance macros that are only good for one line prints */
#define manycore_pr_dbg(mc, fmt, ...)                   \
        bsg_pr_dbg("%s: " fmt, mc->name, ##__VA_ARGS__)

#define manycore_pr_err(mc, fmt, ...)                   \
        bsg_pr_err("%s: " fmt, mc->name, ##__VA_ARGS__)

#define manycore_pr_warn(mc, fmt, ...)                          \
        bsg_pr_warn("%s: " fmt, mc->name, ##__VA_ARGS__)

#define manycore_pr_info(mc, fmt, ...)                          \
        bsg_pr_info("%s: " fmt, mc->name, ##__VA_ARGS__)

typedef struct machine_t {
        Vmanycore_tb_top * top;
        bsg_nonsynth_dpi::dpi_manycore<HB_MC_CONFIG_MAX> *dpi;
        VerilatedVcdC *trace_object;
        hb_mc_manycore_id_t id;
} machine_t;

/**
 * Transmit a request packet to manycore hardware
 * @param[in] mc      A manycore instance initialized with hb_mc_manycore_init()
 * @param[in] request A request packet to transmit to manycore hardware
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_machine_transmit(hb_mc_manycore_t *mc,
                           hb_mc_packet_t *packet,
                           long timeout)
{
        machine_t *machine = reinterpret_cast<machine_t *>(mc->machine); 
        Vmanycore_tb_top *top = machine->top;
        __m128i *pkt = reinterpret_cast<__m128i*>(packet);

        int err;
        if (timeout != -1) {
                manycore_pr_err(mc, "%s: Only a timeout value of -1 is supported\n",
                                __func__);
                return HB_MC_INVALID;
        }

        do {
                bsg_nonsynth_dpi::bsg_timekeeper::next();
                top->eval();
                err = machine->dpi->tx_req(*pkt);
                //trace_object->dump(sc_time_stamp());
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
int hb_mc_machine_receive(hb_mc_manycore_t *mc,
                          hb_mc_packet_t *packet,
                          long timeout)
{

        static bool order = false;
        int err;
        machine_t *machine = reinterpret_cast<machine_t *>(mc->machine); 
        Vmanycore_tb_top *top = machine->top;
        __m128i *pkt = reinterpret_cast<__m128i*>(packet);

        if (timeout != -1) {
                manycore_pr_err(mc, "%s: Only a timeout value of -1 is supported\n",
                                __func__);
                return HB_MC_INVALID;
        }

        do {
                bsg_nonsynth_dpi::bsg_timekeeper::next();
                top->eval();

                if(order){
                        err = machine->dpi->rx_rsp(*pkt);
                } else {
                        err = machine->dpi->rx_req(*pkt);
                }

                if(err == BSG_NONSYNTH_DPI_NOT_VALID)
                        order ^= 1;

                //trace_object->dump(sc_time_stamp());
        } while (err != BSG_NONSYNTH_DPI_SUCCESS &&
                 (err == BSG_NONSYNTH_DPI_NOT_WINDOW ||
                  err == BSG_NONSYNTH_DPI_NOT_VALID));

        order ^= 1;

        if(err != BSG_NONSYNTH_DPI_SUCCESS){
                manycore_pr_err(mc, "%s: Failed to receive packet: %s\n",
                                __func__, bsg_nonsynth_dpi_strerror(err));
                return HB_MC_INVALID;
        }

        return HB_MC_SUCCESS;
}


/**
 * Read the number of remaining credits of host
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[out] credits The number of remaining credits
 * @param[in] timeout A timeout counter. Unused - set to -1 to wait forever.
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_machine_get_credits(hb_mc_manycore_t *mc, int *credits, long timeout){
        int res;
        machine_t *machine = reinterpret_cast<machine_t *>(mc->machine); 
        Vmanycore_tb_top *top = machine->top;
        if (timeout != -1) {
                manycore_pr_err(mc, "%s: Only a timeout value of -1 is supported\n",
                                __func__);
                return HB_MC_NOIMPL;
        }

        do {
                bsg_nonsynth_dpi::bsg_timekeeper::next();
                top->eval();
                //trace_object->dump(sc_time_stamp());
                res = machine->dpi->get_credits(*credits);
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
 * Read the configuration register at an index
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  idx    Configuration register index to access
 * @param[out] config Configuration value at index
 * @return HB_MC_SUCCESS on success. Otherwise an error code defined in bsg_manycore_errno.h.
 */
int hb_mc_machine_get_config_at(hb_mc_manycore_t *mc, 
                                unsigned int idx,
                                hb_mc_config_raw_t *config)
{
        machine_t *machine = reinterpret_cast<machine_t *>(mc->machine); 

        if(idx < HB_MC_CONFIG_MAX){
                *config = machine->dpi->config[idx];
                return HB_MC_SUCCESS;
        }

        return HB_MC_INVALID;
}

static int hb_mc_machine_dpi_init(machine_t *machine, std::string hierarchy)
{
        svScope scope;
        int credits = 0, err;
        Vmanycore_tb_top *top = machine->top;

        top->eval();

        machine->dpi = new bsg_nonsynth_dpi::dpi_manycore<HB_MC_CONFIG_MAX>(hierarchy);

        return HB_MC_SUCCESS;
}

static void hb_mc_machine_dpi_cleanup(machine_t *machine)
{
        bsg_nonsynth_dpi::dpi_manycore<HB_MC_CONFIG_MAX> *dpi;

        dpi = machine->dpi;

        delete dpi;

        return;
}

/**
 * Initialize the runtime machine
 * @param[in] mc    A manycore to initialize
 * @param[in] id    ID which selects the physical hardware from which this manycore is configured
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_machine_init(hb_mc_manycore_t *mc, hb_mc_manycore_id_t id)
{
        // check if mc is already initialized
        if (mc->machine)
                return HB_MC_INITIALIZED_TWICE;

        int r = HB_MC_FAIL, err;
        
        machine_t *machine = new machine_t;
        std::string hierarchy = "TOP.manycore_tb_top.mc_dpi";
        Vmanycore_tb_top *top = new Vmanycore_tb_top();

        machine->top = top;

        machine->id = id;

        mc->machine = reinterpret_cast<void *>(machine);

        // initialize simulation
        if ((err = hb_mc_machine_dpi_init(machine, hierarchy)) != HB_MC_SUCCESS)
                goto cleanup;

        return HB_MC_SUCCESS;

 cleanup:
        delete top;
        delete machine;
        return err;

}

/**
 * Clean up the runtime machine
 * @param[in] mc    A manycore to clean up
 */
void hb_mc_machine_cleanup(hb_mc_manycore_t *mc)
{
        machine_t *machine = reinterpret_cast<machine_t *>(mc->machine); 

        hb_mc_machine_dpi_cleanup(machine);
        machine->id = 0;

        Vmanycore_tb_top *top = machine->top;

        delete top;

        return;
}
