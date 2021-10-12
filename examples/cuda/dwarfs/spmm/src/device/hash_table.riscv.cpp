#include "bsg_manycore.h"
#include "bsg_tile_config_vars.h"
#include "spmm.hpp"
#include "sparse_matrix.h"
#include "spmm_solve_row.hpp"
#include "spmm_hash_table.hpp"
#include <algorithm>
#include <cstring>


void spmm_solve_row_init()
{
    hash_table::init();
}

int prefetch(sparse_matrix_t *__restrict__ A_ptr, // csr
                        sparse_matrix_t *__restrict__ B_ptr, // csr
                        sparse_matrix_t *__restrict__ C_ptr, // csr
                        std::atomic<intptr_t> *mem_pool_arg, // mem pool
                        bsg_attr_remote int *__restrict__ glbl_updates, // list of hash table updates
                        int n_updates) // number of updates
{
    // prefetch glbl_updates
    for (int i = 0; i < n_updates; i += VCACHE_STRIPE_WORDS) {
        asm volatile ("lw zero,0(%0)" : : "r" (&glbl_updates[i]));
    }
#ifdef PREFETCH_TABLE
    // prefetch nonzeros
    for (int i = 0; i < NONZEROS_TABLE_SIZE; i += VCACHE_STRIPE_WORDS) {
        asm volatile ("lw zero,0(%0)"
                      :
#ifdef ALIGNED_TABLE
                      : "r" (&nonzeros_table[aligned_idx(i)]));
#else // ALIGNED_TABLE
                      : "r" (&nonzeros_table[i]));
#endif // ALIGNED_TABLE
    }
#endif
    bsg_fence();
    return 0;
}


__attribute__((noinline))
void cpy_in(int *__restrict__ dst,
            bsg_attr_remote const int *__restrict__ src)
{
    bsg_unroll(32)
    for (int i = 0; i < VCACHE_STRIPE_WORDS; i++) {
        dst[i] = src[i];
    }
}

extern "C" int kernel_update_stream(sparse_matrix_t *__restrict__ A_ptr, // csr
                                    sparse_matrix_t *__restrict__ B_ptr, // csr
                                    sparse_matrix_t *__restrict__ C_ptr, // csr
                                    std::atomic<intptr_t> *mem_pool_arg, // mem pool
                                    bsg_attr_remote int *__restrict__ glbl_updates, // list of hash table updates
                                    int n_updates) // number of updates
{
    spmm_init(A_ptr, B_ptr, C_ptr, mem_pool_arg);
    spmm_solve_row_init();
    prefetch(A_ptr, B_ptr, C_ptr, mem_pool_arg, glbl_updates, n_updates);
    bsg_cuda_print_stat_start(TAG_ROW_SOLVE);
    int i = 0;
    while (i + VCACHE_STRIPE_WORDS <= n_updates) {
        int updates[VCACHE_STRIPE_WORDS];
        cpy_in(updates, &glbl_updates[i]);

        for (int j = 0; j < ARRAY_SIZE(updates); j++) {
            int idx = updates[j];
            int hsh = hash_table::hash(idx);
            pr_dbg("hash(%d) => 0x%08x\n"
                          , idx
                          , hsh);
            hash_table::update(1, idx, hsh);
        }
        i += VCACHE_STRIPE_WORDS;        
    }
    bsg_cuda_print_stat_end(TAG_ROW_SOLVE);    
    return 0;
}

