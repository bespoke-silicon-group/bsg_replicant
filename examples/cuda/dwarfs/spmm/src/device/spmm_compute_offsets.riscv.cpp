#include "sparse_matrix.h"
#include "spmm.hpp"
#include "spmm_compute_offsets.hpp"
#include "bsg_tile_config_vars.h"

void spmm_compute_offsets()
{
    for (int Ci = __bsg_id; Ci < C_lcl.n_major; Ci += THREADS) {
        int nnz = C_glbl_p->mjr_nnz_remote_ptr[Ci];
        pr_dbg("Computing offsets from %d\n", Ci);
        // Max: this can and should be unrolled
        for (int Cip = Ci+1; Cip < C_lcl.n_major; Cip++) {
            std::atomic<int>* nnzp = reinterpret_cast<std::atomic<int>*>(&C_lcl.mnr_off_ptr[Cip]);
            nnzp->fetch_add(nnz, std::memory_order_relaxed);
        }
    }
}

#if defined(__KERNEL_COMPUTE_OFFSETS__)
extern "C" int kernel_spmm_compute_offsets(sparse_matrix_t *__restrict A_ptr, // csr
                                           sparse_matrix_t *__restrict B_ptr, // csr
                                           sparse_matrix_t *__restrict C_ptr, // csr
                                           std::atomic<intptr_t> *mem_pool_arg)
{
    spmm_init();
    spmm_compute_offsets();
}
#endif
