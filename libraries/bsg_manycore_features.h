#ifndef BSG_MANYCORE_FEATURES_H
#define BSG_MANYCORE_FEATURES_H
// <features.h> sorts out many of these defines based on compile time flags (e.g. -std=c++11)
#include <features.h>
// check _BSG_SOURCE
#ifndef _BSD_SOURCE
#error "_BSG_SOURCE not defined: required for bsg_manycore_runtime"
#endif

// check _XOPEN_SOURCE
#ifndef _XOPEN_SOURCE
#error "_XOPEN_SOURCE not defined: required for bsg_manycore_runtime"
#else
#if _XOPEN_SOURCE < 500
#error "_XOPEN_SOURCE < 500: bsg_manycore_runtime requires _XOPEN_SOURCE >= 500"
#endif
#endif

#endif
