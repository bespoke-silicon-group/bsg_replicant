#pragma once
#include "bsg_manycore.h"
#include "bsg_tile_config_vars.h"
#include "sparse_matrix.h"
#include "spmm_solve_row_common.hpp"
#include <algorithm>
#include <cstring>

static float mul(volatile float a, volatile float b)
{
    return a*b;
}

static void spmm_scalar_row_product(float Aij, int Bi)
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
        float Cij;
        asm volatile ("fmul.s %[Cij], %[Aij], %[Bij]"
                      : [Cij] "=f" (Cij)
                      : [Aij] "f" (Aij), [Bij] "f" (Bij));
        //float Cij = mul(Aij, Bij);
    }
}

static void spmm_solve_row_init()
{
    pr_dbg(__FILE__ ": Calling spmm_solve_row_init\n");
}

static void spmm_solve_row(
    int Ai
    ,int Ai_off
    ,int Ai_nnz
    )
{
    //bsg_print_int(Ai);
    pr_dbg("Solving for row %d\n", Ai);
  
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
        spmm_scalar_row_product(Aij, Bi);
    }
}

static void spmm_solve_row_exit()
{
}
