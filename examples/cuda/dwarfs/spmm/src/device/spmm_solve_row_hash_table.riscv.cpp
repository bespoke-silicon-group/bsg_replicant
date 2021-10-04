#include "bsg_manycore.h"
#include "bsg_tile_config_vars.h"
#include "sparse_matrix.h"
#include "spmm_solve_row.hpp"
#include <algorithm>
#include <cstring>

typedef struct spmm_elt {
    spmm_partial_t part; // data
    spmm_elt    *bkt_next; // list for this table bucket
    spmm_elt    *tbl_next; // list for all elements in the table, used in available list
} spmm_elt_t;

// first we source from our local pool of available items
static thread spmm_elt_t local_elt_pool[SPMM_SOLVE_ROW_LOCAL_DATA_WORDS*sizeof(int)/sizeof(spmm_elt_t)];

#define solve_row_dbg(fmt, ...)                 \
    pr_dbg("%d: " fmt, __bsg_id, ##__VA_ARGS__)

#define array_size(x)                           \
    (sizeof(x)/sizeof(x[0]))

// linked list of all entries in the table
static thread spmm_elt_t *tbl_head  = nullptr;
static thread int tbl_num_entries = 0;

// linked list of available free frames in local memory
static thread spmm_elt_t *free_local_head = nullptr;

// linked list of available free frames in local memory
static thread spmm_elt_t *free_global_head = nullptr;

// this is the hash table that lives in global memory
static thread spmm_elt_t **nonzeros_table = nullptr;


#define NONZEROS_TABLE_SIZE                     \
    (1024)

static int hash(int sx)
{
    unsigned x = static_cast<unsigned>(sx);
#ifdef COMPLEX_HASH
    x = ((x >> 16) ^ x) * 0x45d9f3bU;
    x = ((x >> 16) ^ x) * 0x45d9f3bU;
    x = ((x >> 16) ^ x);
#endif
    return x % NONZEROS_TABLE_SIZE;
}

// never allocate fewer than a cache line
#define ELTS_REALLOC_SIZE                       \
    ((VCACHE_STRIPE_WORDS*sizeof(int))/sizeof(spmm_elt_t))

static int elts_realloc_size = ELTS_REALLOC_SIZE;

static spmm_elt_t* alloc_elt()
{
    spmm_elt_t *elt;
    if (free_local_head != nullptr) {
        elt = free_local_head;
        free_local_head = elt->tbl_next;
        elt->tbl_next = nullptr;
        return elt;
    } else if (free_global_head != nullptr) {
        elt = free_global_head;
        free_global_head = elt->tbl_next;
        elt->tbl_next = nullptr;
        return elt;
    } else {
        int n_newelts = elts_realloc_size;
        spmm_elt_t *newelts = (spmm_elt_t*)spmm_malloc(sizeof(spmm_elt_t) * n_newelts);
        int i;
        for (i = 0; i < n_newelts-1; i++) {
            newelts[i].tbl_next = &newelts[i+1];
        }
        newelts[n_newelts-1].tbl_next = nullptr;
        free_global_head = &newelts[0];
        // next time allocate twice as many
        elts_realloc_size <<= 1;

        solve_row_dbg("  %s: free_global_head = 0x%08x\n"
               , __func__
               , free_global_head);

        return alloc_elt();
    }
}

static void free_elt(spmm_elt_t *elt)
{
    intptr_t eltaddr = reinterpret_cast<intptr_t>(elt);
    elt->bkt_next = nullptr;
    if (!(eltaddr & 0x80000000)) {
        // local
        elt->tbl_next = free_local_head;
        free_local_head = elt;
    } else {
        // global
        elt->tbl_next = free_global_head;
        free_global_head = elt;
    }
}

/**
 * Update the non-zeros table
 * v    - floating point value to add/insert
 * idx  - the hash table key
 * hidx - the hash(idx)
 */
static
void spmm_update_table(float v, int idx, int hidx)
{
    spmm_elt_t **u = &nonzeros_table[hidx];    
    spmm_elt_t  *p = nonzeros_table[hidx];
    solve_row_dbg("  table[%3d] = 0x%08x\n"
                  , idx
                  , p);
    while (p != nullptr) {
        // match?
        if (p->part.idx == idx) {
            solve_row_dbg("  %3d found at 0x%08x\n"
                          , idx
                          , p);
#if !defined(SPMM_NO_FLOPS)
            p->part.val += v; // flw; fadd; fsw
#else 
            p->part.val  = v; // fsw
#endif
            return;
        }
        u = &p->bkt_next;
        p = p->bkt_next;
    }
    // allocate a hash item
    p = alloc_elt();
    solve_row_dbg("  %3d not found, inserting at 0x%08x\n"
                  , idx
                  , p);
    // set item parameters
    p->part.idx = idx;
    p->part.val = v;
    p->bkt_next = nullptr;
    p->tbl_next = tbl_head;
    tbl_head = p;
    // update last
    *u = p;
    tbl_num_entries++;
    return;
}

/**
 * Solve A[i;j] * B[j;]
 */
__attribute__((noinline))
void spmm_scalar_row_product(float Aij, int Bi)
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
#if defined(SPMM_NO_FLOPS)
        float Cij = Aij;
#else
        float Cij = Aij * Bij;
#endif
        // compute hash
        int idx = hash(Bj);

        // perform symbol table lookup/update
        spmm_update_table(Cij,Bj, idx);
    }
}

#ifdef __KERNEL_SCALAR_ROW_PRODUCT__
extern "C" kernel_spmm_scalar_row_product(sparse_matrix_t *__restrict A_ptr, // csr
                                          sparse_matrix_t *__restrict B_ptr, // csr
                                          sparse_matrix_t *__restrict C_ptr, // csr
                                          std::atomic<intptr_t> *mem_pool_arg, // mem pool
                                          float Aij,
                                          int Bi)
{
    spmm_init(A_ptr, B_ptr, C_ptr, mem_pool_arg);
    bsg_cuda_print_stat_start(TAG_ROW_SOLVE);
    spmm_scalar_row_product(Aij, Bi);
    bsg_cuda_print_stat_end(TAG_ROW_SOLVE);
}
#endif

void spmm_solve_row_init()
{
    solve_row_dbg("init: calling from " __FILE__ "\n");
    // initialize nonzeros table in dram
    nonzeros_table = (spmm_elt_t**)spmm_malloc(sizeof(spmm_elt_t*) * NONZEROS_TABLE_SIZE);
    solve_row_dbg("init: nonzeros_table   = 0x%08x\n"
           , reinterpret_cast<unsigned>(nonzeros_table));

    // initialize list of local nodes
    int i;
    if (array_size(local_elt_pool) > 0) {
        free_local_head = &local_elt_pool[0];
        for (i = 0; i < array_size(local_elt_pool)-1; i++) {
            local_elt_pool[i].tbl_next = &local_elt_pool[i+1];
        }
        local_elt_pool[array_size(local_elt_pool)-1].tbl_next = nullptr;
        solve_row_dbg("init: local_elt_pool[N-1]=0x%08x\n"
               , &local_elt_pool[array_size(local_elt_pool)-1]);
    }
    solve_row_dbg("init: free_local_head  = 0x%08x\n", free_local_head);
    solve_row_dbg("init: free_global_head = 0x%08x\n", free_global_head);
}

void spmm_solve_row(int Ai)
{
    //bsg_print_int(Ai);
    solve_row_dbg("solving for row %3d\n", Ai);
    // set the number of partials to zero
    tbl_num_entries = 0;

    // fetch row meta data
    int off = A_lcl.mnr_off_remote_ptr[Ai];
    int nnz = A_lcl.mjr_nnz_remote_ptr[Ai];

    // this will stall on 'off'
    kernel_remote_int_ptr_t cols = &A_lcl.mnr_idx_remote_ptr[off];
    kernel_remote_float_ptr_t vals = &A_lcl.val_remote_ptr[off];

    // for each nonzero entry in row A[i:]
    for (int nonzero = 0; nonzero < nnz; nonzero++) {
        int Bi = cols[nonzero];
        float Aij = vals[nonzero];
        spmm_scalar_row_product(Aij, Bi);
    }

    // insert partials into C
    C_lcl.mjr_nnz_remote_ptr[Ai] = tbl_num_entries;
    spmm_partial_t *parts_glbl = (spmm_partial_t*)spmm_malloc(sizeof(spmm_partial_t)*tbl_num_entries);

    //bsg_print_int(tbl_num_entries);
    solve_row_dbg("solved row %3d, saving %3d nonzeros to address 0x%08x\n"
                  , Ai
                  , tbl_num_entries
                  , parts_glbl);

    // for each entry in the table
    int j = 0; // tracks nonzero number
    for (spmm_elt_t *e = tbl_head; e != nullptr; ) {
        // save the next poix1nter
        spmm_elt_t *next = e->tbl_next;
        // clear table entry
        nonzeros_table[hash(e->part.idx)] = nullptr;
        // copy to partitions
        solve_row_dbg("  copying from 0x%08x to 0x%08x\n"
                      , reinterpret_cast<unsigned>(e)
                      , reinterpret_cast<unsigned>(&parts_glbl[j]));
        parts_glbl[j++] = e->part;
        // free entry
        free_elt(e);
        // continue
        e = next;
    }

#if !defined(SPMM_NO_SORTING)
    std::sort(parts_glbl, parts_glbl+tbl_num_entries, [](const spmm_partial_t &l, const spmm_partial_t &r) {
            return l.idx < r.idx;
        });
#endif
    tbl_head = nullptr;

    // store as array of partials in the alg_priv_ptr field
    C_lcl.alg_priv_remote_ptr[Ai] = reinterpret_cast<intptr_t>(parts_glbl);

    // update the global number of nonzeros
    std::atomic<int>* nnzp = reinterpret_cast<std::atomic<int> *>((&C_glbl_p->n_non_zeros));
    nnzp->fetch_add(tbl_num_entries);
}

#ifdef __KERNEL_SOLVE_ROW__
extern "C" int kernel_solve_row(sparse_matrix_t *__restrict A_ptr, // csr
                                sparse_matrix_t *__restrict B_ptr, // csr
                                sparse_matrix_t *__restrict C_ptr, // csr
                                std::atomic<intptr_t> *mem_pool_arg, // mem pool
                                int Ai)
{
    spmm_init(A_ptr, B_ptr, C_ptr, mem_pool_arg);
    spmm_solve_row_init();
    bsg_cuda_print_stat_start(TAG_ROW_SOLVE);
    spmm_solve_row(Ai);
    bsg_cuda_print_stat_end(TAG_ROW_SOLVE);
    return 0;
}
#endif
