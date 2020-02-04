#pragma once
#include <bsg_manycore_features.h>
#include <stdint.h>

#if defined(COSIM)
extern void sv_set_virtual_dip_switch(int, int);

#define HB_MC_VDIP_CLK_MUX          0x1
#define HB_MC_VDIP_DRAMSIM_TIMING_N 0x2

#endif
