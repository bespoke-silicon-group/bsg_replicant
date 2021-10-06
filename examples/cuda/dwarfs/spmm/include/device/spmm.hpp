#pragma once
#ifndef BSG_TILE_GROUP_X_DIM
#define BSG_TILE_GROUP_X_DIM bsg_tiles_X
#endif
#ifndef BSG_TILE_GROUP_Y_DIM
#define BSG_TILE_GROUP_Y_DIM bsg_tiles_Y
#endif
#include "bsg_mcs_mutex.hpp"
#include "sparse_matrix.h"
#ifdef __BUILD_FOR_HOST__
#define thread thread_local
#else
#define thread
#endif

#ifdef __BUILD_FOR_HOST__
#define bsg_attr_remote
#else
#include "bsg_manycore.h"
#endif
#include <math.h>

//#define DEBUG
#ifdef DEBUG
#ifdef __BUILD_FOR_HOST__
#define pr_dbg(fmt, ...)
#else
extern bsg_mcs_mutex_t mtx;
#define pr_dbg(fmt, ...)                                                \
    do {                                                                \
        bsg_mcs_mutex_node_t lcl;                                       \
        bsg_mcs_mutex_node_t *lcl_as_glbl = (bsg_mcs_mutex_node_t*)bsg_tile_group_remote_ptr(int, __bsg_x, __bsg_y, &lcl); \
        bsg_mcs_mutex_acquire(&mtx, &lcl, lcl_as_glbl);                 \
        bsg_printf(fmt, ##__VA_ARGS__);                                 \
        bsg_fence();                                                    \
        bsg_mcs_mutex_release(&mtx, &lcl, lcl_as_glbl);                 \
    } while (0)
#endif
#else
#define pr_dbg(fmt, ...)
#endif

#include <atomic>
#include <cstdint>

#define THREADS                                 \
    (bsg_tiles_X*bsg_tiles_Y)

#define LOG2F(x)                                \
    (logf(x)/log(2))

extern thread sparse_matrix_t A_lcl;
extern thread sparse_matrix_t B_lcl;
extern thread sparse_matrix_t C_lcl;

extern thread sparse_matrix_t *A_glbl_p;
extern thread sparse_matrix_t *B_glbl_p;
extern thread sparse_matrix_t *C_glbl_p;

typedef struct spmm_partial {
    int   idx; // column index
    float val; // real value
} spmm_partial_t;

template <typename T>
static bsg_attr_remote T* to_remote_ptr(T*ptr)
{
    union {
        T *vanilla;
        bsg_attr_remote T*remote;
    } u;
    u.vanilla = ptr;
    return u.remote;
}

template <typename T>
static T* from_remote_ptr(bsg_attr_remote T*rmt)
{
    union {
        T *vanilla;
        bsg_attr_remote T*remote;
    } u;
    u.remote = rmt;
    return u.vanilla;    
}


extern std::atomic<intptr_t> *spmm_mem_pool;
static inline void *spmm_malloc(std::size_t size)
{
    return reinterpret_cast<void*>(spmm_mem_pool->fetch_add(size, std::memory_order_acquire));
}

/**
 * Allocate some memory
 */
void *spmm_malloc(std::size_t size);

/**
 * Thread barrier
 */
void  spmm_barrier();

/**
 * Initialize global state
 */
void spmm_init(sparse_matrix_t *__restrict A_ptr, // csr
               sparse_matrix_t *__restrict B_ptr, // csr
               sparse_matrix_t *__restrict C_ptr, // csr
               std::atomic<intptr_t> *mem_pool_arg); // mem pool

