#include "bsg_manycore.h"
#include "bsg_tile_config_vars.h"
#include "sparse_matrix.h"
#include "bsg_manycore_atomic.h"
#include <cstdint>
#include <atomic>
#include <algorithm>
#include <cstring>
#include <functional>
#include "spmm.hpp"
#include "spmm_solve_row.hpp"
#include "spmm_sort_row.hpp"
#include "spmm_compute_offsets.hpp"
#include "spmm_copy_results.hpp"
#include "spmm_barrier.hpp"

#ifdef __KERNEL_SPMM__
extern "C" int kernel_spmm(
    sparse_matrix_partition_t *__restrict__ A_ptr // csr
    ,sparse_matrix_partition_t *__restrict__ B_ptr // csr
    ,sparse_matrix_partition_t *__restrict__ C_ptr // csr
    ,std::atomic<intptr_t> *mem_pool_arg // mem pool
#ifdef __ABREV__
    , int row_start
    , int row_stop
#endif
    )
{
    spmm_init(A_ptr, B_ptr, C_ptr, mem_pool_arg);
    spmm_solve_row_init();

#if !defined(__ABREV__)
#if !defined(__PART__)
    int row_start = 0;
    int row_stop = A_lcl.n_major;
#else
    int row_start = C_part_lcl.partinfo.major_start;
    int row_stop  = C_part_lcl.partinfo.major_stop;
#endif
#endif
    int row_range = row_stop-row_start;
    // sync
    barrier::spmm_barrier();
    
    bsg_cuda_print_stat_start(TAG_ROW_SOLVE);

    // foreach row
    for (int Ci_base =  __bsg_id * VCACHE_STRIPE_WORDS;
         Ci_base < row_range;
         Ci_base += THREADS * VCACHE_STRIPE_WORDS) {
        int Ci_start = row_start + Ci_base;
        int Ci_stop  = std::min(row_stop, Ci_start + VCACHE_STRIPE_WORDS);
        for (int Ci = Ci_start; Ci < Ci_stop; Ci++) {
            spmm_solve_row(Ci);
        }
    }

    bsg_cuda_print_stat_end(TAG_ROW_SOLVE);

    // sync
    barrier::spmm_barrier();

    bsg_cuda_print_stat_start(TAG_ROW_SORT);
    
    // foreach row
#ifndef SPMM_SKIP_SORTING
    // foreach row
    for (int Ci_base =  __bsg_id * VCACHE_STRIPE_WORDS;
         Ci_base < row_range;
         Ci_base += THREADS * VCACHE_STRIPE_WORDS) {
        int Ci_start = row_start + Ci_base;
        int Ci_stop  = std::min(row_stop, Ci_start + VCACHE_STRIPE_WORDS);
        for (int Ci = Ci_start; Ci < Ci_stop; Ci++) {
            spmm_sort_row(Ci);
        }
    }
#endif
    // sync
    bsg_cuda_print_stat_end(TAG_ROW_SORT);

    barrier::spmm_barrier();
    
    bsg_cuda_print_stat_start(TAG_OFFSET_COMPUTE);

    C_lcl = *C_glbl_p;
    spmm_compute_offsets();

    // sync
    if (__bsg_id == 0) {
        pr_dbg("%d nonzeros found\n", C_glbl_p->n_non_zeros);
        C_glbl_p->mnr_idx_ptr = (kernel_int_ptr_t)(spmm_malloc(sizeof(int) * C_glbl_p->n_non_zeros));
        C_glbl_p->val_ptr = (kernel_float_ptr_t)(spmm_malloc(sizeof(float) * C_glbl_p->n_non_zeros));
    }

    bsg_cuda_print_stat_end(TAG_OFFSET_COMPUTE);
    // sync
    barrier::spmm_barrier();
    
    bsg_cuda_print_stat_start(TAG_RESULTS_COPY);

    // foreach row
    for (int Ci_base =  __bsg_id * VCACHE_STRIPE_WORDS;
         Ci_base < row_range;
         Ci_base += THREADS * VCACHE_STRIPE_WORDS) {
        int Ci_start = row_start + Ci_base;
        int Ci_stop  = std::min(row_stop, Ci_start + VCACHE_STRIPE_WORDS);
        for (int Ci = Ci_start; Ci < Ci_stop; Ci++) {
            spmm_copy_results(Ci);
        }
    }

    bsg_cuda_print_stat_end(TAG_RESULTS_COPY);
    // sync
    barrier::spmm_barrier();
    
    return 0;
}
#endif
