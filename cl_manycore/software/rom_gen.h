#ifndef ROM_GEN_H
#define ROM_GEN_H

#include <stdio.h>
#include <stdint.h>

static const int ROM_WIDTH = 32;
static const int ROM_DEPTH = 10;

#define MC_VERSION_ID       0x00020100
#define COMPILATION_DATA    0x03262019
#define NETWORK_ADDR_WIDTH  28
#define NETWORK_DATA_WIDTH  32
#define X_Y_DIMENSION       0x00050004
#define HOST_LOCATION       0x00000003
#define DESCRIPTION_PTR     0x1234
#define BSG_IP_CORES_HASH   0xd9ed2b48
#define BSG_MANYCORE_HASH   0xFFFFFFFF
#define BSG_F1_HASH         0xFFFFFFFF

#define ROM_TABLE \
                X(E_MC_VERSION_ID, "MC_VERSION_ID", MC_VERSION_ID)  \
                X(E_COMPILATION_DATA, "COMPILATION_DATA", COMPILATION_DATA) \
                X(E_NETWORK_ADDR_WIDTH, "NETWORK_ADDR_WIDTH", NETWORK_ADDR_WIDTH) \
                X(E_NETWORK_DATA_WIDTH, "NETWORK_DATA_WIDTH", NETWORK_DATA_WIDTH) \
                X(E_X_Y_DIMENSION, "X_Y_DIMENSION", X_Y_DIMENSION) \
                X(E_HOST_LOCATION, "HOST_LOCATION", HOST_LOCATION)  \
                X(E_DESCRIPTION_PTR, "DESCRIPTION_PTR", DESCRIPTION_PTR) \
                X(E_BSG_IP_CORES_HASH, "BSG_IP_CORES_HASH", BSG_IP_CORES_HASH) \
                X(E_BSG_MANYCORE_HASH, "BSG_MANYCORE_HASH", BSG_MANYCORE_HASH) \
                X(E_BSG_F1_HASH, "BSG_F1_HASH", BSG_F1_HASH)

#define X(idx, key, val) idx,
    enum ROM_ENUM { ROM_TABLE };
#undef X

#define X(idx, key, val) key,
    const char *ROM_KEY[] = { ROM_TABLE };
#undef X

#define X(idx, key, val) val,
    const uint32_t ROM_DATA[] = { ROM_TABLE };
#undef X

#endif
