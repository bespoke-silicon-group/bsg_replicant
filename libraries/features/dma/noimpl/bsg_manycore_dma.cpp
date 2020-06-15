#include <bsg_manycore_dma.h>
#include <bsg_manycore.h>
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

/**
 * Write memory out to manycore DRAM via DMA
 * 
 * NOTE: This method is declared with __attribute__((weak)) so that a
 * platform can define write OR read in its own bsg_manycore_dma.cpp
 * implementation, but does not need to declare both.
 *
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t - must be an L2 cache coordinate
 * @param[in]  data   A buffer to be written out manycore hardware
 * @param[in]  sz     The number of bytes to write to manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int __attribute__((weak)) hb_mc_dma_write(hb_mc_manycore_t *mc,
                    const hb_mc_npa_t *npa,
                    const void *data, size_t sz)
{
        dma_pr_err(mc, "%s: This function is not supported on this platform\n",
                        __func__);
        return HB_MC_NOIMPL;
}


/**
 * Read memory from manycore DRAM via DMA
 * 
 * NOTE: This method is declared with __attribute__((weak)) so that a
 * platform can define write OR read in its own bsg_manycore_dma.cpp
 * implementation, but does not need to declare both.
 *
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @param[in]  npa    A valid hb_mc_npa_t - must be an L2 cache coordinate
 * @param[in]  data   A host buffer to be read into from manycore hardware
 * @param[in]  sz     The number of bytes to read from manycore hardware
 * @return HB_MC_FAIL if an error occured. HB_MC_SUCCESS otherwise.
 */
int __attribute__((weak)) hb_mc_dma_read(hb_mc_manycore_t *mc, 
                   const hb_mc_npa_t *npa,
                   void *data, size_t sz)
{
        dma_pr_err(mc, "%s: This function is not supported on this platform\n",
                        __func__);
        return HB_MC_NOIMPL;
}
