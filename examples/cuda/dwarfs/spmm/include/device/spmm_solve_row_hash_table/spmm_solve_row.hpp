#pragma once
#include "spmm_solve_row.hpp"
#include "spmm.hpp"
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
static void spmm_scalar_row_product(
    float Aij
    ,int Bi
    )
{
    int off = B_lcl.mnr_off_remote_ptr[Bi];
    //int nnz = B_lcl.mjr_nnz_ptr[Bi];
    int nnz = B_lcl.mnr_off_remote_ptr[Bi+1] - off;

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

static void spmm_solve_row_init()
{
    hash_table::init();
}

static void spmm_solve_row_exit()
{
}

static inline void spmm_solve_row(
    int Ai
    ,int Ai_off
    ,int Ai_nnz
    )
{
    //bsg_print_int(Ai);
    // pr_dbg("solving for row %3d\n", Ai);
    // set the number of partials to zero
    hash_table::tbl_num_entries = 0;

    // fetch row meta data
    int off = Ai_off;
    int nnz = Ai_nnz;

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

    if (hash_table::tbl_num_entries > 0) {
        // insert partials into C
        C_lcl.mjr_nnz_remote_ptr[Ai] = hash_table::tbl_num_entries;

        // allocate temporary storage for non zeros
        std::size_t size = sizeof(spmm_partial_t)*hash_table::tbl_num_entries;

        // pad up to a cache line
        std::size_t rem = size & (VCACHE_STRIPE_WORDS*sizeof(int)-1);
        if (rem != 0)
            size += (VCACHE_STRIPE_WORDS*sizeof(int) - rem);

        spmm_partial_t *parts_glbl = (spmm_partial_t*)spmm_malloc(size);

        //bsg_print_int(tbl_num_entries);
        pr_dbg("solved row %3d, saving %3d nonzeros to address 0x%08x\n"
               , Ai
               , hash_table::tbl_num_entries
               , parts_glbl);

        // for each entry in the table
        int j = 0; // tracks nonzero number
        for (hash_table::spmm_elt_t *e = hash_table::tbl_head; e != nullptr; ) {
            // save the next poix1nter
            spmm_partial_t part;
            hash_table::spmm_elt_t *next;
            // we are hand scheduling this...
            part.idx = e->part.idx;
            bsg_compiler_memory_barrier();
            part.val = e->part.val;
            bsg_compiler_memory_barrier();
            next = e->tbl_next;
            bsg_compiler_memory_barrier();
            // clear table entry
            hash_table::nonzeros_table[hash_table::hash(part.idx)] = nullptr;
            // copy to partitions
            pr_dbg("  copying from 0x%08x to 0x%08x\n"
                   , reinterpret_cast<unsigned>(e)
                   , reinterpret_cast<unsigned>(&parts_glbl[j]));
            parts_glbl[j++] = part;
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
}
