#include "bsg_manycore.h"
#include "bsg_tile_config_vars.h"
#include "sparse_matrix.h"
#include "bsg_mcs_mutex.h"
#include <cstdint>
#include <atomic>
#include <algorithm>

#ifndef GROUPS
#define GROUPS 1
#endif

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

static void *new_slab()
{
    intptr_t slab = mem_pool->fetch_add(FRAME_SIZE, std::memory_order_acquire);
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


static partial_t* binary_search(const partial_t *partials, int n, nt key) {
    int upper_bound = n-1;
    int lower_bound = 0;
    while (upper_bound >= lower_bound) {
        i = (lower_bound+upper_bound)/2;        
        if (partials[i].col < key) {
            upper_bound = i-1;
        } else if (partials[i].col > key) {
            lower_bound = i+1;
        } else {
            return &partials[i];
        }
    }
    return nullptr;
}

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
    for (int nonzero = 0; nonzero < nnz; ++nonzero) {
        float Bij = vals[nonzero];        
        int Bj    = cols[nonzero];
        // compute partial
        float Cij_partial = Aij * Bij;
        // binary search on our local partials
        partial_t *p = binary_search(partials, n_partials, Bj);
        if (p == nullptr) {
            // append and resort
            if (n_partials == LOCAL_PARTIALS) {
                // death
                bsg_print_int(0xDEAD);
                while (1);
            }
            p = &partials[n_partials++];
            p->col = Bj;
            p->data = Bij;
            std::sort(partials, partials+n_partials, [](const partial &a, const partial &b) {
                    return std::less(a.col, b.col);
                });
        } else {
            // found the partial, update
            p->data += Bij;
        }
    }
}

static int spmm_update_mjr_offset()
{
    
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

    // foreach row
    for (int Ai = __bsg_tile_group_id_x; Ai < A.n_major; Ai += GROUPS) {
        // solve  row
        int off = A.mnr_off_ptr[Ai];
        int nnz = A.mjr_nnz_ptr[Ai];
        // stall on 'off'
        kernel_remote_int_ptr_t cols = &A.mnr_idx_ptr[off];
        kernel_remote_float_ptr_t vals = &A.val_ptr[off];
        // foreach nonzero entry in row A[i:]
        for (int nonzero = 0; nonzero < nnz; ++nonzero) {
            int Bi  = cols[nonzero];
            int Aij = vals[nonzero];
            // calculate Aij * Bi
            scalar_row_product(Bi, Aij);
        }
        // insert partials into C
        // one solution: store as pointers in C.mnr_idx_ptr
        // and then at the end we do one big copy
        C.mjr_nnz_ptr[Ai] = n_partials;
        partial_t *nonzeros = new_slab(sizeof(partial_t) * n_partials);        
        memcpy(partials, nonzeros, sizeof(partial_t) * n_partials);
        C.alg_priv_data[Ai] = reinterpret_cast<intptr_t>(nonzeros);
    }
    // Max: make seperate kernel because it's easier to analyze for now...
    // barrier here
    if (__bsg_tile_group_id_x == 0 &&
        __bsg_x == 0 &&
        __bsg_y == 0) {
        int *cols   = new_slab(sizeof(int) * C.n_major);
        float *vals = new_slab(sizeof(float) * C.n_major);
        C_ptr->mnr_idx_ptr = cols;
        C_ptr->val_ptr = vals;
    }
    // barrier here
    // note: a broadcast operation would be helpful here
    // something like 'update your neighbor and wake them so they can update their neighbor'
    // this would avoid a hotspot and also would avoid a hotspot when we update a global data structure
    C.mnr_idx_ptr = C_ptr->mnr_idx_ptr;
    C.val_ptr = C_ptr->val_ptr;
    // now everone can update the offset values
    spmm_update_mjr_offset();

    return 0;
}

