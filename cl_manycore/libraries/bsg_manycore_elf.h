#ifndef BSG_MANYCORE_ELF_H
#define BSG_MANYCORE_ELF 
#include <bsg_manycore_features.h>
#include <bsg_manycore_mem.h>

#ifdef __cplusplus 
extern "C" {
#endif

__attribute__((deprecated))
int symbol_to_eva(const char *fname, const char *sym_name, eva_t *eva);

#ifdef __cplusplus
}
#endif

#endif
