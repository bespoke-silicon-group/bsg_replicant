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
        list->head.next = nullptr;
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
        node->next = nullptr;
    }

    /**
     * pop from the front of the list
     */
    static inline list_node_t * list_pop_front(
        list_t *list
        ) {
        list_node_t *tmp = list->head.next;
        list->head.next = tmp->next;
        tmp->next = nullptr;
        // update tail if necessary
        if (list->head.next == nullptr)
            list->tail = &list->head;
        return tmp;
    }


    /**
     * clear a list
     */
    static inline void list_clear(
        list_t *list
        ) {
        list->head.next = nullptr;
        list->tail = &list->head;
    }

    /**
     * check if list is empty
     */
    static inline int list_empty(
        list_t *list
        ) {
        return list->tail == &list->head;
    }
    /**
     * move a list
     */
    static inline void list_move(
        list_t *dst
        ,list_t *src
        ) {
        dst->head.next = src->head.next;
        dst->tail = (list_empty(src) ? &dst->head : src->tail);
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
    static partial_t __local_partial_pool[SPMM_SOLVE_ROW_LOCAL_DATA_WORDS/sizeof(partial_t)];
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
        bsg_print_hexadecimal(0xBEADBEAD);
        // prioritize tile group memory
        list_node_t *tmp;
        if (list_empty(&free_tg)) {
            // allocate from offchip memory
            if (list_empty(&free_offchip)) {                
                // use malloc to allocate a new chunk
                bsg_print_hexadecimal(0xFFFF0000);                
                partial_t *parts = (partial_t*)spmm_malloc(sizeof(partial_t) * VCACHE_STRIPE_WORDS);
                int i = 0;
                for (; i < VCACHE_STRIPE_WORDS-1; i++) {
                    parts[i].tbl.next = &parts[i+1].tbl;
                }
                parts[i].tbl.next = nullptr;
                free_offchip.tail->next = &parts[0].tbl;
                free_offchip.tail = &parts[i].tbl;
                // pop head
                tmp = list_pop_front(&free_offchip);
            } else {
                bsg_print_hexadecimal(0xFFFF0001);
                tmp = list_pop_front(&free_offchip);
            }
        } else {
            // allocate from tile group memory
            bsg_print_hexadecimal(0xFFFF0002);
            tmp = list_pop_front(&free_tg);
        }
        bsg_print_hexadecimal((unsigned)partial_from_node(tmp));
        return partial_from_node(tmp);
    }

    /**
     * free a partial
     */
    static void free_partial(partial_t *part) {
        if (utils::is_dram(part)) {
            bsg_print_hexadecimal(0xEEEEEEEE);
            bsg_print_hexadecimal((unsigned)part);
            list_append(&free_offchip, &part->tbl);
        } else {
            bsg_print_hexadecimal(0xDDDDDDDD);
            bsg_print_hexadecimal((unsigned)part);            
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
        // for iterating over two input lists
        list_node_t *frst_curr = frst_list->head.next;
        list_node_t *scnd_curr = scnd_list->head.next;
        list_node_t *frst_stop = frst_list->tail->next;
        list_node_t *scnd_stop = scnd_list->tail->next;
        bsg_print_hexadecimal((unsigned)frst_curr);
        bsg_print_hexadecimal((unsigned)frst_stop);
        bsg_print_hexadecimal((unsigned)scnd_curr);
        bsg_print_hexadecimal((unsigned)scnd_stop);        
        // build the merged list head
        list_t merged;
        list_init(&merged);
        
        // until we reach end of list
        while (frst_curr != frst_stop
               && scnd_curr != scnd_stop) {
            partial_t *frst = partial_from_node(frst_curr);
            partial_t *scnd = partial_from_node(scnd_curr);
            // prefetch
            float frst_v = frst->iv.val;
            float scnd_v = scnd->iv.val;
            list_node_t *frst_next = frst_curr->next;
            list_node_t *scnd_next = scnd_curr->next;
            bsg_compiler_memory_barrier();
            frst_curr->next = nullptr;
            scnd_curr->next = nullptr;
            if (frst->iv.idx < scnd->iv.idx) {
                // add frst to merged list
                // todo: make this an amoswap?
                // nope - if this lives in tile memory
                // an amoswap will fail
                list_append(&merged, frst_curr);
                frst_curr = frst_next;
            } else if (frst->iv.idx > scnd->iv.idx) {
                // add scnd to merged list
                list_append(&merged, scnd_curr);
                scnd_curr = scnd_next;
            } else {
                // merge one into the other
                if (utils::is_dram(frst)) {
                    // merge frst -> scnd
                    scnd->iv.val = frst_v + scnd_v;
                    // free frst
                    list_append(&free_offchip, frst_curr);
                    // add scnd to merged
                    list_append(&merged, scnd_curr);
                    scnd_curr = scnd_next;
                } else {
                    // merge scnd -> frst
                    frst->iv.val = frst_v + scnd_v;
                    // free scnd
                    if (utils::is_dram(scnd)) {
                        list_append(&free_offchip, scnd_curr);
                    } else {
                        list_append(&free_tg, scnd_curr);
                    }
                    // add frst to merged
                    list_append(&merged, frst_curr);
                    frst_curr = frst_next;
                }
            }
        }
        if (frst_curr != frst_stop) {
            merged.tail->next = frst_curr;
            merged.tail = frst_list->tail;
        } else if (scnd_curr != scnd_stop) {
            merged.tail->next = scnd_curr;
            merged.tail = scnd_list->tail;
        }
        // clear first and scnd
        list_clear(frst_list);
        list_clear(scnd_list);
        // move output
        list_move(merged_list_o, &merged);
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
            bsg_print_int(nz);            
            bsg_print_int(nnz);
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
                //bsg_print_hexadecimal(part[pre]->iv.idx);
            }

            for (int pre = 0; pre < PREFETCH; pre++)
                list_append(&to_merge, &part[pre]->tbl);
        }
#endif
        // strip mining code
        for (; nz < nnz; nz++) {
            bsg_print_int(nz);
            bsg_print_int(nnz);            
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
            //bsg_print_hexadecimal(part->iv.idx);            
            list_append(&to_merge, &part->tbl);
        }

        // merge results
        bsg_print_hexadecimal(0xCAFEBABE);
        bsg_compiler_memory_barrier();
        merge(&row_partials, &to_merge, &row_partials);
        bsg_compiler_memory_barrier();        
        bsg_print_hexadecimal(0xDEADBEEF);
    }
}

void spmm_solve_row(int Ai)
{
    using namespace solve_row_merge;
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
        bsg_print_float(-static_cast<float>(Bi));
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
        
        list_node_t *start = row_partials.head.next;
        list_node_t *stop  = row_partials.tail->next;
        int nz = 0;
        bsg_print_hexadecimal(0xBEBADA55);
        for (list_node_t *node = start; node != stop; ) {
            // prefetch next
            list_node_t *next = node->next;
            bsg_compiler_memory_barrier();
            // fetch idx
            partial_t *part = partial_from_node(node);
            bsg_print_hexadecimal((unsigned)part);
            int idx = part->iv.idx;
            bsg_compiler_memory_barrier();
            // fetch val
            float val = part->iv.val;
            bsg_compiler_memory_barrier();
            // free part
            free_partial(part);
            bsg_compiler_memory_barrier();
            // write to save buffer
            save_buffer[nz].idx = idx;
            save_buffer[nz].val = val;
            nz++;            
            node = next;
        }
        bsg_print_hexadecimal(0xADDAB011);        
        nnz = nz;
        // store as array of partials
        C_lcl.alg_priv_remote_ptr[Ai] = reinterpret_cast<intptr_t>(save_buffer);
    }

    list_clear(&row_partials);
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
    int i;
    for (i = 0; i < ARRAY_SIZE(__local_partial_pool)-1; i++) {
        __local_partial_pool[i].tbl.next = &__local_partial_pool[i+1].tbl;
    }
    __local_partial_pool[i].tbl.next = nullptr;
    free_tg.head.next = &__local_partial_pool[0].tbl;
    free_tg.tail = &__local_partial_pool[i].tbl;
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

