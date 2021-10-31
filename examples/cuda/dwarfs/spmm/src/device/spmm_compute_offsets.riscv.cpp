#include "sparse_matrix.h"
#include "spmm.hpp"
#include "spmm_compute_offsets.hpp"
#include "bsg_tile_config_vars.h"

void spmm_compute_offsets()
{
#ifdef __PART__
    int start = C_part_lcl.partinfo.major_start;
    int stop  = C_part_lcl.partinfo.major_stop;
#else
    int start = 0;
    int stop = C_lcl.n_major;
#endif
    for (int Ci = start + __bsg_id; Ci < stop; Ci += THREADS) {
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
extern "C" int kernel_compute_offsets(sparse_matrix_t *__restrict__ A_ptr, // csr
                                      sparse_matrix_t *__restrict__ B_ptr, // csr
                                      sparse_matrix_t *__restrict__ C_ptr, // csr
                                      std::atomic<intptr_t> *mem_pool_arg)
{
    spmm_init(A_ptr, B_ptr, C_ptr, mem_pool_arg);
    spmm_barrier();
    bsg_cuda_print_stat_start(TAG_OFFSET_COMPUTE);
    spmm_compute_offsets();
    bsg_cuda_print_stat_end(TAG_OFFSET_COMPUTE);
    spmm_barrier();
    return 0;
}
#endif
