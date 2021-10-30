#pragma once
#include "sparse_matrix.h"
#ifdef __cplusplus
extern "C" {
#endif
    typedef struct sparse_matrix_partition_info {
        int major_start;
        int major_stop;
        int minor_id;
    } sparse_matrix_partition_info_t;

    typedef struct sparse_matrix_partition {
        sparse_matrix_t              matrix;
        sparse_matrix_partition_info partinfo;
    } sparse_matrix_partition_t;
#ifdef __cplusplus
}
#endif
