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

// this is the hash table that lives in global memory
static thread spmm_elt_t **nonzeros_table = nullptr;


#ifndef NONZEROS_TABLE_SIZE
#error "define NONZEROS_TABLE_SIZE"
#endif

static int hash(int x)
{
    x = ((x >> 16) ^ x) * 0x45d9f3bU;
    x = ((x >> 16) ^ x) * 0x45d9f3bU;
    x = ((x >> 16) ^ x);
    return x % NONZEROS_TABLE_SIZE;
}

#define ELTS_REALLOC_SIZE                       \
    ((VCACHE_STRIPE_WORDS*sizeof(int))/sizeof(spmm_elt_t))

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
        spmm_elt_t *newelts = (spmm_elt_t*)spmm_malloc(sizeof(spmm_elt_t) * ELTS_REALLOC_SIZE);
        int i;
        for (i = 0; i < ELTS_REALLOC_SIZE-1; i++) {
            newelts[i].tbl_next = &newelts[i+1];
        }
        newelts[ELTS_REALLOC_SIZE-1].tbl_next = nullptr;
        free_global_head = &newelts[0];
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

extern "C" int kernel_update_stream(sparse_matrix_t *__restrict A_ptr, // csr
                                    sparse_matrix_t *__restrict B_ptr, // csr
                                    sparse_matrix_t *__restrict C_ptr, // csr
                                    std::atomic<intptr_t> *mem_pool_arg, // mem pool
                                    bsg_attr_remote int *__restrict glbl_updates, // list of hash table updates
                                    int n_updates) // number of updates
{
    spmm_init(A_ptr, B_ptr, C_ptr, mem_pool_arg);
    spmm_solve_row_init();
    bsg_cuda_print_stat_start(TAG_ROW_SOLVE);
    int i = 0;
    while (i + VCACHE_STRIPE_WORDS <= n_updates) {
        int updates[VCACHE_STRIPE_WORDS];
        for (int j = 0; j < array_size(updates); j++)
            updates[j] = glbl_updates[i+j];        

        for (int j = 0; j < array_size(updates); j++) {
            int idx = updates[j];
            int hsh = hash(idx);
            spmm_update_table(0.0f, idx, hsh);
        }
        i += VCACHE_STRIPE_WORDS;        
    }
    bsg_cuda_print_stat_end(TAG_ROW_SOLVE);    
    return 0;
}
