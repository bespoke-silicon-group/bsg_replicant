#include "spmm.hpp"
#include "spmm_hash_table.hpp"
#include "util.h"

namespace hash_table {
/**
 * Pool of entries allocated in DMEM.
 */
    thread spmm_elt_t local_elt_pool[SPMM_ELT_LOCAL_POOL_SIZE];

/**
 * List of all entries in the table.
 */
    thread spmm_elt_t *tbl_head;
    thread int tbl_num_entries;

/**
 * List of available free frames in local memory
 */
    thread spmm_elt_t *free_local_head;

/**
 * List of available free frames in off-chip memory
 */
    thread spmm_elt_t *free_global_head;

/**
 * Total non-zeros table
 */
#ifndef NONZEROS_TABLE_SIZE
#error "define NONZEROS_TABLE_SIZE"
#endif
    __attribute__((section(".dram"), aligned(2*bsg_global_X*VCACHE_STRIPE_WORDS*sizeof(int))))
    spmm_elt_t *nonzeros_table [bsg_global_X * bsg_global_Y * NONZEROS_TABLE_SIZE];

   /**
    * Next reallocation size, initialize one cache line.
    */
    int elts_realloc_size = (VCACHE_STRIPE_WORDS*sizeof(int))/sizeof(spmm_elt_t);

    thread hidx_t block_select;
}
