#include "bsg_manycore.h"
#include "bsg_tile_config_vars.h"
#include "spmm.hpp"
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


#ifndef NONZEROS_TABLE_SIZE
#error "define NONZEROS_TABLE_SIZE"
#endif

// this is the hash table that lives in global memory
__attribute__((section(".dram"), aligned(2 * bsg_global_X * VCACHE_STRIPE_WORDS * sizeof(int))))
static spmm_elt_t *nonzeros_table[bsg_global_X * bsg_global_Y * NONZEROS_TABLE_SIZE];

typedef unsigned hidx_t;

#if defined(ALIGNED_TABLE)
thread static hidx_t block_select;
#endif

#ifndef LOG2_VCACHE_STRIPE_WORDS
#error "Define LOG2_VCACHE_STRIPE_WORDS"
#endif

#ifndef LOG2_GLOBAL_X
#error "Define LOG2_GLOBAL_X"
#endif

#ifndef LOG2_GLOBAL_Y
#error "Define LOG2_GLOBAL_Y"
#endif

#define X_SHIFT                                 \
    (LOG2_VCACHE_STRIPE_WORDS)

#define SOUTH_NOT_NORTH_SHIFT                   \
    ((X_SHIFT)+(LOG2_GLOBAL_X))

#define Y_SHIFT                                 \
    ((SOUTH_NOT_NORTH_SHIFT)+1)

#define HI_SHIFT                                \
    ((Y_SHIFT)+(LOG2_GLOBAL_Y-1))

static void hash_init()
{
#if defined(ALIGNED_TABLE)
    // compute tbl_x
    hidx_t tbl_x = __bsg_x;
    // compute south not north
    hidx_t south_not_north = __bsg_y / (bsg_global_Y/2);
    // compute tbl_y
    hidx_t tbl_y = __bsg_y % (bsg_global_Y/2);
    solve_row_dbg("init: bsg_global_X = %3u, bsg_global_Y = %3u\n"
                  , bsg_global_X
                  , bsg_global_Y);
    solve_row_dbg("init: (x=%3d,y=%3d): tbl_y = %3u, tbl_x = %3u, south_not_north = %3u\n"
                  , __bsg_x
                  , __bsg_y
                  , tbl_y
                  , tbl_x
                  , south_not_north);
    block_select
        = (tbl_x << X_SHIFT)
        | (tbl_y << Y_SHIFT)
        | (south_not_north << SOUTH_NOT_NORTH);
#endif
}

static hidx_t aligned_idx(hidx_t x)
{
    hidx_t hi = x / VCACHE_STRIPE_WORDS;
    hidx_t lo = x % VCACHE_STRIPE_WORDS;
    return (hi << HI_SHIFT)
        |  (block_select)
        |  lo;
}

static int hash(int sx)
{
    hidx_t x = static_cast<hidx_t>(sx);
#if defined(COMPLEX_HASH)
    // maybe do an xor shift
    // maybe just the low bits
    x = ((x >> 16) ^ x) * 0x45d9f3bU;
    x = ((x >> 16) ^ x) * 0x45d9f3bU;
    x = ((x >> 16) ^ x);
#endif
    x = x % NONZEROS_TABLE_SIZE;
#if !defined(ALIGNED_TABLE)
    return x;
#else
    return aligned_idx(x);
#endif
}

#define ELTS_REALLOC_SIZE                       \
    ((VCACHE_STRIPE_WORDS*sizeof(int))/sizeof(spmm_elt_t))

int elts_realloc_size =  ELTS_REALLOC_SIZE;
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
        spmm_elt_t *newelts = (spmm_elt_t*)spmm_malloc(elts_realloc_size*sizeof(spmm_elt_t));
        int i;
        for (i = 0; i < elts_realloc_size-1; i++) {
            newelts[i].tbl_next = &newelts[i+1];
        }
        newelts[elts_realloc_size-1].tbl_next = nullptr;
        free_global_head = &newelts[0];
        solve_row_dbg("  %s: free_global_head = 0x%08x\n"
               , __func__
               , free_global_head);
        elts_realloc_size <<= 1;
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
    solve_row_dbg("  &table[%3d] = 0x%08x\n"
                  , idx
                  , u);    
    solve_row_dbg("  table[%3d] = 0x%08x\n"
                  , idx
                  , p);
    while (p != nullptr) {
        // match?
        if (p->part.idx == idx) {
            solve_row_dbg("  %3d found at 0x%08x\n"
                          , idx
                          , p);
#define      SPMM_NO_FLOPS
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

void spmm_solve_row_init()
{
    solve_row_dbg("init: calling from " __FILE__ "\n");
    // initialize nonzeros table in dram
    solve_row_dbg("init: nonzeros_table[start] = 0x%08x\n"
                  , &nonzeros_table[0]);
    solve_row_dbg("init: nonzeros_table[end]   = 0x%08x\n"
                  , &nonzeros_table[array_size(nonzeros_table)-1]);
    hash_init();
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

        for (int j = 0; j < array_size(updates); j++) {
            int idx = updates[j];
            int hsh = hash(idx);
            solve_row_dbg("hash(%d) => 0x%08x\n"
                          , idx
                          , hsh);
            spmm_update_table(1, idx, hsh);
        }
        i += VCACHE_STRIPE_WORDS;        
    }
    bsg_cuda_print_stat_end(TAG_ROW_SOLVE);    
    return 0;
}

