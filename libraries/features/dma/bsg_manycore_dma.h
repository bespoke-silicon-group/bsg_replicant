#ifndef __BSG_MANYCORE_DMA_HPP
#define __BSG_MANYCORE_DMA_HPP

#include <bsg_manycore.h>
#include <bsg_manycore_npa.h>
#include <bsg_manycore_config.h>

/**
 * Check if DMA writing is supported.
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return One if DMA writing is supported - Zero otherwise.
 */
static inline int hb_mc_dma_supports_write(const hb_mc_manycore_t *mc)
{
        return hb_mc_config_memsys_feature_dma(hb_mc_manycore_get_config(mc)) == 1;
}

/**
 * Check if DMA reading is supported.
 * @param[in]  mc     A manycore instance initialized with hb_mc_manycore_init()
 * @return One if DMA reading is supported - Zero otherwise.
 */
static inline int hb_mc_dma_supports_read(const hb_mc_manycore_t *mc)
{
        return hb_mc_config_memsys_feature_dma(hb_mc_manycore_get_config(mc)) == 1;
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
                   void *data, size_t sz);

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
                    const void *data, size_t sz);

#endif
