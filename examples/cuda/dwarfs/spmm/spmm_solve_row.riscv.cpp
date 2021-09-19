#include "sparse_matrix.h"
#include "spmm_solve_row.hpp"
#include <algorithm>
#include <cstring>

#define LOCAL_DATA_WORDS                                                \
    (3*256)

static thread spmm_partial_t parts [LOCAL_DATA_WORDS/sizeof(spmm_partial_t)];
static thread int            num_parts = 0;


__attribute__((noinline))
void spmm_scalar_row_product(float Aij, int Bi)
{
    int off = B_lcl.mnr_off_ptr[Bi];
    int nnz = B_lcl.mjr_nnz_ptr[Bi];

    // stall on off
    kernel_remote_int_ptr_t cols = &B_lcl.mnr_idx_remote_ptr[off];
    kernel_remote_float_ptr_t vals = &B_lcl.val_remote_ptr[off];

    // Max: unroll part of this loop to improve ILP
    for (int nonzero = 0; nonzero < nnz; nonzero++) {
        float Bij = vals[nonzero];
        int Bj = cols[nonzero];

        float Cij = Aij * Bij;

        spmm_partial_t newp;
        newp.idx = Bj;
        newp.val = Cij;
        
        spmm_partial_t *p = std::lower_bound(
            parts,
            parts+num_parts,
            newp,
            [](const spmm_partial_t&l, const spmm_partial_t&r) {return l.idx < r.idx;});

        if (p == parts+num_parts) {
            *p = newp;
            num_parts++;
        } else if (p->idx == Bj) {
            p->val += Cij;
        } else {
            std::memmove(p+1, p, sizeof(spmm_partial_t)*(num_parts-(p-&parts[0])));
            *p = newp;
            num_parts++;
        }
    }
}

void spmm_solve_row(int Ai)
{
    pr_dbg("Solving for row %d\n", Ai);
    // set the number of partials to zero
    num_parts = 0;

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
        spmm_scalar_row_product(Aij, Bi);
    }

    // insert partials into C
    C_lcl.mjr_nnz_remote_ptr[Ai] = num_parts;
    spmm_partial_t *parts_glbl = reinterpret_cast<spmm_partial_t*>(spmm_malloc(sizeof(spmm_partial_t)*num_parts));
    std::memcpy(parts_glbl, parts, sizeof(spmm_partial_t)*num_parts);

    // store as array of partials in the alg_priv_ptr field    
    C_lcl.alg_priv_remote_ptr[Ai] = reinterpret_cast<intptr_t>(parts_glbl);
    pr_dbg("Solved row %d, saving %d nonzeros to address 0x%08x\n"
           , Ai
           , num_parts
           , parts_glbl);

    // update the global number of nonzeros
    std::atomic<int>* nnzp = reinterpret_cast<std::atomic<int> *>((&C_glbl_p->n_non_zeros));
    nnzp->fetch_add(num_parts);
}
