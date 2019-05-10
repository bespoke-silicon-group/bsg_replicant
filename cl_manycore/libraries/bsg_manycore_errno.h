#ifndef BSG_MANYCORE_ERRNO
#define BSG_MANYCORE_ERRNO
#ifndef COSIM
#include <bsg_manycore_features.h>
#else
#include "bsg_manycore_features.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

#define HB_MC_SUCCESS (0)
#define HB_MC_FAIL (-1)

#ifdef __cplusplus
}
#endif

#endif
