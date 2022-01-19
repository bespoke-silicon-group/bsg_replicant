#pragma once
#include "spmm_solve_row_common.hpp"
#include "util.h"

namespace hash_table {
/* hash table entry */
    typedef struct spmm_elt {
        spmm_partial_t part; //!< partial
        union {            
            spmm_elt    *bkt_next; //!< next in bucket
            spmm_elt    *tbl_prev; //!< next in bucket            
        };
        spmm_elt    *tbl_next; //!< next in table
    } spmm_elt_t;

#ifndef SPMM_SOLVE_ROW_LOCAL_DATA_WORDS
#error "define SPMM_SOLVE_ROW_LOCAL_DATA_WORDS"
#endif

#define SPMM_ELT_LOCAL_POOL_SIZE                                        \
    (SPMM_SOLVE_ROW_LOCAL_DATA_WORDS*sizeof(int)/sizeof(spmm_elt_t))
/**
 * Pool of entries allocated in DMEM.
 */
    extern thread spmm_elt_t local_elt_pool[SPMM_ELT_LOCAL_POOL_SIZE];

/**
 * List of all entries in the table.
 */
    extern thread spmm_elt_t *tbl_head;
    extern thread int tbl_num_entries;

/**
 * List of available free frames in local memory
 */
    extern thread spmm_elt_t *free_local_head;

/**
 * List of available free frames in off-chip memory
 */
    extern thread spmm_elt_t *free_global_head;

/**
 * Total non-zeros table
 */
#ifndef NONZEROS_TABLE_SIZE
#error "define NONZEROS_TABLE_SIZE"
#endif
    extern spmm_elt_t *nonzeros_table [bsg_global_X * bsg_global_Y * NONZEROS_TABLE_SIZE];

    /* type of for hash index */
    typedef unsigned hidx_t;

    extern thread hidx_t block_select;

#ifndef LOG2_VCACHE_STRIPE_WORDS
#error "Define LOG2_VCACHE_STRIPE_WORDS"
#endif

#ifndef LOG2_GLOBAL_X
#error "Define LOG2_GLOBAL_X"
#endif

#ifndef LOG2_GLOBAL_Y
#error "Define LOG2_GLOBAL_Y"
#endif

#if defined(ALIGNED_TABLE)
/**
 * x cord shift
 */
#define X_SHIFT                                 \
    (LOG2_VCACHE_STRIPE_WORDS)
/**
 * south-not-north bit shift
 */
#define SOUTH_NOT_NORTH_SHIFT                   \
    ((X_SHIFT)+(LOG2_GLOBAL_X))
/**
 * y cord shift (selects set in vcache)
 */
#define Y_SHIFT                                 \
    ((SOUTH_NOT_NORTH_SHIFT)+1)
/**
 * high bits shift
 */
#define HI_SHIFT                                \
    ((Y_SHIFT)+(LOG2_GLOBAL_Y-1))
#endif

    /**
     * Do some initialization for the hash function.
     */
    static void hash_init()
    {
#if defined(ALIGNED_TABLE)
        hidx_t tbl_x, tbl_y, south_not_north;
        tbl_x = __bsg_x;
        south_not_north = __bsg_y / (bsg_global_Y/2);
        tbl_y = __bsg_y % (bsg_global_Y/2);
        pr_dbg("init: bsg_global_X = %3u, bsg_global_Y = %3u\n"
               , bsg_global_X
               , bsg_global_Y);
        pr_dbg("init: (x=%3d,y=%3d): tbl_y = %3u, tbl_x = %3u, south_not_north = %3u\n"
               , __bsg_x
               , __bsg_y
               , tbl_y
               , tbl_x
               , south_not_north);
        block_select
            = (tbl_y << Y_SHIFT)
            | (south_not_north << SOUTH_NOT_NORTH_SHIFT)
            | (tbl_x << X_SHIFT);
#else
        block_select = __bsg_id * NONZEROS_TABLE_SIZE;
#endif
    }

    /**
     * Hash function
     */
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
        return block_select + x;
#else
        hidx_t hi = x / VCACHE_STRIPE_WORDS;
        hidx_t lo = x % VCACHE_STRIPE_WORDS;
        return (hi << HI_SHIFT) | block_select | lo;
#endif
    }


   /**
    * Next reallocation size, initialize one cache line.
    */
    extern int elts_realloc_size;

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
     * Update with v, idx, and the compute hash index hidx
     */
    static void update(float v, int idx, int hidx)
    {
            spmm_elt_t **u = &nonzeros_table[hidx];
            spmm_elt_t  *p = nonzeros_table[hidx];
            pr_dbg("  &table[%3d] = 0x%08x\n"
                          , idx
                          , u);
            pr_dbg("  table[%3d] = 0x%08x\n"
                          , idx
                          , p);
            while (p != nullptr) {
                // match?
                if (p->part.idx == idx) {
                    pr_dbg("  col %3d found at 0x%08x\n"
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
            pr_dbg("  col %3d not found, inserting at 0x%08x\n"
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

#if 0
    /**
     * Update with v, idx, and the compute hash index hidx
     */
    static void update2(float   v0
                        , int   idx0
                        , int   hidx0
                        , float v1
                        , int   idx1
                        , int   hidx1)
    {
        // assumption: hidx0 != hidx1
        // lookups can overlap
        spmm_elt_t **u0 = &nonzeros_table[hidx0];
        spmm_elt_t **u1 = &nonzeros_table[hidx1];
        spmm_elt_t  *p0 = nonzeros_table[hidx];
        spmm_elt_t  *p1 = nonzeros_table[hidx];
        int pidx0 = -1, pidx1 = -1;
        if (p0 != nullptr)
            pidx0 = p->idx;
        if (p1 != nullptr)
            pidx1 = p->idx;

        bsg_compiler_memory_barrier();

        int d0 = 0, d1 = 0;
        while (!(d0 && d1)) {
            // done scanning p0?
            if (pidx0 != idx0
                && p0 != nullptr) {
                u0 = &p0->bkt_next;
                p0 =  p0->bkt_next;
            } else {
                d0 = 1;
            }

            // done scanning p1?
            if (pidx0 != idx1
                && p1 != nullptr) {
                u1 = &p1->bkt_next;
                p1 =  p1->bkt_next;
            } else {
                d1 = 1;
            }
            // memory barrier
            bsg_compiler_memory_barrier();
            // fetch idx0?
            if (pidx0 != idx0
                && p0 != nullptr) {
                idx0 = p0->idx;
            }

            // fetch idx0?
            if (pidx1 != idx1
                && p1 != nullptr) {
                idx1 = p1->idx;
            }
        }
        // fetch floating point values
        float uv0, uv1;            
        if (p0 != nullptr)
            uv0 = p0->val;

        if (p1 != nullptr)
            uv1 = p1->val;
        
        bsg_compiler_memory_barrier();

        // update floating point values
        if (p0 != nullptr) {
#if !defined(SPMM_NO_FLOPS)
            uv0 += v0;
#else
            uv0 = v0;
#endif
            p0->val = uv0;
        }
        if (p1 != nullptr) {
#if !defined(SPMM_NO_FLOPS)
            uv1 += v1;
#else
            uv1 = v1;
#endif
            p1->val = uv1;
        }

        // insert
        if (p0 == nullptr) {
            p0 = alloc_elt();
            p0->part.idx = idx0;
            p0->part.val = v0;
            p0->bkt_next = nullptr;
            p0->tbl_next = tbl_head;
            tbl_head = p0;
            *u0 = p0;
            tbl_num_entries++;
        }
        if (p1 == nullptr) {
            p1 = alloc_elt();
            p1->part.idx = idx1;
            p1->part.val = v1;
            p1->bkt_next = nullptr;
            p1->tbl_next = tbl_head;
            tbl_head = p1;
            *u1 = p1;
            tbl_num_entries++;
        }
    }
#endif
    
    /**
     * Hash table init
     */
    static void init()
    {
        pr_dbg("init: calling from " __FILE__ "\n");
        // initialize nonzeros table in dram
        pr_dbg("init: nonzeros_table[start] = 0x%08x\n"
                      , &nonzeros_table[0]);
        pr_dbg("init: nonzeros_table[end]   = 0x%08x\n"
                      , &nonzeros_table[ARRAY_SIZE(nonzeros_table)-1]);
        hash_init();
        // initialize list of local nodes
        int i;
        if (ARRAY_SIZE(local_elt_pool) > 0) {
            free_local_head = &local_elt_pool[0];
            for (i = 0; i < ARRAY_SIZE(local_elt_pool)-1; i++) {
                local_elt_pool[i].tbl_next = &local_elt_pool[i+1];
            }
            local_elt_pool[ARRAY_SIZE(local_elt_pool)-1].tbl_next = nullptr;
            pr_dbg("init: local_elt_pool[N-1]=0x%08x\n"
                          , &local_elt_pool[ARRAY_SIZE(local_elt_pool)-1]);
        }
        pr_dbg("init: free_local_head  = 0x%08x\n", free_local_head);
        pr_dbg("init: free_global_head = 0x%08x\n", free_global_head);
    }
}
