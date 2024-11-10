#include <bsg_manycore_dma.h>
#include <bsg_manycore.h>
#include <bsg_manycore_printing.h>
#include <bsg_manycore_platform.hpp>
#include <bsg_manycore_vcache.h>

/* these are convenience macros that are only good for one line prints */
#define dma_pr_dbg(mc, fmt, ...)                   \
        bsg_pr_dbg("%s: " fmt, mc->name, ##__VA_ARGS__)

#define dma_pr_err(mc, fmt, ...)                   \
        bsg_pr_err("%s: " fmt, mc->name, ##__VA_ARGS__)

#define dma_pr_warn(mc, fmt, ...)                          \
        bsg_pr_warn("%s: " fmt, mc->name, ##__VA_ARGS__)

#define dma_pr_info(mc, fmt, ...)                          \
        bsg_pr_info("%s: " fmt, mc->name, ##__VA_ARGS__)

int hb_mc_npa_to_bp_eva(hb_mc_manycore_t *mc,
                        const hb_mc_npa_t *npa,
                        uint64_t *bp_eva) {
        const hb_mc_config_t *cfg = hb_mc_manycore_get_config(mc);
        hb_mc_coordinate_t coord = hb_mc_npa_get_xy(npa);
        hb_mc_coordinate_t npod = hb_mc_config_npod(cfg, coord);
        unsigned long long npody2 = npod.y / 2; // We pack all vcache pods into 2
        hb_mc_epa_t epa = hb_mc_npa_get_epa(npa);

        uint64_t base_eva;
        if (hb_mc_config_is_vanilla_core(cfg, coord)) {
                *bp_eva = 0
                    | (3ULL << 38ULL)
                    | (coord.y << 25ULL)
                    | (coord.x << 18ULL)
                    | (epa << 0ULL);
        } else if (hb_mc_config_is_dram(cfg, coord)) {
                *bp_eva = 0
                    | (2ULL << 38ULL)
                    | (npody2 << 36ULL)
                    | (coord.x << 29ULL)
                    | (epa << 0ULL);
        } else {
                dma_pr_err(mc, "%s: DMA region not supported on this platform\n", __func__);
                return HB_MC_NOIMPL;
        }

    return HB_MC_SUCCESS;
}

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
int hb_mc_dma_write(hb_mc_manycore_t *mc,
                    const hb_mc_npa_t *npa,
                    const void *data, size_t sz)
{
        int rc;
        bp_mc_link_t *mcl = reinterpret_cast<bp_mc_link_t *>(mc->platform);

        uint64_t base_eva;
        if ((rc = hb_mc_npa_to_bp_eva(mc, npa, &base_eva)) != HB_MC_SUCCESS) {
                return rc;
        }

        int32_t *buf = (int32_t *) data;
        for (int i = 0; i < sz; i+=4) {
                uint64_t bp_eva = base_eva + i;
                if ((rc = mcl->mmio_write(bp_eva, buf[i/4], 0xf)) != HB_MC_SUCCESS) {
                        return rc;
                }
        }

        return HB_MC_SUCCESS;
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
int hb_mc_dma_read(hb_mc_manycore_t *mc, 
                   const hb_mc_npa_t *npa,
                   void *data, size_t sz)
{
        int rc;
        bp_mc_link_t *mcl = reinterpret_cast<bp_mc_link_t *>(mc->platform);

        uint64_t base_eva;
        if ((rc = hb_mc_npa_to_bp_eva(mc, npa, &base_eva)) != HB_MC_SUCCESS) {
                return rc;
        }

        int32_t *buf = (int32_t *) data;
        for (int i = 0; i < sz; i+=4) {
                uint64_t bp_eva = base_eva + i;
                if ((rc = mcl->mmio_read(bp_eva, &buf[i/4])) != HB_MC_SUCCESS) {
                        return rc;
                }
        }

        return HB_MC_SUCCESS;
}

int hb_mc_dma_init(hb_mc_manycore_t *mc)
{
        mc->config.memsys.dma2cache = 1;
        return HB_MC_SUCCESS;
}

