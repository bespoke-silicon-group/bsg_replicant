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

#ifndef SPMM_WORK_GRANULARITY
#error "define SPMM_WORK_GRANULARITY"
#endif

#ifdef SPMM_PREFETCH
#ifndef PREFETCH
#define PREFETCH 4
#endif
#endif

__attribute__((section(".dram")))
std::atomic<int> rowq_solve;
__attribute__((section(".dram")))
std::atomic<int> rowq_sort;
__attribute__((section(".dram")))
std::atomic<int> rowq_cpy;

#ifdef __KERNEL_SPMM__
extern "C" int kernel_spmm(
    sparse_matrix_partition_t *__restrict__ A_ptr // csr
    ,sparse_matrix_partition_t *__restrict__ B_ptr // csr
    ,sparse_matrix_partition_t *__restrict__ C_ptr // csr
    ,std::atomic<intptr_t> *mem_pool_arg // mem pool
#ifdef __ABREV__
    ,int row_start
    ,int row_stop
#endif
    )
{        
    spmm_init(A_ptr, B_ptr, C_ptr, mem_pool_arg);
#if defined(__PART__)
    int row_start = C_part_lcl.partinfo.major_start;
    int row_stop  = C_part_lcl.partinfo.major_stop;
#elif !defined(__ABREV__)
    int row_start = 0;
    int row_stop  = A_lcl.n_major;
#endif

    if (__bsg_id == 0) {
        rowq_solve.store(row_start, std::memory_order_release);
        rowq_sort.store(row_start, std::memory_order_release);
        rowq_cpy.store(row_start, std::memory_order_release);
    }


    barrier::spmm_barrier();
    spmm_solve_row_init();
    bsg_cuda_print_stat_start(TAG_ROW_SOLVE);
//#if 0
    // foreach row
    for (int Ci_base = rowq_solve.fetch_add(SPMM_WORK_GRANULARITY, std::memory_order_relaxed);
         Ci_base < row_stop;
         Ci_base = rowq_solve.fetch_add(SPMM_WORK_GRANULARITY, std::memory_order_relaxed)) {
        int Ci_stop = std::min(Ci_base+SPMM_WORK_GRANULARITY, row_stop);
        int Ci = Ci_base;
#ifdef SPMM_PREFETCH
        for (; Ci + PREFETCH < Ci_stop; Ci += PREFETCH) {
            int Ci_nnz[PREFETCH];
            int Ci_off[PREFETCH];
            bsg_unroll(8)
            for (int pre = 0; pre < PREFETCH; pre++) {
                Ci_off[pre] = A_lcl.mnr_off_remote_ptr[Ci + pre];
                Ci_nnz[pre] = A_lcl.mnr_off_remote_ptr[Ci + pre + 1] - Ci_off[pre];
            }
            bsg_compiler_memory_barrier(); 
            for (int pre = 0; pre < PREFETCH; pre++) {
#ifdef HEART_BEAT
                bsg_print_int(Ci+pre);
#endif
                spmm_solve_row(Ci+pre, Ci_off[pre], Ci_nnz[pre]);
            }
        }
#endif
        for (;Ci < Ci_stop; Ci++) {
            int Ci_off = A_lcl.mnr_off_remote_ptr[Ci];
            int Ci_nnz = A_lcl.mnr_off_remote_ptr[Ci+1] - Ci_off;
#ifdef HEART_BEAT
            bsg_print_int(Ci);
#endif
            spmm_solve_row(Ci, Ci_off, Ci_nnz);
        }
    }
//#endif
    // sync
    bsg_cuda_print_stat_end(TAG_ROW_SOLVE);
    spmm_print_int(__bsg_id);
    spmm_solve_row_exit();
    barrier::spmm_barrier();

    bsg_cuda_print_stat_start(TAG_ROW_SORT);
#ifndef SPMM_SKIP_SORTING
    // foreach row
    for (int Ci_base = rowq_sort.fetch_add(SPMM_WORK_GRANULARITY, std::memory_order_relaxed);
         Ci_base < row_stop;
         Ci_base = rowq_sort.fetch_add(SPMM_WORK_GRANULARITY, std::memory_order_relaxed)) {
        int Ci_stop = std::min(Ci_base+SPMM_WORK_GRANULARITY, row_stop);
        for (int Ci = Ci_base; Ci < Ci_stop; Ci++) {
            spmm_sort_row(Ci);
        }
    }
#endif
    bsg_cuda_print_stat_end(TAG_ROW_SORT);

    spmm_print_int(__bsg_id);
    barrier::spmm_barrier(); // sync
    
    bsg_cuda_print_stat_start(TAG_OFFSET_COMPUTE);

    C_lcl = *C_glbl_p;
    spmm_compute_offsets();

    spmm_print_int(__bsg_id);
    barrier::spmm_barrier(); // sync

    if (__bsg_id == 0) {
        pr_dbg("%d nonzeros found\n", C_glbl_p->n_non_zeros);
        C_glbl_p->mnr_idx_ptr = (kernel_int_ptr_t)(spmm_malloc(sizeof(int) * C_glbl_p->n_non_zeros));
        C_glbl_p->val_ptr = (kernel_float_ptr_t)(spmm_malloc(sizeof(float) * C_glbl_p->n_non_zeros));
    }
    bsg_cuda_print_stat_end(TAG_OFFSET_COMPUTE);

    spmm_print_int(__bsg_id);
    barrier::spmm_barrier();
    C_lcl = *C_glbl_p;

    bsg_cuda_print_stat_start(TAG_RESULTS_COPY);
    // foreach row
    for (int Ci_base = rowq_cpy.fetch_add(SPMM_WORK_GRANULARITY, std::memory_order_relaxed);
         Ci_base < row_stop;
         Ci_base = rowq_cpy.fetch_add(SPMM_WORK_GRANULARITY, std::memory_order_relaxed)) {
        int Ci = Ci_base;
        int Ci_stop = std::min(Ci_base+SPMM_WORK_GRANULARITY, row_stop);
#ifdef SPMM_PREFETCH
        for (; Ci + PREFETCH < Ci_stop; Ci += PREFETCH) {
            int Ci_off[PREFETCH];
            int Ci_nnz[PREFETCH];
            bsg_unroll(8)
            for (int pre = 0; pre < PREFETCH; pre++) {
                Ci_off[pre] = C_lcl.mnr_off_remote_ptr[Ci + pre];
                Ci_nnz[pre] = C_lcl.mnr_off_remote_ptr[Ci + pre + 1] - Ci_off[pre];
            }
            bsg_compiler_memory_barrier();
            for (int pre = 0; pre < PREFETCH; pre++) {
                spmm_copy_results(Ci, Ci_off[pre], Ci_nnz[pre]);
            }
        }
#endif
        for (; Ci < Ci_stop; Ci++) {
            int Ci_off = C_lcl.mnr_off_remote_ptr[Ci];
            int Ci_nnz = C_lcl.mnr_off_remote_ptr[Ci+1] - Ci_off;
            spmm_copy_results(Ci, Ci_off, Ci_nnz);
        }
    }

    bsg_cuda_print_stat_end(TAG_RESULTS_COPY);
    spmm_print_int(__bsg_id);
    barrier::spmm_barrier();

    return 0;
}
#endif
