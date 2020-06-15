#include <bsg_manycore.h>
#include <bsg_mem_dma.hpp>
#include <bsg_manycore_vcache.h>
#include <bsg_manycore_printing.h>
/* these are convenience macros that are only good for one line prints */
#define dma_pr_dbg(mc, fmt, ...)                   \
        bsg_pr_dbg("%s: " fmt, mc->name, ##__VA_ARGS__)

#define dma_pr_err(mc, fmt, ...)                   \
        bsg_pr_err("%s: " fmt, mc->name, ##__VA_ARGS__)

#define dma_pr_warn(mc, fmt, ...)                          \
        bsg_pr_warn("%s: " fmt, mc->name, ##__VA_ARGS__)

#define dma_pr_info(mc, fmt, ...)                          \
        bsg_pr_info("%s: " fmt, mc->name, ##__VA_ARGS__)

using namespace bsg_mem_dma;

/**
 * Given an NPA that maps to DRAM, return a buffer that holds the data for that address.
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t - must be an L2 cache coordinate
 * @param[in]  sz     The number of bytes to write to manycore hardware - used for sanity check
 * @param[out] buffer The valid buffer
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
static int hb_mc_dma_npa_to_buffer(hb_mc_manycore_t *mc, const hb_mc_npa_t *npa, size_t sz,
                                        unsigned char **buffer)
{
        /*
          Our system supports having multiple caches per memory channel.
          Currently, we do this by splitting the channels evenly into even 'banks' for each cache.

          IF THE ADDRESS MAPPING SCHEME FROM CACHES TO DRAM CHANGES THIS FUNCTION WILL BREAK!!!!!

          As of the time of this writing we are in the process of designing the memory system.
          So take note...
        */

        /*
          Get system parameters for performing the address mapping
        */
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        unsigned long caches = hb_mc_vcache_num_caches(mc);
        unsigned long channels = hb_mc_config_get_dram_channels(cfg);
        unsigned long caches_per_channel = caches/channels;

        dma_pr_dbg(mc, "%s: caches = %lu, channels = %lu, caches_per_channel = %lu\n",
                        __func__, caches, channels, caches_per_channel);

        /*
          Figure out which memory channel and bank this NPA maps to.
        */
        hb_mc_idx_t cache_id = hb_mc_config_get_dram_id(cfg, hb_mc_npa_get_xy(npa)); // which cache
        parameter_t id = cache_id / caches_per_channel; // which channel
        parameter_t bank = cache_id % caches_per_channel; // which bank within channel

        /*
          Use the backdoor to our non-synthesizable memory.
        */
        Memory *memory = bsg_mem_dma_get_memory(id);
        parameter_t bank_size = memory->_data.size()/caches_per_channel;

        hb_mc_epa_t epa = hb_mc_npa_get_epa(npa);
        char npa_str[256];

        if (memory == nullptr) {
                dma_pr_err(mc, " %s: Could not get the memory for endpoint at %s\n",
                                __func__, hb_mc_npa_to_string(npa, npa_str, sizeof(npa_str)));

                return HB_MC_FAIL;
        }

        // this is the address that comes out of cache_to_test_dram_tx
        address_t cache_addr = bank*bank_size + epa;
        address_t addr = hb_mc_memsys_map_to_physical_channel_address(&cfg->memsys, cache_addr);


        dma_pr_dbg(mc, "%s: Mapped %s to Channel %2lu, Address 0x%08lx\n",
                        __func__, hb_mc_npa_to_string(npa, npa_str, sizeof(npa_str)), id, addr);

        /*
          Don't overflow memory if you can help it.
        */
        assert(addr + sz <= memory->_data.size());

        *buffer = &memory->_data[addr];

        return HB_MC_SUCCESS;
}

/**
 * Write memory out to manycore DRAM via C++ backdoor
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t - must be an L2 cache coordinate
 * @param[in]  data   A buffer to be written out manycore hardware
 * @param[in]  sz     The number of bytes to write to manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_dma_write(hb_mc_manycore_t *mc,
                    const hb_mc_npa_t *npa,
                    const void *data, size_t sz)
{
        unsigned char *membuffer;
        int err = hb_mc_dma_npa_to_buffer(mc, npa, sz, &membuffer);
        if (err != HB_MC_SUCCESS)
                return err;

        char npa_str[256];

        dma_pr_dbg(mc, "%s: Writing %3zu bytes to %s\n",
                        __func__, sz, hb_mc_npa_to_string(npa, npa_str, sizeof(npa_str)));

        memcpy(reinterpret_cast<void*>(membuffer), data, sz);

        return HB_MC_SUCCESS;
}


/**
 * Read memory from manycore DRAM via C++ backdoor
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t - must be an L2 cache coordinate
 * @param[in]  data   A host buffer to be read into from manycore hardware
 * @param[in]  sz     The number of bytes to read from manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int hb_mc_dma_read(hb_mc_manycore_t *mc, 
                   const hb_mc_npa_t *npa,
                   void *data, size_t sz)
{
        unsigned char *membuffer;
        int err = hb_mc_dma_npa_to_buffer(mc, npa, sz, &membuffer);
        if (err != HB_MC_SUCCESS)
                return err;

        char npa_str[256];

        dma_pr_dbg(mc, "%s: Reading %3zu bytes from %s\n",
                        __func__, sz, hb_mc_npa_to_string(npa, npa_str, sizeof(npa_str)));

        memcpy(data, reinterpret_cast<void*>(membuffer), sz);

        return HB_MC_SUCCESS;
}

