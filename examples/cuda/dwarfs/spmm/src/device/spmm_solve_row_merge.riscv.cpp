#include <cstddef>
#include "bsg_manycore.h"
#include "bsg_tile_config_vars.h"
#include "sparse_matrix.h"
#include "spmm_solve_row.hpp"
#include "spmm_utils.hpp"

namespace solve_row_merge
{
    /**
     * simple list head
     */
    typedef struct list_node {
        struct list_node *next;
    } list_node_t;
    typedef list_node_t  list_head_t;
    /**
     * list structure
     */
    typedef struct list {
        list_head_t head;
        list_node_t *tail;
    } list_t;

    /**
     * initialize list
     */
    static inline void list_init(
        list_t *list
        ) {
        list->tail = &list->head;
        list->head.next = &list->head;
    }

    /**
     * clear a list
     */
    static inline void list_clear(
        list_t *list
        ) {
        list_init(list);
    }

    /**
     * check if list is empty
     */
    static inline int list_empty(
        list_t *list
        ) {
        return list->head.next == &list->head;
    }
    /**
     * append to list
     */
    static inline void list_append(
        list_t *list
        ,list_node_t *node
        ) {
        list->tail->next = node;
        list->tail = node;
        node->next = &list->head;
    }
    /**
     * prepend to list
     */
    static inline void list_prepend(
        list_t *list
        ,list_node_t *node
        ) {
        if (list_empty(list))
            list->tail = node;
        
        node->next = list->head.next;
        list->head.next = node;
    }
    /**
     * extend list with another
     */
    static inline void list_extend(
        list_t *extend_to
        ,list_t *with
        ) {
        if (!list_empty(with)) {
            extend_to->tail->next = with->head.next;
            extend_to->tail = with->tail;
            extend_to->tail->next = &extend_to->head;
            list_clear(with);
        }
    }

    static inline list_node_t *list_front(
        list_t *list
        ) {
        return list->head.next;
    }
    
    /**
     * pop from the front of the list
     */
    static inline void list_pop_front(
        list_t *list
        ) {
        if (!list_empty(list)) {            
            list_node_t *tmp = list->head.next;
            list->head.next = tmp->next;
            tmp->next = nullptr;

            // update tail if necessary
            if (list_empty(list))
                list->tail = &list->head;
        }
    }

    /**
     * move a list
     */
    static inline void list_move(
        list_t *dst
        ,list_t *src
        ) {
        if (!list_empty(src) && (dst != src)) {
            dst->head.next = src->head.next;
            dst->tail =  src->tail;
            dst->tail->next = &dst->head;
            list_clear(src);
        }
    }
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
     * list of free nodes in tile group memory
     */
    list_t free_tg;

    /**
     * list of free nodes in off-chip memory
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

        int nz = 0;
#if defined(SPMM_PREFETCH)
#define PREFETCH 4
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
        bsg_compiler_memory_barrier();
        merge(&row_partials, &to_merge, &row_partials);
        bsg_compiler_memory_barrier();        
    }
}

void spmm_solve_row(int Ai)
{
    using namespace solve_row_merge;
    // fetch row meta data
    int off = A_lcl.mnr_off_remote_ptr[Ai];
    //int nnz = A_lcl.mjr_nnz_remote_ptr[Ai];
    int nnz = A_lcl.mnr_off_remote_ptr[Ai+1]-off;

    // clear list of partial results
    list_clear(&row_partials);
    
    // this will stall on 'off'
    kernel_remote_int_ptr_t cols = &A_lcl.mnr_idx_remote_ptr[off];
    kernel_remote_float_ptr_t vals = &A_lcl.val_remote_ptr[off];
    
    // for each nonzero entry in row A[i:]
    for (int nz = 0; nz < nnz; nz++) {
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
        pr_dbg("solve_row: write back\n");
        pr_dbg("&row_partials->head = %p\n", &row_partials.head);
        while (!list_empty(&row_partials)) {
            partial_t *part = partial_from_node(list_front(&row_partials));
            pr_dbg("part = %p\n", part);            
            list_pop_front(&row_partials);
            save_buffer[nz] = part->iv;
            free_partial(part);
            nz++;
        }

        nnz = nz;
        // store as array of partials
        C_lcl.alg_priv_remote_ptr[Ai] = reinterpret_cast<intptr_t>(save_buffer);
        C_lcl.mjr_nnz_remote_ptr[Ai] = nnz;        
    }

    // update the global number of nonzeros
    std::atomic<int>* nnzp = reinterpret_cast<std::atomic<int> *>((&C_glbl_p->n_non_zeros));
    nnzp->fetch_add(nnz);
    
}

/**
 * initialize this module
 */
void spmm_solve_row_init()
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
void spmm_solve_row_exit()
{
    return;
}

/**
 * used for microbenchmarking
 */
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

