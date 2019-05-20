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

#define HB_MC_SUCCESS       (0)
#define HB_MC_FAIL          (-1)
#define HB_MC_TIMEOUT       (-2)
#define HB_MC_UNINITIALIZED (-3)
#define HB_MC_INVALID       (-4)

static inline const char * hb_mc_strerror(int err)
{
        static const char *strtab [] = {
                [-HB_MC_SUCCESS] = "Success",
                [-HB_MC_FAIL]    = "Failure",
                [-HB_MC_TIMEOUT] = "Timeout",
		[-HB_MC_UNINITIALIZED] = "Manycore not initialized",
		[-HB_MC_INVALID] = "Invalid input",
        };
        return strtab[-err];
}

#ifdef __cplusplus
}
#endif

#endif
