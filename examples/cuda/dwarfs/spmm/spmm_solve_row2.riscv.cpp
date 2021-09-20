#include "sparse_matrix.h"
#include "spmm_solve_row.hpp"
#include <algorithm>
#include <cstring>

#define LOCAL_DATA_WORDS                                                \
    (3*256)

typedef struct spmm_elt {
    spmm_partial_t part; // data
    spmm_elt    *bkt_next; // list for this table bucket
    spmm_elt    *tbl_next; // list for all elements in the table, used in available list
} spmm_elt_t;

// first we source from our local pool of available items
static thread spmm_elt_t local_elt_pool[LOCAL_DATA_WORDS/sizeof(spmm_elt_t)];

#define array_size(x)                           \
    (sizeof(x)/sizeof(x[0]))

// linked list of all entries in the table
static thread spmm_elt_t *tbl_head  = nullptr;
static thread spmm_elt_t *tbl_tail  = nullptr;
static thread int tbl_num_entries = 0;

// linked list of available free frames in local memory
static thread spmm_elt_t *free_local_head = nullptr;
static thread spmm_elt_t *free_local_tail = nullptr;

// linked list of available free frames in local memory
static thread spmm_elt_t *free_global_head = nullptr;
static thread spmm_elt_t *free_global_tail = nullptr;

static thread spmm_elt_t **nonzeros_table = nullptr;


#define NONZEROS_TABLE_SIZE                     \
    (32 * 1024)

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
        return elt;
    } else if (free_global_head != nullptr) {
        elt = free_global_head;
        free_global_head = elt->tbl_next;
        return elt;
    } else {
        spmm_elt_t *newelts = (spmm_elt_t*)spmm_malloc(sizeof(spmm_elt_t) * ELTS_REALLOC_SIZE);
        int i;
        for (i = 1; i < ELTS_REALLOC_SIZE-1; i++) {
            newelts[i].tbl_next = &newelts[i+1];
        }
        newelts[i].tbl_next = nullptr;
        free_global_head = newelts;
        return &newelts[0];
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
        float Cij = Aij * Bij;

        // compute hash
        int idx = hash(Bj);
        spmm_elt_t *p = nonzeros_table[idx];
        pr_dbg("  lookup[%d] = 0x%08x\n"
               , Bj
               , reinterpret_cast<unsigned>(p));
        // fetch entry
        spmm_elt_t *e;        
        if (p == nullptr) {
            // allocate an elt
            e = alloc_elt();
            pr_dbg("  not found: inserting %d with 0x%08x\n"
                   , Bj
                   , reinterpret_cast<unsigned>(e));            
            e->part.idx = Bj;
            e->part.val = Cij;                    
            // zero out bucket
            e->bkt_next = nullptr;
            // push e on to the table list
            e->tbl_next = tbl_head;
            tbl_head = e;
            // insert into table
            nonzeros_table[idx] = e;
            // increment number of entries
            tbl_num_entries++;
        } else if (p->part.idx == Bj) {
            pr_dbg("  found: updating %d\n", Bj);
            // matches
            p->part.val += Cij;
        } else {
            // collision, go through bucket
            while (p->bkt_next != nullptr) {
                p = p->bkt_next;
                if (p->part.idx == Bj) {
                    pr_dbg("  found: updating %d\n", Bj);
                    p->part.val += Cij;
                    break;
                }
            }
            // add to list
            if (p->part.idx != Bj) {
                // allocate an elt                
                e = alloc_elt();
                pr_dbg("  colision found: inserting %d with 0x%08x\n"
                       , Bj
                       , reinterpret_cast<unsigned>(e));                
                e->part.idx = Bj;
                e->part.val = Cij;                
                // insert into buckets list
                e->bkt_next = nullptr;
                p->bkt_next = e;
                // insert into table list
                // Max: could use an amoswap here for more efficient swapping
                e->tbl_next = p->tbl_next;
                p->tbl_next = e;
                // increment number of entries
                tbl_num_entries++;                
            }
        }
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
    pr_dbg(__FILE__ ": Calling spmm_solve_row_init\n");
    nonzeros_table = (spmm_elt_t**)spmm_malloc(sizeof(spmm_elt_t*) * NONZEROS_TABLE_SIZE);
    int i;
    for (i = 0; i < array_size(local_elt_pool)-1; i++) {
        local_elt_pool[i].tbl_next = &local_elt_pool[i+1];
    }
    local_elt_pool[i].tbl_next = nullptr;
    free_local_head = &local_elt_pool[0];
}

void spmm_solve_row(int Ai)
{
    bsg_print_int(Ai);
    pr_dbg("Solving for row %d\n", Ai);
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

    pr_dbg("Solved row %d, saving %d nonzeros to address 0x%08x\n"
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
        pr_dbg("  copying from 0x%08x to 0x%08x\n"
               , reinterpret_cast<unsigned>(e)
               , reinterpret_cast<unsigned>(&parts_glbl[j]));        
        parts_glbl[j++] = e->part;
        // free entry
        free_elt(e);
        // continue
        e = next;
    }

    std::sort(parts_glbl, parts_glbl+tbl_num_entries, [](const spmm_partial_t &l, const spmm_partial_t &r) {
            return l.idx < r.idx;
        });
    
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
    bsg_cuda_print_stat_start(TAG_ROW_SOLVE);
    spmm_solve_row(Ai);
    bsg_cuda_print_stat_end(TAG_ROW_SOLVE);    
}
#endif
