#pragma once
#include "types.h"
#ifdef __cplusplus
extern "C" {
#endif

    typedef struct sparse_matrix {
        int is_row_major;
        int n_major;
        int n_minor;
        int n_non_zeros;
        kernel_int_ptr_t mjr_nnz_ptr;
        kernel_int_ptr_t mnr_off_ptr;
        kernel_int_ptr_t mnr_idx_ptr;
        kernel_float_ptr_t   val_ptr;
    } sparse_matrix_t;

#ifdef __cplusplus
}
#endif
