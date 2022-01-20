#pragma once
#include "spmm.hpp"
#include "spmm_solve_row_common.hpp"
#include "spmm_utils.hpp"
#include "list.hpp"

namespace vmerge {
using namespace list;
enum {
    FREE_100
    ,FREE_1024
    ,FREE_16384
    ,FREE_N
};
// list of free buffers
static list_t free_buffers [FREE_N];

/**
 * used for managing pools of free buffers
 */
typedef struct pvec {
    list_node_t    free;
    int            size;
    spmm_partial_t v[1];
} pvec_t;

/**
 * used for pool of free buffers in local memory
 */
template <int N>
struct pvec_static {
    pvec_t pv;
    spmm_partial_t v[N-1]; // subtract 1 for first included in pvec_t.
};

/**
 * return a pvec header from a list node
 */
static pvec_t *pvec_from_list_node(list_node_t *node)
{
    return reinterpret_cast<pvec_t*>(
        reinterpret_cast<char*>(node)
        - offsetof(pvec_t, free)
        );
}

/**
 * return a pvec header from the first partial in a buffer
 */
static pvec_t *pvec_from_partial(spmm_partial_t *p)
{
    return reinterpret_cast<pvec_t*>(
        reinterpret_cast<char *>(p)
        - offsetof(pvec_t, v)
        );
}

/**
 * maps a size to a bucket with the smallest buffers
 * that can fit size
 */
static int bucket(int size)
{
    // decide bucket
    if (size <= 100) {
        return FREE_100;
    } else if (size <= 1024) {
        return FREE_1024;
    } else {
        return FREE_16384;
    }
    return 0;
}
/**
 * maps a bucket to a size
 */
static int bucket_to_size(int bkt) {
    switch (bkt) {
    case FREE_100:   return 100;
    case FREE_1024:  return 1024;
    case FREE_16384: return 16384;
    default:         return 0;
    }
}
/**
 * allocate a new buffer of partials large enough to fit 'size'
 * partials
 */
static spmm_partial_t *new_pvec(int size)
{
    int bkt = bucket(size);
    pvec_t *pv;
    if (!list_empty(&free_buffers[bkt])) {
        list_node_t *n = list_front(&free_buffers[bkt]);
        list_pop_front(&free_buffers[bkt]);
        pr_dbg("%p allocd from pool %d: size = %d\n"
               ,&pvec_from_list_node(n)->v[0]
               ,bkt
               ,pvec_from_list_node(n)->size
            );
        return &pvec_from_list_node(n)->v[0];
    } else {
        int alloc_size = size * sizeof(spmm_partial_t);
        // include extra for the buffer header
        alloc_size += sizeof(pvec_t);
        // make sure size is cache aligned
        constexpr int alignment = VCACHE_STRIPE_WORDS*sizeof(int);
        int rem = alloc_size % alignment;
        if (rem != 0)
            alloc_size += (alignment-rem);
        // allocate
        pv = (pvec_t*)spmm_malloc(alloc_size);
        pv->size = bucket_to_size(bkt);
        pv->free.next = nullptr;
        pr_dbg("%p allocd from heap: size = %d\n"
               ,&pv->v[0]
               ,pv->size
            );
        return &pv->v[0];
    }
}

/**
 * free a buffer of partials
 */
static void free_pvec(spmm_partial_t *p)
{
    pvec_t *pv = pvec_from_partial(p);
    int bkt = bucket(pv->size);
    pr_dbg("%p freeing to pool %d: size = %d\n"
           ,p
           ,bkt
           ,pv->size
        );
    list_append(&free_buffers[bkt], &pv->free);
}


/**
 * used to keep a dynamically sized array of partials
 */
typedef struct partial_buffer {
    int size;
    spmm_partial_t *partials;
} partial_buffer_t;

/**
 * initialize a partial buffer to hold at least max_size partials
 */
static void partial_buffer_init(partial_buffer_t *pb, int max_size)
{
    if (max_size > 0) {
        pb->partials = new_pvec(max_size);
    } else {
        pb->partials = nullptr;
    }
    pr_dbg("initializing partial_buffer %p for max_size = %d, partials = %p\n"
           ,pb
           ,max_size
           ,pb->partials
        );
    pb->size = 0;
}
/**
 * performs cleanup for a partial buffer
 */
static void partial_buffer_exit(partial_buffer_t *pb)
{
    pr_dbg("cleaning up partial_buffer %p, size = %d, partials = %p\n"
           ,pb
           ,pb->size
           ,pb->partials
        );
    pb->size = 0;
    if (pb->partials != nullptr) {
        free_pvec(pb->partials);
        pb->partials = nullptr;
    }
}

/**
 * move a partial buffer
 */
static void partial_buffer_move(partial_buffer_t *to, partial_buffer_t *from)
{
    partial_buffer_exit(to);
    to->size = from->size;
    to->partials = from->partials;
    from->size = 0;
    from->partials = nullptr;
    partial_buffer_exit(from);
}

/**
 * merge two partial buffers
 */
static void partial_buffers_merge(
    partial_buffer_t  *first
    ,partial_buffer_t *second
    ,partial_buffer_t *merged_o
    ) {
    partial_buffer_t merged;
    partial_buffer_init(&merged, first->size + second->size);
    pr_dbg("merging {%2d @ %p} and {%2d @ %p} into {%2d @ %p}\n"
           ,first->size
           ,first->partials
           ,second->size
           ,second->partials
           ,merged.size
           ,merged.partials
        );
    int first_i = 0, second_i = 0, merged_i = 0;
    while (first_i < first->size
           && second_i < second->size) {
        spmm_partial_t *fp = &first->partials[first_i];
        spmm_partial_t *sp = &second->partials[second_i];
        spmm_partial_t *mp = &merged.partials[merged_i];
        if (fp->idx < sp->idx) {
            *mp = *fp;
            first_i++;
        } else if (sp->idx < fp->idx) {
            *mp = *sp;
            second_i++;
        } else {
            spmm_partial_t tmp;
            tmp.idx = fp->idx;
            tmp.val = fp->val + sp->val;
            *mp = tmp;
            first_i++;
            second_i++;
        }
        merged_i++;
    }
    // finish merging first
    for (;first_i < first->size; first_i++) {
        merged.partials[merged_i++] = first->partials[first_i];
    }
    // finish merging second
    for (;second_i < second->size; second_i++) {
        merged.partials[merged_i++] = second->partials[second_i];
    }
    // set the size
    merged.size = merged_i;
    // clear first + second
    partial_buffer_exit(first);
    partial_buffer_exit(second);
    // move merged -> merged_o
    partial_buffer_move(merged_o, &merged);
}

}

static vmerge::partial_buffer_t Cv;

/**
 * perform Aik * B[k;] and update C[i;]
 */
static void scalar_row_product(
    float Aij
    ,int Bi
    ) {
    using namespace vmerge;
    // fetch row data
    int off = B_lcl.mnr_off_remote_ptr[Bi];
    int nnz = B_lcl.mnr_off_remote_ptr[Bi+1] - off;

    pr_dbg("reading %d nonzeros from B[%d]\n"
           ,nnz
           ,Bi
        );
    // initialize a partial buffer
    partial_buffer_t Bv;
    partial_buffer_init(&Bv, nnz);
    Bv.size = nnz;

    // stall on off
    kernel_remote_int_ptr_t cols = &B_lcl.mnr_idx_remote_ptr[off];
    kernel_remote_float_ptr_t vals = &B_lcl.val_remote_ptr[off];

    int nz = 0;
#if defined(SPMM_PREFETCH)
    for (; nz + PREFETCH < nnz; nz += PREFETCH) {
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
            Cij[pre]   = Aij * Bij[pre];
        }
        // write into temp buffer
        bsg_unroll(8)
        for (int pre = 0; pre < PREFETCH; pre++) {
            Bv.partials[nz+pre].idx =  Bj[pre];
            Bv.partials[nz+pre].val = Cij[pre];
        }
    }
#endif

    // strip mining code
    for (; nz < nnz; nz++) {
        float Bij, Cij;
        int   Bj;
        Bij = vals[nz];
        Bj  = cols[nz];
        Cij = Aij * Bij;
        Bv.partials[nz].idx =  Bj;
        Bv.partials[nz].val = Cij;
    }
    // merge results
    partial_buffers_merge(&Cv, &Bv, &Cv);
}

static void spmm_solve_row(
    int Ci
    ,int Ci_off
    ,int Ci_nnz
    ) {
    using namespace vmerge;
    // fetch row meta data
    int off = Ci_off;
    int nnz = Ci_nnz;

    // this will stall on 'off'
    kernel_remote_int_ptr_t cols = &A_lcl.mnr_idx_remote_ptr[off];
    kernel_remote_float_ptr_t vals = &A_lcl.val_remote_ptr[off];
    pr_dbg("solving row %d\n", Ci);
    partial_buffer_init(&Cv, 0);
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

    pr_dbg("solved row %d: writing back\n", Ci);
    if (Cv.size != 0) {
        // update the global number of nonzeros
        std::atomic<int>* nnzp = reinterpret_cast<std::atomic<int> *>((&C_glbl_p->n_non_zeros));
        nnzp->fetch_add(Cv.size);

        // write back
        if (utils::is_dram(Cv.partials)) {
            // if row data is already in dram
            // just store the allocated pointer back
            partial_buffer_t Cv_prime;
            partial_buffer_init(&Cv_prime, 0);
            partial_buffer_move(&Cv_prime, &Cv);

            C_lcl.mjr_nnz_remote_ptr[Ci]  = Cv_prime.size;
            C_lcl.alg_priv_remote_ptr[Ci] = reinterpret_cast<intptr_t>(Cv_prime.partials);
        } else {
            // allocate a save buffer
            int alloc_size = Cv.size * sizeof(spmm_partial_t);
            // make sure size is cache aligned
            constexpr int alignment = VCACHE_STRIPE_WORDS*sizeof(int);
            int rem = alloc_size % alignment;
            if (rem != 0)
                alloc_size += (alignment-rem);
            spmm_partial_t *save_buffer = (spmm_partial_t*)spmm_malloc(alloc_size);

            // write back data
            for (int i = 0; i < Cv.size; i++) {
                save_buffer[i] = Cv.partials[i];
            }

            // store row data
            C_lcl.mjr_nnz_remote_ptr[Ci] = Cv.size;
            C_lcl.alg_priv_remote_ptr[Ci] = reinterpret_cast<intptr_t>(save_buffer);
        }
    } else {
        C_lcl.mjr_nnz_remote_ptr[Ci] = 0;
    }
    // uninitialize
    partial_buffer_exit(&Cv);
}

namespace vmerge {
static pvec_static<100>  __freepool100 [3];
}

static inline void spmm_solve_row_init()
{
    using namespace vmerge;
    // initialize all lists
    for (int i = 0; i < FREE_N; i++) {
        list_init(&free_buffers[i]);
    }
    // add small sized buffers to free list
    for (int i = 0; i < ARRAY_SIZE(__freepool100); i++) {
        pvec_t *pv = &__freepool100[i].pv;
        pv->size = 100;
        list_append(&free_buffers[FREE_100], &pv->free);
    }
}

static inline void spmm_solve_row_exit()
{
}
