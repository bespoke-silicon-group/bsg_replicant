#ifndef BSG_MANYCORE_CHIP_ID_H
#define BSG_MANYCORE_CHIP_ID_H
#ifdef __cplusplus
extern "C" {
#endif

#define HB_MC_CHIP_ID_MASTER   0x00000000
#define HB_MC_CHIP_ID_BIGBLADE 0xbb1ade00

#define HB_MC_IS_CHIP_ID(id)                    \
    ((id) == HB_MC_CHIP_ID_MASTER ||            \
     (id) == HB_MC_CHIP_ID_BIGBLADE )

#ifdef __cplusplus
}
#endif
#endif


