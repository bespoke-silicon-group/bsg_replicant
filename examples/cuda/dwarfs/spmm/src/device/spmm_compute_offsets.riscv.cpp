#include "sparse_matrix.h"
#include "spmm_compute_offsets.hpp"

// // compute offsets for each row
// // Max: Figure out a way to coalesce this
// for (int Ci = __bsg_id; Ci < C.n_major; Ci += THREADS) {
//     int nnz = C_ptr->mjr_nnz_ptr[Ci];
//     // add to each row after
//     for (int Cip = Ci+1; Cip < C.n_major; Cip++) {
//         asm volatile ("amoadd.w zero, %[nnz], %[mem]" :: [mem] "m" (C.mnr_off_ptr_vanilla[Cip]), [nnz] "r"(nnz));
//     }
// }

void spmm_compute_offsets(int Ci)
{
    int nnz = C_glbl_p->mjr_nnz_remote_ptr[Ci];
    pr_dbg("Computing offsets from %d\n", Ci);
    bsg_print_int(Ci);    
    // Max: this can and should be unrolled
    for (int Cip = Ci+1; Cip < C_lcl.n_major; Cip++) {
        std::atomic<int>* nnzp = reinterpret_cast<std::atomic<int>*>(&C_lcl.mnr_off_ptr[Cip]);
        nnzp->fetch_add(nnz, std::memory_order_relaxed);
    }
}
