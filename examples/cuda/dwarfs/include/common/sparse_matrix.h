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
        kernel_remote_int_ptr_t mjr_nnz_ptr;
        kernel_remote_int_ptr_t mnr_off_ptr;
        kernel_remote_int_ptr_t mnr_idx_ptr;
        kernel_remote_float_ptr_t   val_ptr;
        kernel_remote_int_ptr_t alg_priv_data;
    } sparse_matrix_t;

#ifdef __cplusplus
}
#endif
