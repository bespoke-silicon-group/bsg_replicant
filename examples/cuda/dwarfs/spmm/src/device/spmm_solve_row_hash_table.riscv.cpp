#include "bsg_manycore.h"
#include "bsg_tile_config_vars.h"
#include "sparse_matrix.h"
#include "spmm_solve_row.hpp"
#include "spmm_hash_table.hpp"

#include <algorithm>
#include <cstring>


/**
 * Solve A[i;j] * B[j;]
 */
static void spmm_scalar_row_product(float Aij, int Bi)
{
    int off = B_lcl.mnr_off_ptr[Bi];
    int nnz = B_lcl.mjr_nnz_ptr[Bi];

    // stall on off
    kernel_remote_int_ptr_t cols = &B_lcl.mnr_idx_remote_ptr[off];
    kernel_remote_float_ptr_t vals = &B_lcl.val_remote_ptr[off];

    int nz = 0;
#if defined(SPMM_PREFETCH)
#define PREFETCH 4
    for (; nz + PREFETCH < nnz; nz += PREFETCH) {
        float Bij [PREFETCH];
        int   Bj  [PREFETCH];
        // prefetch data
        bsg_unroll(8)
        for (int pre = 0; pre < PREFETCH; pre++) {
            Bij[pre] = vals[nz+pre];
            Bj [pre] = cols[nz+pre];
        }
        float Cij  [PREFETCH];
        int   hash [PREFETCH];
        // ilp fmul and hash function comp.
        bsg_unroll(8)
        for (int pre = 0; pre < PREFETCH; pre++) {
#if defined(SPMM_NO_FLOPS)
            Cij[pre] = Bij[pre];
#else
            Cij[pre]   = Aij * Bij[pre];
#endif
            hash[pre]  = hash_table::hash(Bj[pre]);
        }
        // update hash table
        // serialized
        // possible optimization is to unroll this loop
        // by hand to extract more mlp
        //
        // this is not as straightforward as we'd expect
        // given that collisions may occur and linked
        // list traversal may occur
        for (int pre = 0; pre < PREFETCH; pre++) {
            hash_table::update(Cij[pre], Bj[pre], hash[pre]);
        }
    }
#endif

    for (; nz < nnz; nz++) {
        float Bij = vals[nz];
        int Bj    = cols[nz];
#if defined(SPMM_NO_FLOPS)
        float Cij = Aij;
#else
        float Cij = Aij * Bij;
#endif
        // compute hash
        int idx = hash_table::hash(Bj);

        // perform symbol table lookup
        hash_table::update(Cij, Bj, idx);
    }
}

#ifdef __KERNEL_SCALAR_ROW_PRODUCT__
extern "C" kernel_spmm_scalar_row_product(sparse_matrix_t *__restrict__ A_ptr, // csr
                                          sparse_matrix_t *__restrict__ B_ptr, // csr
                                          sparse_matrix_t *__restrict__ C_ptr, // csr
                                          std::atomic<intptr_t> *mem_pool_arg, // mem pool
                                          float Aij,
                                          int Bi)
{
    spmm_init(A_ptr, B_ptr, C_ptr, mem_pool_arg);
    bsg_cuda_print_stat_start(TAG_ROW_SOLVE);
    spmm_scalar_row_product(Aij, Bi);
    bsg_cuda_print_stat_end(TAG_ROW_SOLVE);
}
#endif

void spmm_solve_row_init()
{
    hash_table::init();
}

void spmm_solve_row(int Ai)
{
    //bsg_print_int(Ai);
    pr_dbg("solving for row %3d\n", Ai);
    // set the number of partials to zero
    hash_table::tbl_num_entries = 0;

    // fetch row meta data
    int off = A_lcl.mnr_off_remote_ptr[Ai];
    int nnz = A_lcl.mjr_nnz_remote_ptr[Ai];

    // this will stall on 'off'
    kernel_remote_int_ptr_t cols = &A_lcl.mnr_idx_remote_ptr[off];
    kernel_remote_float_ptr_t vals = &A_lcl.val_remote_ptr[off];

    // for each nonzero entry in row A[i:]
    for (int nonzero = 0; nonzero < nnz; nonzero++) {
        int Bi = cols[nonzero];
        float Aij = vals[nonzero];
        pr_dbg("+ A[%d,%d] * B[%d;]\n", Ai, Bi, Bi);
        spmm_scalar_row_product(Aij, Bi);
    }

    // insert partials into C
    C_lcl.mjr_nnz_remote_ptr[Ai] = hash_table::tbl_num_entries;
    spmm_partial_t *parts_glbl = (spmm_partial_t*)spmm_malloc(sizeof(spmm_partial_t)*hash_table::tbl_num_entries);

    //bsg_print_int(tbl_num_entries);
    pr_dbg("solved row %3d, saving %3d nonzeros to address 0x%08x\n"
           , Ai
           , hash_table::tbl_num_entries
           , parts_glbl);

    // for each entry in the table
    int j = 0; // tracks nonzero number
    for (hash_table::spmm_elt_t *e = hash_table::tbl_head; e != nullptr; ) {
        // save the next poix1nter
        hash_table::spmm_elt_t *next = e->tbl_next;
        // clear table entry
        hash_table::nonzeros_table[hash_table::hash(e->part.idx)] = nullptr;
        // copy to partitions
        pr_dbg("  copying from 0x%08x to 0x%08x\n"
                      , reinterpret_cast<unsigned>(e)
                      , reinterpret_cast<unsigned>(&parts_glbl[j]));
        parts_glbl[j++] = e->part;
        // free entry
        hash_table::free_elt(e);
        // continue
        e = next;
    }

    hash_table::tbl_head = nullptr;

    // store as array of partials in the alg_priv_ptr field
    C_lcl.alg_priv_remote_ptr[Ai] = reinterpret_cast<intptr_t>(parts_glbl);

    // update the global number of nonzeros
    std::atomic<int>* nnzp = reinterpret_cast<std::atomic<int> *>((&C_glbl_p->n_non_zeros));
    nnzp->fetch_add(hash_table::tbl_num_entries);
}

#ifdef __KERNEL_SOLVE_ROW__
extern "C" int kernel_solve_row(sparse_matrix_t *__restrict__ A_ptr, // csr
                                sparse_matrix_t *__restrict__ B_ptr, // csr
                                sparse_matrix_t *__restrict__ C_ptr, // csr
                                std::atomic<intptr_t> *mem_pool_arg, // mem pool
                                int Ai)
{
    spmm_init(A_ptr, B_ptr, C_ptr, mem_pool_arg);
    spmm_solve_row_init();
    bsg_cuda_print_stat_start(TAG_ROW_SOLVE);
    spmm_solve_row(Ai);
    bsg_cuda_print_stat_end(TAG_ROW_SOLVE);
    return 0;
}
#endif
