#ifndef BSG_MANYCORE_ELF_H
#define BSG_MANYCORE_ELF 

#ifdef __cplusplus 
extern "C" {
#endif
#ifndef COSIM
#include <bsg_manycore_features.h>
#include <bsg_manycore_mem.h>
#else
#include "bsg_manycore_features.h"
#include "bsg_manycore_mem.h"
#endif

int symbol_to_eva(const char *fname, const char *sym_name, eva_t *eva);
#ifdef __cplusplus
}
#endif

#endif
