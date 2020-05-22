/**
 * Write memory out to manycore DRAM via DMA
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
        manycore_pr_err(mc, "%s: This function is not supported on this platform\n",
                        __func__);
        return HB_MC_NOIMPL;
}


/**
 * Read memory from manycore DRAM via DMA
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
        manycore_pr_err(mc, "%s: This function is not supported on this platform\n",
                        __func__);
        return HB_MC_NOIMPL;
}
