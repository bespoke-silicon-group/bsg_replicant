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
    sparse_matrix_t *__restrict__ A_ptr // csr
    ,sparse_matrix_t *__restrict__ B_ptr // csr
    ,sparse_matrix_t *__restrict__ C_ptr // csr
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
    int row_start = 0;
    int row_stop = A_lcl.n_major;
#endif
        
    // sync
    barrier::spmm_barrier();
    
    bsg_cuda_print_stat_start(TAG_ROW_SOLVE);

    // foreach row
    for (int Ai = row_start + __bsg_id; Ai < row_stop; Ai += THREADS) {
        bsg_print_int(Ai);
        spmm_solve_row(Ai);
    }

    bsg_cuda_print_stat_end(TAG_ROW_SOLVE);

    // sync
    barrier::spmm_barrier();

    bsg_cuda_print_stat_start(TAG_ROW_SORT);
    
    // foreach row
    for (int Ai = row_start + __bsg_id; Ai < row_stop; Ai += THREADS)
        spmm_sort_row(Ai);
    
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

    for (int Ci = row_start + __bsg_id; Ci < row_stop; Ci += THREADS)
        spmm_copy_results(Ci);

    bsg_cuda_print_stat_end(TAG_RESULTS_COPY);
    // sync
    barrier::spmm_barrier();
    
    return 0;
}
#endif
