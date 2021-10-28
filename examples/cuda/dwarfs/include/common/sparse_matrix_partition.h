#pragma once
#include "sparse_matrix.h"
#ifdef __cplusplus
extern "C" {
#endif
    typedef struct sparse_matrix_partition_info {
        int major_start;
        int partid_hash; // to find the partition of row i, lookup partid_hash[i/partitions];
    } sparse_matrix_partition_info_t;

    typedef struct sparse_matrix_partition {
        sparse_matrix_t              matrix;
        int                          partitions;
        int                          partid;
        sparse_matrix_partition_info partinfo[1];
    } sparse_matrix_partition_t;
#ifdef __cplusplus
}
#endif
