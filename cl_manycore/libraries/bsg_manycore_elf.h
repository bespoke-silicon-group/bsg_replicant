#ifndef BSG_MANYCORE_ELF_H
#define BSG_MANYCORE_ELF 

#ifdef __cplusplus 
extern "C" {
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <stdint.h>
#include <alloca.h>
#include <inttypes.h>
#include <elf.h>
void object_symbol_table_init(const char *fname);

#ifdef __cplusplus
}
#endif

#endif
