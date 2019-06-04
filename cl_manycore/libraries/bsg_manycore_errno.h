#ifndef BSG_MANYCORE_ERRNO
#define BSG_MANYCORE_ERRNO
#include <bsg_manycore_features.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HB_MC_SUCCESS       (0)
#define HB_MC_FAIL          (-1)
#define HB_MC_TIMEOUT       (-2)
#define HB_MC_UNINITIALIZED (-3)
#define HB_MC_INVALID       (-4)
#define HB_MC_INITIALIZED_TWICE (-4) // same as invalid
#define HB_MC_NOMEM         (-5)
#define HB_MC_NOIMPL        (-6)
#define HB_MC_NOTFOUND      (-7)
#define HB_MC_BUSY          (-8)

static inline const char * hb_mc_strerror(int err)
{
        static const char *strtab [] = {
                [-HB_MC_SUCCESS]           = "Success",
                [-HB_MC_FAIL]              = "Failure",
                [-HB_MC_TIMEOUT]           = "Timeout",
		[-HB_MC_UNINITIALIZED]     = "Not initialized",
		[-HB_MC_INVALID]           = "Invalid input",
		[-HB_MC_NOMEM]             = "Out of memory",
		[-HB_MC_NOIMPL]            = "Not implemented",
                [-HB_MC_NOTFOUND]          = "Not found",
		[-HB_MC_BUSY]              = "Busy",
        };
        return strtab[-err];
}

#ifdef __cplusplus
}
#endif

#endif
