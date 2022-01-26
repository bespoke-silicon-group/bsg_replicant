#pragma once
#include <cstddef>
#include "spmm_solve_row_common.hpp"
#include "bsg_manycore.h"
#include "bsg_tile_config_vars.h"
#include "sparse_matrix.h"
#include "spmm_utils.hpp"
#include "list.hpp"

#ifdef  SPMM_PREFETCH
#ifndef PREFETCH
#define PREFETCH 4
#endif
#endif

namespace solve_row_merge
{
    using namespace list;

    /**
     * partial list node
     */
    typedef struct partial {
        spmm_partial_t  iv; // index/value pair
        list_head_t tbl; // list node for table of found pairs
    } partial_t;

    /**
     * given a list node, produce a pointer to the partial
     */
    static inline partial_t * partial_from_node(list_node_t *lnode)
    {
        return
            reinterpret_cast<partial_t*>(
                reinterpret_cast<char*>(lnode)
                - (offsetof(partial_t, tbl))
                );
    }


    /**
     * pool of free tile group partial results
     */
     static partial_t
     __local_partial_pool[SPMM_SOLVE_ROW_LOCAL_DATA_WORDS/(sizeof(partial_t)/sizeof(int))];

    /**
     * pool of free nodes in tile group memory
     */
    list_t free_tg;

    /**
     * pool of free nodes in off-chip memory
     */
    list_t free_offchip;

    /**
     * allocate a new partial
     */
    static partial_t *new_partial() {
        // prioritize tile group memory
        list_node_t *tmp;
        if (list_empty(&free_tg)) {
            // allocate from offchip memory
            if (list_empty(&free_offchip)) {                
                // use malloc to allocate a new chunk
                partial_t *parts = (partial_t*)spmm_malloc(sizeof(partial_t) * VCACHE_STRIPE_WORDS);
                for (int i = 0; i < VCACHE_STRIPE_WORDS; i++) {
                    list_append(&free_offchip, &parts[i].tbl);
                }
                // pop head
                tmp = list_front(&free_offchip);
                list_pop_front(&free_offchip);
            } else {
                tmp = list_front(&free_offchip);
                list_pop_front(&free_offchip);
            }
        } else {
            // allocate from tile group memory
            tmp = list_front(&free_tg);
            list_pop_front(&free_tg);
        }
        return partial_from_node(tmp);
    }

    /**
     * free a partial
     */
    static void free_partial(partial_t *part) {
        if (utils::is_dram(part)) {
            list_append(&free_offchip, &part->tbl);
        } else {
            list_append(&free_tg, &part->tbl);
        }
    }

    /**
     * merge two sorted partial lists
     */
    static void merge(
        // inputs
        list_t *frst_list
        ,list_t *scnd_list
        // outputs
        ,list_t *merged_list_o
        ) {

        list_t merged;
        list_init(&merged);

        while (!list_empty(frst_list) && !list_empty(scnd_list)) {
            partial_t *frst = partial_from_node(list_front(frst_list));
            partial_t *scnd = partial_from_node(list_front(scnd_list));
            // compare
            if (frst->iv.idx < scnd->iv.idx) {
                // pop from frst
                list_pop_front(frst_list);
                // append first to merged
                list_append(&merged, &frst->tbl);
            } else if (scnd->iv.idx < frst->iv.idx) {
                // pop from scnd
                list_pop_front(scnd_list);
                // append second to merged
                list_append(&merged, &scnd->tbl);
            } else {
                partial_t *from, *into;
                if (utils::is_dram(frst)) {
                    from = frst;
                    into = scnd;
                } else {
                    from = scnd;
                    into = frst;
                }
                into->iv.val += from->iv.val;
                // pop frst and scnd
                list_pop_front(frst_list);
                list_pop_front(scnd_list);
                // append 'into' to merged
                list_append(&merged, &into->tbl);
                // free 'from'
                free_partial(from);
            }
        }        
        // extend what's left        
        if (!list_empty(frst_list)) {
            list_extend(&merged, frst_list);
        } else if (!list_empty(scnd_list)) {
            list_extend(&merged, scnd_list);
        }
        list_move(merged_list_o, &merged);
        return;
    }

    /**
     * current list of all partials for the current row
     */
    list_t row_partials;
    int  n_row_partials;

    /**
     * scalar row product; A[i;j] * B[j;]
     */
    static void scalar_row_product(float Aij, int Bi)
    {
        int off = B_lcl.mnr_off_remote_ptr[Bi];
        //int nnz = B_lcl.mjr_nnz_ptr[Bi];
        int nnz = B_lcl.mnr_off_remote_ptr[Bi+1] - off;

        list_t to_merge;
        list_init(&to_merge);
        
        n_row_partials += nnz;
        
        // stall on off
        kernel_remote_int_ptr_t cols = &B_lcl.mnr_idx_remote_ptr[off];
        kernel_remote_float_ptr_t vals = &B_lcl.val_remote_ptr[off];
        //bsg_print_hexadecimal(0xAAAAAAAA);
        //bsg_fence();
        int nz = 0;
#if defined(SPMM_PREFETCH)
        for (; nz + PREFETCH < nnz; nz += PREFETCH) {
            partial_t *part[PREFETCH];
            for (int pre = 0; pre < PREFETCH; pre++) {
                part[pre] = new_partial();
            }
            bsg_compiler_memory_barrier();
            float Bij [PREFETCH];
            int   Bj  [PREFETCH];
            // prefetch data
            bsg_unroll(8)
            for (int pre = 0; pre < PREFETCH; pre++) {
                Bij[pre] = vals[nz+pre];
                Bj [pre] = cols[nz+pre];
                // this memory barrier get ths compiler to schedule all 8 loads
                // at once; it tries to do the multiplies in the next loop before issueing
                // all loads otherwise
                // we don't want that; we want all loads outstanding first
                bsg_compiler_memory_barrier();
            }
            float Cij  [PREFETCH];

            // ilp fmul
            bsg_unroll(8)
            for (int pre = 0; pre < PREFETCH; pre++) {
#if defined(SPMM_NO_FLOPS)
                Cij[pre] = Bij[pre];
#else
                Cij[pre]   = Aij * Bij[pre];
#endif
                part[pre]->iv.idx = Bj[pre];
                part[pre]->iv.val = Cij[pre];
            }

            for (int pre = 0; pre < PREFETCH; pre++)
                list_append(&to_merge, &part[pre]->tbl);
        }
#endif
        // strip mining code
        for (; nz < nnz; nz++) {
            float Bij, Cij;
            int   Bj;
            Bij = vals[nz];
            Bj  = cols[nz];

            partial_t *part = new_partial();
#if defined(SPMM_NO_FLOPS)
            Cij = Bij;
#else
            Cij = Aij * Bij;
#endif
            part->iv.idx = Bj;
            part->iv.val = Cij;
            list_append(&to_merge, &part->tbl);
        }

        // merge results
        //bsg_print_hexadecimal(0xBBBBBBBB);
        //bsg_fence();
        bsg_compiler_memory_barrier();
        merge(&row_partials, &to_merge, &row_partials);
        bsg_compiler_memory_barrier();        
    }
}

static inline void spmm_solve_row(
    int Ci
    ,int Ci_off
    ,int Ci_nnz
    )
{
    using namespace solve_row_merge;
    // fetch row meta data
    int off = Ci_off;
    //int nnz = A_lcl.mjr_nnz_remote_ptr[Ai];
    int nnz = Ci_nnz;

    // clear list of partial results
    list_clear(&row_partials);
    n_row_partials = 0;

    // this will stall on 'off'
    kernel_remote_int_ptr_t cols = &A_lcl.mnr_idx_remote_ptr[off];
    kernel_remote_float_ptr_t vals = &A_lcl.val_remote_ptr[off];

    int nz = 0;
#ifdef SPMM_PREFETCH
    for (; nz + PREFETCH < nnz; nz += PREFETCH) {
        int    Bi[PREFETCH];
        float Aij[PREFETCH];
        bsg_unroll(4)
        for (int pre = 0; pre < PREFETCH; pre++) {
            Bi[pre] = cols[nz+pre];
            Aij[pre] = vals[nz+pre];            
        }
        for (int pre = 0; pre < PREFETCH; pre++) {
            scalar_row_product(Aij[pre], Bi[pre]);
        }
    }
#endif
    // for each nonzero entry in row A[i:]
    for (; nz < nnz; nz++) {
        int Bi = cols[nz];
        float Aij = vals[nz];
        scalar_row_product(Aij, Bi);
    }

    nnz = 0;
    if (!list_empty(&row_partials)) {
        constexpr int align = VCACHE_STRIPE_WORDS*sizeof(int);
        int sz = n_row_partials * sizeof(spmm_partial_t);
        int rem = sz % (VCACHE_STRIPE_WORDS * sizeof(int));
        if (rem != 0)
            sz += (align-rem);
        
        spmm_partial_t *save_buffer = (spmm_partial_t*)spmm_malloc(sz);
        int nz = 0;
        while (!list_empty(&row_partials)) {
            partial_t *part = partial_from_node(list_front(&row_partials));
            list_pop_front(&row_partials);
            save_buffer[nz] = part->iv;
            free_partial(part);
            nz++;
        }

        nnz = nz;

        // store as array of partials
        C_lcl.alg_priv_remote_ptr[Ci] = reinterpret_cast<intptr_t>(save_buffer);
    }
    C_lcl.mjr_nnz_remote_ptr[Ci] = nnz;

    // update the global number of nonzeros
    std::atomic<int>* nnzp = reinterpret_cast<std::atomic<int> *>((&C_glbl_p->n_non_zeros));
    nnzp->fetch_add(nnz);
    
}

/**
 * initialize this module
 */
static void spmm_solve_row_init()
{
    using namespace solve_row_merge;
    list_init(&free_tg);
    list_init(&free_offchip);
    list_init(&row_partials);
    for (int i = 0; i < ARRAY_SIZE(__local_partial_pool); i++) {
        list_append(&free_tg, &__local_partial_pool[i].tbl);
    }
    // do we just allocate a huge chunk here?
    return;
}

/**
 * cleanup this module
 */
static void spmm_solve_row_exit()
{
    return;
}
