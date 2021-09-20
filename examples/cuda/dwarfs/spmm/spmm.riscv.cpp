#include "bsg_manycore.h"
#include "bsg_tile_config_vars.h"
#include "sparse_matrix.h"
#include "bsg_mcs_mutex.h"
#include "bsg_manycore_atomic.h"
#include <cstdint>
#include <atomic>
#include <algorithm>
#include <cstring>
#include <functional>
#include "spmm.hpp"
#include "spmm_solve_row.hpp"
#include "spmm_compute_offsets.hpp"
#include "spmm_copy_results.hpp"

#define BSG_TILE_GROUP_X_DIM bsg_tiles_X
#define BSG_TILE_GROUP_Y_DIM bsg_tiles_Y
#include "bsg_tile_group_barrier.h"

INIT_TILE_GROUP_BARRIER(rbar, cbar, 0, bsg_tiles_X-1, 0, bsg_tiles_Y-1);

void spmm_barrier()
{    
    bsg_tile_group_barrier(&rbar, &cbar);
}

#define THREADS                                 \
    (bsg_tiles_X*bsg_tiles_Y)

thread std::atomic<intptr_t> *spmm_mem_pool = nullptr;
thread sparse_matrix_t A_lcl;
thread sparse_matrix_t B_lcl;
thread sparse_matrix_t C_lcl;

thread sparse_matrix_t *A_glbl_p;
thread sparse_matrix_t *B_glbl_p;
thread sparse_matrix_t *C_glbl_p;

void spmm_init(sparse_matrix_t *__restrict A_ptr, // csr
               sparse_matrix_t *__restrict B_ptr, // csr
               sparse_matrix_t *__restrict C_ptr, // csr
               std::atomic<intptr_t> *mem_pool_arg) // mem pool
{
    A_lcl = *A_ptr;
    B_lcl = *B_ptr;
    C_lcl = *C_ptr;

    A_glbl_p = A_ptr;
    B_glbl_p = B_ptr;
    C_glbl_p = C_ptr;
    
    spmm_mem_pool = mem_pool_arg;

    
}

#ifdef __KERNEL_SPMM__
extern "C" int kernel_spmm(sparse_matrix_t *__restrict A_ptr, // csr
                           sparse_matrix_t *__restrict B_ptr, // csr
                           sparse_matrix_t *__restrict C_ptr, // csr
                           std::atomic<intptr_t> *mem_pool_arg) // mem pool
{
    spmm_init(A_ptr, B_ptr, C_ptr, mem_pool_arg);
    spmm_solve_row_init();

    spmm_barrier();
    bsg_cuda_print_stat_start(TAG_ROW_SOLVE);    

    // foreach row
    for (int Ai = __bsg_id; Ai < A_lcl.n_major; Ai += THREADS)
        spmm_solve_row(Ai);    
    
    // sync
    bsg_cuda_print_stat_end(TAG_ROW_SOLVE);
    spmm_barrier();
    bsg_cuda_print_stat_start(TAG_OFFSET_COMPUTE);

    C_lcl = *C_glbl_p;
    for (int Ci = __bsg_id; Ci < C_lcl.n_major; Ci += THREADS)
        spmm_compute_offsets(Ci);
    
    // sync
    if (__bsg_id == 0) {
        pr_dbg("%d nonzeros found\n", C_glbl_p->n_non_zeros);        
        C_glbl_p->mnr_idx_ptr = (kernel_int_ptr_t)(spmm_malloc(sizeof(int) * C_glbl_p->n_non_zeros));
        C_glbl_p->val_ptr = (kernel_float_ptr_t)(spmm_malloc(sizeof(float) * C_glbl_p->n_non_zeros));
    }

    bsg_cuda_print_stat_end(TAG_OFFSET_COMPUTE);
    spmm_barrier();
    bsg_cuda_print_stat_start(TAG_RESULTS_COPY);

    for (int Ci = __bsg_id; Ci < C_lcl.n_major; Ci += THREADS)
        spmm_copy_results(Ci);

    bsg_cuda_print_stat_end(TAG_RESULTS_COPY);    
    spmm_barrier();

    return 0;
}
#endif
