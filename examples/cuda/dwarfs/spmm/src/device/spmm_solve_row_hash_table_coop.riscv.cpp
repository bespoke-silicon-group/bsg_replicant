#include "bsg_manycore.h"
#include "bsg_tile_config_vars.h"
#include "sparse_matrix.h"
#include "spmm_solve_row.hpp"
#include "spmm_hash_table.hpp"
#include "spmm_utils.hpp"
#include "bsg_manycore.hpp"
#include "spmm_barrier.hpp"

#include <algorithm>
#include <cstring>

namespace hash_table_coop
{
static thread int   solve_row_done;
static thread int * solve_row_done_ptr[bsg_global_Y/2];

using spmm_elt_t = hash_table::spmm_elt_t;
using hidx_t = hash_table::hidx_t;

#define NZ_BLOCK_SIZE 256
static constexpr hidx_t NZ_TBL_SIZE = NZ_BLOCK_SIZE*(bsg_global_Y/2);

static thread spmm_elt_t *  nonzeros_table[NZ_BLOCK_SIZE];
static thread spmm_elt_t ** nonzeros_table_blocks[bsg_global_Y/2];
static spmm_elt_t* alloc_elt();
static void free_elt(spmm_elt_t *elt);



#define TILE_ELT_POOL_SIZE \
    ((SPMM_SOLVE_ROW_LOCAL_DATA_WORDS-NZ_BLOCK_SIZE)/sizeof(spmm_elt_t))

/**
 * Pool of free entries allocated in tiles
 */
static thread spmm_elt_t local_elt_pool[TILE_ELT_POOL_SIZE];

/**
 * List of all entries in the table
 */
static thread spmm_elt_t *tbl_head;
static thread int         tbl_size;

/**
 * List of available free frames in local memory
 */
static thread spmm_elt_t *free_local_head;
static thread spmm_elt_t *free_local_tail_init; // use for init only

/**
 * List of available free frames in on-chip memory
 */
static thread spmm_elt_t *free_global_head;


/**
 * Next reallocation size, initialize one cache line.
 */
static int elts_realloc_size;

/**
 * Allocate a hash element.
 */
static spmm_elt_t* alloc_elt()
{
    spmm_elt_t *elt;
    // try to allocate from local memory
    if (free_local_head != nullptr) {
        elt = free_local_head;
        free_local_head = elt->tbl_next;
        elt->tbl_next = nullptr;
        return elt;
            // try to allocate from global memory
    } else if (free_global_head != nullptr) {
        elt = free_global_head;
        free_global_head = elt->tbl_next;
        elt->tbl_next = nullptr;
        return elt;
        // allocate more frames from global memory
    } else {
        spmm_elt_t *newelts = (spmm_elt_t*)spmm_malloc(elts_realloc_size*sizeof(spmm_elt_t));
        int i;
        for (i = 0; i < elts_realloc_size-1; i++) {
            newelts[i].tbl_next = &newelts[i+1];
        }
        newelts[elts_realloc_size-1].tbl_next = nullptr;
        free_global_head = &newelts[0];
        pr_dbg("  %s: free_global_head = 0x%08x\n"
               , __func__
               , free_global_head);
        elts_realloc_size <<= 1;
        return alloc_elt();
    }
}

/**
 * Return a hash element to the free pool
 */
static void free_elt(spmm_elt_t *elt)
{
    intptr_t eltaddr = reinterpret_cast<intptr_t>(elt);
    elt->bkt_next = nullptr;
    // belongs in local memory?
    if (!(eltaddr & 0x80000000)) {
        elt->tbl_next = free_local_head;
        free_local_head = elt;
        // belongs in global
    } else {
        elt->tbl_next = free_global_head;
        free_global_head = elt;
    }
}


/**
 * Returns a pointer to a hash bucket
 */
static spmm_elt_t ** hash(int sx)
{
    hidx_t x = static_cast<hidx_t>(sx);
#if defined(COMPLEX_HASH)
    // maybe do an xor shift
    // maybe just the low bits
    x = ((x >> 16) ^ x) * 0x45d9f3bU;
    x = ((x >> 16) ^ x) * 0x45d9f3bU;
    x = ((x >> 16) ^ x);
#endif
    x = x % NZ_TBL_SIZE;

    hidx_t block_off = x % NZ_BLOCK_SIZE;
    hidx_t block     = (x / NZ_BLOCK_SIZE) % (bsg_global_Y/2);
    spmm_elt_t ** bptr = nonzeros_table_blocks[block];
    return &bptr[block_off];
}
/**
 * Update with v, idx, and the compute hash index hidx
 */
static void update(float v, int idx, spmm_elt_t **u)
{
    spmm_elt_t  *p = *u;
    while (p != nullptr) {
        // match?
        if (p->part.idx == idx) {
            p->part.val += v; // flw; fadd; fsw
            return;
        }
        u = &p->bkt_next;
        p = p->bkt_next;
    }
    // allocate a hash item
    p = alloc_elt();
    // set item parameters
    p->part.idx = idx;
    p->part.val = v;
    p->bkt_next = nullptr;
    p->tbl_next = tbl_head;
    tbl_head = p;
    // update last
    *u = p;
    tbl_size++;
    return;
}

    
static void sort_table()
{
    if (tbl_size  == 0)
        return;
    
    spmm_elt_t *sorted_head = tbl_head;
    spmm_elt_t *sorted_tail = tbl_head;
    spmm_elt_t *to_sort = sorted_tail->tbl_next;

    while (to_sort != nullptr) {
        // remove to_sort from list
        sorted_tail->tbl_next = to_sort->tbl_next;
        to_sort->tbl_next = nullptr;
        // insert into sorted list
        if (to_sort->part.idx < sorted_head->part.idx) {
            // special case if goes at start            
            to_sort->tbl_next = sorted_head;
            sorted_head = to_sort;
        } else if (to_sort->part.idx > sorted_tail->part.idx) {
            // special case if goes at end
            to_sort->tbl_next = sorted_tail->tbl_next;
            sorted_tail->tbl_next = to_sort;
            sorted_tail = to_sort;
        } else {
            spmm_elt_t *last, *next;
            last = sorted_head;
            cand = sorted_head->tbl_next;
            
            while (last != sorted_tail) {
                // find first where to_sort < cand
                if (to_sort->part.idx < cand->part.idx) {
                    // insert to_sort between last and cand
                    last->tbl_next = to_sort;
                    to_sort->tbl_next = cand;
                    break;
                }
                last = cand;
                cand = cand->tbl_next;
            }
        }
        // next to sort
        to_sort = sorted_tail->tbl_next;
    }
}

static void init()
{
    if (utils::is_cache_adjacent(utils::y())) {
        for (int i = 0; i < bsg_global_Y/2; i++) {
            nonzeros_table_blocks[i]
                = (spmm_elt_t**) utils::is_south_not_north(utils::y())
                ? bsg_tile_group_remote_pointer(utils::x(), utils::y()-i, &nonzeros_table[0])
                : bsg_tile_group_remote_pointer(utils::x(), utils::y()+i, &nonzeros_table[0]);
            solve_row_done_ptr[i]
                = utils::is_south_not_north(utils::y())
                ? bsg_tile_group_remote_pointer(utils::x(), utils::y()-i, &solve_row_done)
                : bsg_tile_group_remote_pointer(utils::x(), utils::y()+i, &solve_row_done);
            pr_dbg("%s: nonzeros_table_blocks[%d] = 0x%08x\n"
                   , __func__
                   , i
                   , nonzeros_table_blocks[i]);
        }
    }

    bsg_compiler_memory_barrier();
    barrier::spmm_barrier();

    // init free frames
    spmm_elt_t *free_frame = bsg_tile_group_remote_pointer(utils::x(), utils::y(), &local_elt_pool[0]);
    free_local_head = free_frame;
    free_global_head = nullptr;

    spmm_elt_t *free_frame_last = free_frame + ARRAY_SIZE(local_elt_pool) - 1;
    for (; free_frame != free_frame_last; free_frame++) {
        free_frame->bkt_next = nullptr;
        free_frame->tbl_next = free_frame + 1;
    }
    free_frame_last->bkt_next = nullptr;
    free_frame_last->tbl_next = nullptr;
    free_local_tail_init = free_frame_last;

    pr_dbg("%s: free_local_head = 0x%08x\n"
           , __func__
           , free_local_tail_init);

    bsg_compiler_memory_barrier();
    barrier::spmm_barrier();

    // init realloc size
    elts_realloc_size = (VCACHE_STRIPE_WORDS*sizeof(int))/sizeof(spmm_elt_t);

    // init tbl
    tbl_head = nullptr;
    tbl_size = 0;

    for (int i = 0; i < ARRAY_SIZE(nonzeros_table); i++) {
        nonzeros_table[i] = nullptr;
    }
    // init free frames with your friends frames
    bsg_compiler_memory_barrier();
    barrier::spmm_barrier();

    // free
    if (utils::is_cache_adjacent(utils::y())) {
        for (int i = 0; i < bsg_global_Y/2; i++) {
            spmm_elt_t **head, **tail;
            head = bsg_tile_group_remote_pointer(
                utils::x()
                , utils::y() + (utils::is_south_not_north(utils::y()) ? -i : i)
                , &free_local_head
                );
            tail = bsg_tile_group_remote_pointer(
                utils::x()
                , utils::y() + (utils::is_south_not_north(utils::y()) ? -i : i)
                , &free_local_tail_init
                );
            free_local_tail_init->tbl_next = *head;
            free_local_tail_init = *tail;
            pr_dbg("%s: free_local_tail_init = 0x%08x\n"
                   , __func__
                   , free_local_tail_init);
        }
    }

    solve_row_done = 0;

    bsg_compiler_memory_barrier();
    barrier::spmm_barrier();

    if (!utils::is_cache_adjacent(utils::y())) {
        // wait until solve_row_done
        bsg_wait_local_int_asm(&solve_row_done, 1);
        pr_dbg("%s: released\n", __func__);
    }
}

static void exit()
{
    if (utils::is_cache_adjacent(utils::y())) {
        for (int i = 0; i < ARRAY_SIZE(solve_row_done_ptr); i++) {
            *solve_row_done_ptr[i] = 1;
        }
    }
}
}

void spmm_solve_row_init()
{
    hash_table_coop::init();
}

void spmm_solve_row_exit()
{
    hash_table_coop::exit();
}

/**
 * Solve A[i;j] * B[j;]
 */
static void spmm_scalar_row_product(float Aij, int Bi)
{
    int off = B_lcl.mnr_off_remote_ptr[Bi];
    //int nnz = B_lcl.mjr_nnz_ptr[Bi];
    int nnz = B_lcl.mnr_off_remote_ptr[Bi+1] - off;

    // stall on off
    kernel_remote_int_ptr_t cols = &B_lcl.mnr_idx_remote_ptr[off];
    kernel_remote_float_ptr_t vals = &B_lcl.val_remote_ptr[off];

    int nz = 0;
#if defined(SPMM_PREFETCH)
#define PREFETCH 4
    for (; nz + PREFETCH < nnz; nz += PREFETCH) {
        float Bij [PREFETCH];
        int   Bj  [PREFETCH];
        // prefetch data
        bsg_unroll(8)
        for (int pre = 0; pre < PREFETCH; pre++) {
            Bij[pre] = vals[nz+pre];
            Bj [pre] = cols[nz+pre];
        }
        float Cij [PREFETCH];
        hash_table_coop::spmm_elt_t** u [PREFETCH];
        // ilp fmul and hash function comp.
        bsg_unroll(8)
        for (int pre = 0; pre < PREFETCH; pre++) {
            Cij[pre]   = Aij * Bij[pre];
            u[pre]  = hash_table_coop::hash(Bj[pre]);
        }
        // update hash table
        // serialized
        // possible optimization is to unroll this loop
        // by hand to extract more mlp
        //
        // this is not as straightforward as we'd expect
        // given that collisions may occur and linked
        // list traversal may occur
        for (int pre = 0; pre < PREFETCH; pre++) {
            hash_table_coop::update(Cij[pre], Bj[pre], u[pre]);
        }
    }
#endif

    for (; nz < nnz; nz++) {
        float Bij = vals[nz];
        int Bj    = cols[nz];
        float Cij = Aij * Bij;

        // compute hash
        hash_table_coop::spmm_elt_t **u = hash_table_coop::hash(Bj);

        // perform symbol table lookup
        hash_table_coop::update(Cij, Bj, u);
    }
}

void spmm_solve_row(int Ai)
{
    //bsg_print_int(Ai);
    // pr_dbg("solving for row %3d\n", Ai);
    // set the number of partials to zero
    hash_table_coop::tbl_size = 0;

    // fetch row meta data
    int off = A_lcl.mnr_off_remote_ptr[Ai];
    //int nnz = A_lcl.mjr_nnz_remote_ptr[Ai];
    int nnz = A_lcl.mnr_off_remote_ptr[Ai+1]-off;

    // this will stall on 'off'
    kernel_remote_int_ptr_t cols = &A_lcl.mnr_idx_remote_ptr[off];
    kernel_remote_float_ptr_t vals = &A_lcl.val_remote_ptr[off];

    // for each nonzero entry in row A[i:]
    for (int nz = 0; nz < nnz; nz++) {
        int Bi = cols[nz];
        float Aij = vals[nz];
        spmm_scalar_row_product(Aij, Bi);
    }

    // sort table entries
    hash_table_coop::sort_table();
    
    if (hash_table_coop::tbl_size > 0) {
        // insert partials into C
        C_lcl.mjr_nnz_remote_ptr[Ai] = hash_table_coop::tbl_size;

        // allocate temporary storage for non zeros
        std::size_t size = sizeof(spmm_partial_t)*hash_table_coop::tbl_size;

        // pad up to a cache line
        std::size_t rem = size & (VCACHE_STRIPE_WORDS*sizeof(int)-1);
        if (rem != 0)
            size += (VCACHE_STRIPE_WORDS*sizeof(int) - rem);

        spmm_partial_t *parts_glbl = (spmm_partial_t*)spmm_malloc(size);

        //bsg_print_int(tbl_size);
        pr_dbg("solved row %3d, saving %3d nonzeros to address 0x%08x\n"
               , Ai
               , hash_table_coop::tbl_size
               , parts_glbl);
        
        // for each entry in the table
        int j = 0; // tracks nonzero number
        for (hash_table_coop::spmm_elt_t *e = hash_table_coop::tbl_head; e != nullptr; ) {
            // save the next poix1nter
            spmm_partial_t part;
            hash_table_coop::spmm_elt_t *next;
            // we are hand scheduling this...
            part.idx = e->part.idx;
            bsg_compiler_memory_barrier();
            part.val = e->part.val;
            bsg_compiler_memory_barrier();
            next = e->tbl_next;
            bsg_compiler_memory_barrier();
            // clear table entry
            *hash_table_coop::hash(part.idx) = nullptr;
            // copy to partitions
            pr_dbg("  copying from 0x%08x to 0x%08x\n"
                   , reinterpret_cast<unsigned>(e)
                   , reinterpret_cast<unsigned>(&parts_glbl[j]));
            parts_glbl[j++] = part;
            // free entry
            hash_table_coop::free_elt(e);
            // continue
            e = next;
        }

        hash_table_coop::tbl_head = nullptr;

        // store as array of partials in the alg_priv_ptr field
        C_lcl.alg_priv_remote_ptr[Ai] = reinterpret_cast<intptr_t>(parts_glbl);

        // update the global number of nonzeros
        std::atomic<int>* nnzp = reinterpret_cast<std::atomic<int> *>((&C_glbl_p->n_non_zeros));
        nnzp->fetch_add(hash_table_coop::tbl_size);
    }
}

#ifdef __KERNEL_SOLVE_ROW__
extern "C" int kernel_solve_row(sparse_matrix_t *__restrict__ A_ptr, // csr
                                sparse_matrix_t *__restrict__ B_ptr, // csr
                                sparse_matrix_t *__restrict__ C_ptr, // csr
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
