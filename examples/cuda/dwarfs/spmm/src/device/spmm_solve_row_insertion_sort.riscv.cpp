#include "bsg_manycore.h"
#include "bsg_tile_config_vars.h"
#include "sparse_matrix.h"
#include "spmm_solve_row.hpp"
#include <algorithm>
#include <cstring>

static thread spmm_partial_t parts [SPMM_SOLVE_ROW_LOCAL_DATA_WORDS/sizeof(spmm_partial_t)];
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

#ifdef __KERNEL_SCALAR_ROW_PRODUCT__
extern "C" kernel_spmm_scalar_row_product(sparse_matrix_t *__restrict A_ptr, // csr
                                          sparse_matrix_t *__restrict B_ptr, // csr
                                          sparse_matrix_t *__restrict C_ptr, // csr
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
    pr_dbg(__FILE__ ": spmm_solve_row_init\n");
    return;
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

#ifdef __KERNEL_SOLVE_ROW__
extern "C" int kernel_solve_row(sparse_matrix_t *__restrict A_ptr, // csr
                                sparse_matrix_t *__restrict B_ptr, // csr
                                sparse_matrix_t *__restrict C_ptr, // csr
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
