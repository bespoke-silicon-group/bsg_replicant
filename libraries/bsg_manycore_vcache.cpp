#include <bsg_manycore_printing.h>
#include <bsg_manycore.h>
#include <bsg_manycore_vcache.h>

int hb_mc_manycore_vcache_init(hb_mc_manycore_t *mc)
{
        const hb_mc_config_t *cfg = &mc->config;

        // initialize vcache wh_dest register
        // tells vcaches how to route DMA requests on
        // the wh network to offchip memory
        hb_mc_coordinate_t pod;
        hb_mc_config_foreach_pod(pod, cfg)
        {
                hb_mc_idx_t bx = hb_mc_config_pod_vcore_origin(cfg, pod).x;
                hb_mc_coordinate_t dram;
                hb_mc_config_pod_foreach_dram(dram, pod, cfg)
                {
                        // one pod column -> split pod in half
                        // o/w -> split pod array in half
                        //int east_not_west = cfg->pods.x == 1    ?
                        //        ((dram.x-bx) >= cfg->pod_shape.x/2) :
                        //        (pod.x >= cfg->pods.x/2);

                        // for miniblade, all vcache traffic should go east
                        int east_not_west = 1;

                        hb_mc_npa_t wh_dst_addr = hb_mc_npa(dram, HB_MC_VCACHE_EPA_OFFSET_WH_DST);
                        // cannot write32 here because wh_dest register only supports remote_store opcode
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write8(mc, &wh_dst_addr, east_not_west));

                        hb_mc_npa_t notifi_addr = hb_mc_npa(dram, HB_MC_VCACHE_EPA_OFFSET_NOTIFI);
                        //printf("notification_en = %d, notifi_addr.epa = %X, wh_dst_addr.epa = %X\n", cfg->vcache_notification_en, notifi_addr.epa, wh_dst_addr.epa);
                        // cannot write32 here because wh_dest register only supports remote_store opcode
                        BSG_MANYCORE_CALL(mc, hb_mc_manycore_write8(mc, &notifi_addr, cfg->vcache_notification_en));
                }
        }
        return HB_MC_SUCCESS;
}
