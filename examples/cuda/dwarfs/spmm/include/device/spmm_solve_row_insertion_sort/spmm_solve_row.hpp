#pragma once
#include "bsg_manycore.h"
#include "bsg_tile_config_vars.h"
#include "sparse_matrix.h"
#include "spmm_solve_row_common.hpp"
#include <algorithm>
#include <cstring>

static thread spmm_partial_t parts [SPMM_SOLVE_ROW_LOCAL_DATA_WORDS/sizeof(spmm_partial_t)];
static thread int            num_parts = 0;

static void insert(int Cj, float Cij)
{
    spmm_partial_t newp;
    newp.idx = Cj;
    newp.val = Cij;
    spmm_partial_t *p = std::lower_bound(
        parts,
        parts+num_parts,
        newp,
        [](const spmm_partial_t &l, const spmm_partial_t &r) {
            return l.idx < r.idx;
        });

    // insert or update?
    if (p == parts+num_parts) {
        *p = newp;
        num_parts++;
    } else if (p->idx == Cj) {
        p->val += Cij;
    } else {
        std::memmove(p+1, p, sizeof(spmm_partial_t)*(num_parts-(p-&parts[0])));
        *p = newp;
        num_parts++;
    }
}

static void spmm_scalar_row_product(float Aij, int Bi)
{
    int off = B_lcl.mnr_off_remote_ptr[Bi];
    //int nnz = B_lcl.mjr_nnz_remote_ptr[Bi];
    int nnz = B_lcl.mnr_off_remote_ptr[Bi+1] - off;

    // stall on off
    kernel_remote_int_ptr_t cols = &B_lcl.mnr_idx_remote_ptr[off];
    kernel_remote_float_ptr_t vals = &B_lcl.val_remote_ptr[off];

    int nz = 0;
#if defined(SPMM_PREFETCH)
#ifndef PREFETCH
#define PREFETCH 4
#endif
    for (; nz + PREFETCH < nnz; nz += PREFETCH) {
        float Bij [PREFETCH];
        int   Bj  [PREFETCH];
        bsg_unroll(8)
        for (int pre = 0; pre < PREFETCH; pre++) {
            Bij[pre] = vals[nz+pre];
            Bj [pre] = cols[nz+pre];
        }

        float Cij [PREFETCH];
        bsg_unroll(8)
        for (int pre = 0; pre < PREFETCH; pre++) {
            Cij[pre] = Bij[pre] * Aij;
        }

        for (int pre = 0; pre < PREFETCH; pre++) {
            insert(Bj[pre], Cij[pre]);
        }
    }
#endif
    for (; nz < nnz; nz++) {
        float Bij = vals[nz];
        int Bj = cols[nz];

        float Cij = Aij * Bij;

        insert(Bj, Cij);
    }
}

static void spmm_solve_row_init()
{
    // pr_dbg(__FILE__ ": spmm_solve_row_init\n");
    return;
}

static void spmm_solve_row(
    int Ai
    ,int Ai_off
    ,int Ai_nnz
    )
{
    // pr_dbg("Solving for row %d\n", Ai);
    // set the number of partials to zero
    num_parts = 0;

    // fetch row meta data
    int off = Ai_off;
    //int nnz = A_lcl.mjr_nnz_remote_ptr[Ai];
    int nnz = Ai_nnz;

    // this will stall on 'off'
    kernel_remote_int_ptr_t cols = &A_lcl.mnr_idx_remote_ptr[off];
    kernel_remote_float_ptr_t vals = &A_lcl.val_remote_ptr[off];

    // for each nonzero entry in row A[i:]
    for (int nonzero = 0; nonzero < nnz; nonzero++) {
        int Bi = cols[nonzero];
        float Aij = vals[nonzero];
        spmm_scalar_row_product(Aij, Bi);
    }

    // insert partials into C
    C_lcl.mjr_nnz_remote_ptr[Ai] = num_parts;
    if (num_parts > 0) {
        spmm_partial_t *parts_glbl = reinterpret_cast<spmm_partial_t*>(spmm_malloc(sizeof(spmm_partial_t)*num_parts));
        std::memcpy(parts_glbl, parts, sizeof(spmm_partial_t)*num_parts);

        // store as array of partials in the alg_priv_ptr field
        C_lcl.alg_priv_remote_ptr[Ai] = reinterpret_cast<intptr_t>(parts_glbl);
        pr_dbg("solved row %d, saving %d nonzeros to address 0x%08x\n"
               , Ai
               , num_parts
               , parts_glbl);

        // update the global number of nonzeros
        std::atomic<int>* nnzp = reinterpret_cast<std::atomic<int> *>((&C_glbl_p->n_non_zeros));
        nnzp->fetch_add(num_parts);
    }
}
static void spmm_solve_row_exit(void)
{
}
