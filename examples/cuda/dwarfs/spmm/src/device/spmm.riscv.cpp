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
#include "spmm_compute_offsets.hpp"
#include "spmm_copy_results.hpp"
//#include "bsg_tile_group_barrier.h"

extern "C" void bsg_barrier_amoadd(int *lock, int *sense);    

//INIT_TILE_GROUP_BARRIER(rbar, cbar, 0, bsg_tiles_X-1, 0, bsg_tiles_Y-1);

__attribute__((section(".dram")))
bsg_mcs_mutex_t mtx;

__attribute__((section(".dram")))
static int lock  = 0;
static int sense = 1;

void spmm_barrier()
{    
    //bsg_tile_group_barrier(&rbar, &cbar);
    bsg_barrier_amoadd(&lock, &sense);
}

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

#ifdef CHECK_BARRIER
    bsg_print_int(0);
#endif
    spmm_barrier();
#ifdef CHECK_BARRIER
    bsg_print_int(1);
#endif
    bsg_cuda_print_stat_start(TAG_ROW_SOLVE);

    // foreach row
    for (int Ai = __bsg_id; Ai < A_lcl.n_major; Ai += THREADS)
        spmm_solve_row(Ai);

    // sync
    bsg_cuda_print_stat_end(TAG_ROW_SOLVE);
#ifdef CHECK_BARRIER
    bsg_print_int(2);
#endif
    spmm_barrier();
#ifdef CHECK_BARRIER
    bsg_print_int(3);
#endif
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
#ifdef CHECK_BARRIER
    bsg_print_int(4);
#endif
    spmm_barrier();
#ifdef CHECK_BARRIER
    bsg_print_int(5);
#endif
    bsg_cuda_print_stat_start(TAG_RESULTS_COPY);

    for (int Ci = __bsg_id; Ci < C_lcl.n_major; Ci += THREADS)
        spmm_copy_results(Ci);

    bsg_cuda_print_stat_end(TAG_RESULTS_COPY);
#ifdef CHECK_BARRIER
    bsg_print_int(6);
#endif
    spmm_barrier();
#ifdef CHECK_BARRIER
    bsg_print_int(7);
#endif
    return 0;
}
#endif
