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
        /*
         * Clang has very strict type safety rules regarding
         * the casting of pointers from different address spaces
         * and performing atomic memory operations on non-default
         * memory locations.
         *
         * We use unions here to hack around that issue.
         */
        union {
            kernel_remote_int_ptr_t mjr_nnz_remote_ptr;
            kernel_int_ptr_t        mjr_nnz_ptr;
        };
        union {
            kernel_remote_int_ptr_t mnr_off_remote_ptr;
            kernel_int_ptr_t        mnr_off_ptr;
        };
        union {
            kernel_remote_int_ptr_t mnr_idx_remote_ptr;
            kernel_int_ptr_t        mnr_idx_ptr;
        };
        union {
            kernel_remote_float_ptr_t val_remote_ptr;
            kernel_float_ptr_t        val_ptr;
        };
        union {
            kernel_remote_int_ptr_t alg_priv_remote_ptr;
            kernel_int_ptr_t        alg_priv_ptr;
        };
    } sparse_matrix_t;

#ifdef __cplusplus
}
#endif
