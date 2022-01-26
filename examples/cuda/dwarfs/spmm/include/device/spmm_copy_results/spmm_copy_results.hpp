#pragma once
#include "spmm_copy_results_common.hpp"

template <int UNROLL>
static  void spmm_update_nonzeros(
    int Ci
    , int nnz
    , bsg_attr_remote int       *__restrict__ idx_ptr
    , bsg_attr_remote float     *__restrict__ val_ptr
    , bsg_attr_remote spmm_partial_t *__restrict__ par_ptr
    )
{
    pr_dbg("copying %3d results for row %3d to 0x%08x and 0x%08x\n"
           , nnz
           , Ci
           , reinterpret_cast<unsigned>(idx_ptr)
           , reinterpret_cast<unsigned>(val_ptr));
           
    int nonzero = 0;
    for (; nonzero + UNROLL < nnz;) {
        int   idx_tmp[UNROLL];
        float val_tmp[UNROLL];
        for (int i = 0; i < UNROLL; i++) {
            idx_tmp[i] = par_ptr[nonzero+i].idx;
            val_tmp[i] = par_ptr[nonzero+i].val;
        }
        for (int i = 0; i < UNROLL; i++) {
            idx_ptr[nonzero+i] = idx_tmp[i];
            val_ptr[nonzero+i] = val_tmp[i];
        }
        nonzero += UNROLL;
    }
   
    for (; nonzero < nnz; nonzero++) {
        int idx_tmp   = par_ptr[nonzero].idx;
        float val_tmp = par_ptr[nonzero].val;
        idx_ptr[nonzero] = idx_tmp;
        val_ptr[nonzero] = val_tmp;
    }

}


static inline void spmm_copy_results(int Ci, int Ci_off, int Ci_nnz)
{
    //pr_dbg("copying results for row %3d\n", Ci);
    //int nnz = C_glbl_p->mjr_nnz_ptr[Ci];
    int off = Ci_off;
    int nnz = Ci_nnz;
    kernel_remote_int_ptr_t idx_ptr = &C_glbl_p->mnr_idx_remote_ptr[off];
    kernel_remote_float_ptr_t val_ptr = &C_glbl_p->val_remote_ptr[off];

    spmm_partial_t *parts = reinterpret_cast<spmm_partial_t*>(C_glbl_p->alg_priv_remote_ptr[Ci]);
    spmm_update_nonzeros<8>(Ci, nnz, idx_ptr, val_ptr, to_remote_ptr(parts));
}

