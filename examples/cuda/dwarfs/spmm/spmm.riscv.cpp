#include "bsg_manycore.h"
#include "bsg_tile_config_vars.h"
#include "sparse_matrix.h"
#include "bsg_mcs_mutex.h"
#include "bsg_manycore_atomic.h"
#include <cstdint>
#include <atomic>
#include <algorithm>
#include <cstring>
#include <functional>

#define BSG_TILE_GROUP_X_DIM bsg_tiles_X
#define BSG_TILE_GROUP_Y_DIM bsg_tiles_Y
#include "bsg_tile_group_barrier.h"

INIT_TILE_GROUP_BARRIER(rbar, cbar, 0, bsg_tiles_X-1, 0, bsg_tiles_Y-1);

#define THREADS                                 \
    (bsg_tiles_X*bsg_tiles_Y)

#define FRAME_SIZE                              \
    (1024 + VCACHE_STRIPE_WORDS*sizeof(int))

typedef struct col_val {
    int   col;
    float val;
} col_val_t;

typedef struct col_val_link {
    col_val_t col_val;
    col_val_link *next;
} col_val_link_t;


static std::atomic<intptr_t> *mem_pool = nullptr;

static sparse_matrix_t A; // local copy of A
static sparse_matrix_t B; // local copy of B
static sparse_matrix_t C; // local copy of C

static void *new_slab(intptr_t size)
{
    intptr_t slab = mem_pool->fetch_add(size, std::memory_order_acquire);
    return reinterpret_cast<void*>(slab);
}

#define LOCAL_DATA_WORDS (512+256)
typedef struct partial {
    int   col;
    float data;
} partial_t;

#define LOCAL_PARTIALS \
    (LOCAL_DATA_WORDS * sizeof(int) / sizeof(partial_t))

static partial_t partials[LOCAL_PARTIALS];
int n_partials = 0;

#ifdef  DEBUG
#define pr_dbg(fmt, ...)                        \
    do { bsg_printf(fmt, ##__VA_ARGS__); } while (0)
#else
#define pr_dbg(...)
#endif

static void scalar_row_product(int Bi, float Aij)
{
    int off = B.mnr_off_ptr[Bi];
    int nnz = B.mjr_nnz_ptr[Bi];
    // stall on off
    kernel_remote_int_ptr_t cols = &B.mnr_idx_ptr[off];
    kernel_remote_float_ptr_t vals = &B.val_ptr[off];
    // Max: this loop may be partially vectorizable
    // we can have two loops where we fetch up to N nonzeros
    // at once and do fmull and store them somewhere
    // then we do hash insertion.
    //
    // foreach nonzero entry in row B[i:]
    pr_dbg("   + B_%d has %d nonzeros\n", Bi, nnz);
    for (int nonzero = 0; nonzero < nnz; ++nonzero) {
        float Bij = vals[nonzero];
        int Bj    = cols[nonzero];

        // compute partial
        float Cij = Aij * Bij;

        // binary search on our local partials
        partial_t newpartial = {
            .col = Bj,
            .data = Cij,
        };

        partial_t *p = std::lower_bound(
            partials
            , partials+n_partials
            , newpartial
            , [](const partial_t &lhs, const partial_t &rhs) { return lhs.col < rhs.col; }
            );

        pr_dbg("     B_%d:", Bj);
        if (p == partials+n_partials) {
            pr_dbg(" not found, inserting at %d\n", n_partials);
            *p = newpartial;
            n_partials++;
        } else if (p->col == Bj) {
            // partial found
            pr_dbg(" found at %d\n", p-&partials[0]);
            p->data += Cij;
        } else {
            pr_dbg(" not found, inserting at %d\n", p-&partials[0]);
            // insert partial for this column
            memmove(p+1, p, sizeof(partial_t) * (n_partials-(p-&partials[0])));
            *p = newpartial;
            n_partials++;
        }
    }
}

template <int UNROLL>
__attribute__((noinline))
void spmm_update_nonzeros(int Ci
                          , int nnz
                          , bsg_attr_remote int       *__restrict idx_ptr
                          , bsg_attr_remote float     *__restrict val_ptr
                          , bsg_attr_remote partial_t *__restrict par_ptr)
{
    int nonzero = 0;
    while (nonzero + UNROLL <= nnz) {
        int   idx_tmp[UNROLL];
        float val_tmp[UNROLL];
        for (int i = 0; i < UNROLL; i++) {
            idx_tmp[i] = par_ptr[nonzero+i].col;
            val_tmp[i] = par_ptr[nonzero+i].data;
        }
        for (int i = 0; i < UNROLL; i++) {
            idx_ptr[nonzero+i] = idx_tmp[i];
            val_ptr[nonzero+i] = val_tmp[i];
        }
        nonzero += UNROLL;
    }
    
    for (; nonzero < nnz; nonzero++) {
        int idx_tmp   = par_ptr[nonzero].col;
        float val_tmp = par_ptr[nonzero].data;
        idx_ptr[nonzero] = idx_tmp;
        val_ptr[nonzero] = val_tmp;
    }

}

extern "C" int spmm(sparse_matrix_t *__restrict A_ptr, // csr
                    sparse_matrix_t *__restrict B_ptr, // csr
                    sparse_matrix_t *__restrict C_ptr, // csr
                    std::atomic<intptr_t> *mem_pool_arg) // mem pool
{
    A = *A_ptr;
    B = *B_ptr;
    C = *C_ptr;
    mem_pool = mem_pool_arg;

    bsg_tile_group_barrier(&rbar, &cbar);
    // foreach row
    for (int Ai = __bsg_id; Ai < A.n_major; Ai += THREADS) {
        // clear partials
        n_partials = 0;

        // solve  row
        int off = A.mnr_off_ptr[Ai];
        int nnz = A.mjr_nnz_ptr[Ai];

        // stall on 'off'
        kernel_remote_int_ptr_t cols = &A.mnr_idx_ptr[off];
        kernel_remote_float_ptr_t vals = &A.val_ptr[off];

        pr_dbg("Solving A_%d\n", Ai);
        // foreach nonzero entry in row A[i:]
        for (int nonzero = 0; nonzero < nnz; ++nonzero) {
            int Bi  = cols[nonzero];
            int Aij = vals[nonzero];
            // calculate Aij * Bi
            pr_dbg("   + A_%d_%d x B_%d\n", Ai, Bi, Bi);
            scalar_row_product(Bi, Aij);
        }

        // insert partials into C
        // store as array of partials in the alg_priv_data field
        C.mjr_nnz_ptr[Ai] = n_partials;
        partial_t *nonzeros = reinterpret_cast<partial_t*>(new_slab(sizeof(partial_t) * n_partials));
        memcpy(nonzeros, partials, sizeof(partial_t) * n_partials);
        C.alg_priv_data[Ai] = reinterpret_cast<intptr_t>(nonzeros);
        pr_dbg("A_%d: %d nonzeros\n", Ai, n_partials);
        // atomically add the number of nonzeros
        bsg_amoadd(&C_ptr->n_non_zeros, n_partials);
    }
    pr_dbg("%d Nonzeros Computed\n", C_ptr->n_non_zeros);
    bsg_print_hexadecimal(0x11111111);

    // sync
    bsg_tile_group_barrier(&rbar, &cbar);
    C = *C_ptr;

    // compute offsets for each row
    // Max: Figure out a way to coalesce this
    for (int Ci = __bsg_id; Ci < C.n_major; Ci += THREADS) {
        int nnz = C_ptr->mjr_nnz_ptr[Ci];
        // add to each row after
        for (int Cip = Ci+1; Cip < C.n_major; Cip++) {
            asm volatile ("amoadd.w zero, %[nnz], %[mem]" :: [mem] "m" (C.mnr_off_ptr_vanilla[Cip]), [nnz] "r"(nnz));
        }
    }

    // sync
    if (__bsg_id == 0) {
        C_ptr->mnr_idx_ptr_vanilla = (kernel_int_ptr_t)(new_slab(sizeof(int) * C_ptr->n_non_zeros));
        C_ptr->val_ptr_vanilla = (kernel_float_ptr_t)(new_slab(sizeof(float) * C_ptr->n_non_zeros));
    }

    bsg_print_hexadecimal(0x22222222);
    bsg_tile_group_barrier(&rbar, &cbar);


    for (int Ci = __bsg_id; Ci < C.n_major; Ci += THREADS) {
        int nnz = C_ptr->mjr_nnz_ptr[Ci];
        int off = C_ptr->mnr_off_ptr[Ci];
        kernel_remote_int_ptr_t idx_ptr = &C_ptr->mnr_idx_ptr[off];
        kernel_remote_float_ptr_t val_ptr = &C_ptr->val_ptr[off];

        // use a union to hack around clang
        union {
            partial_t *vanilla;
            bsg_attr_remote partial_t *__restrict remote;
        } pu;

        pu.vanilla = reinterpret_cast<partial_t *>(C_ptr->alg_priv_data[Ci]);
        spmm_update_nonzeros<8>(Ci, nnz, idx_ptr, val_ptr, pu.remote);
    }

    bsg_print_hexadecimal(0x33333333);
    bsg_tile_group_barrier(&rbar, &cbar);

    return 0;
}

