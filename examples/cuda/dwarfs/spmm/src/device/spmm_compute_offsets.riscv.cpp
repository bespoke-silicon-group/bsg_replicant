#include "sparse_matrix.h"
#include "spmm_compute_offsets.hpp"

void spmm_compute_offsets(int Ci)
{
    int nnz = C_glbl_p->mjr_nnz_remote_ptr[Ci];
    pr_dbg("Computing offsets from %d\n", Ci);
    // Max: this can and should be unrolled
    for (int Cip = Ci+1; Cip < C_lcl.n_major; Cip++) {
        std::atomic<int>* nnzp = reinterpret_cast<std::atomic<int>*>(&C_lcl.mnr_off_ptr[Cip]);
        nnzp->fetch_add(nnz, std::memory_order_relaxed);
    }
}
