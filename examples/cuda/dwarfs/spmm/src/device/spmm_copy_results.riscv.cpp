#include "spmm_copy_results.hpp"

template <int UNROLL>
__attribute__((noinline))
void spmm_update_nonzeros(int Ci
                          , int nnz
                          , bsg_attr_remote int       *__restrict idx_ptr
                          , bsg_attr_remote float     *__restrict val_ptr
                          , bsg_attr_remote spmm_partial_t *__restrict par_ptr)
{
    pr_dbg("Copying results for row %d to 0x%08x and 0x%08x\n"
           , Ci
           , reinterpret_cast<unsigned>(idx_ptr)
           , reinterpret_cast<unsigned>(val_ptr));
           
    int nonzero = 0;
    while (nonzero + UNROLL <= nnz) {
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


void spmm_copy_results(int Ci)
{
    pr_dbg("copying results for row %d\n", Ci);
    int nnz = C_glbl_p->mjr_nnz_ptr[Ci];
    int off = C_glbl_p->mnr_off_ptr[Ci];
    kernel_remote_int_ptr_t idx_ptr = &C_glbl_p->mnr_idx_remote_ptr[off];
    kernel_remote_float_ptr_t val_ptr = &C_glbl_p->val_remote_ptr[off];

    spmm_partial_t *parts = reinterpret_cast<spmm_partial_t*>(C_glbl_p->alg_priv_remote_ptr[Ci]);
    spmm_update_nonzeros<8>(Ci, nnz, idx_ptr, val_ptr, to_remote_ptr(parts));
}
