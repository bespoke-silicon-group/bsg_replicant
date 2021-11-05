#pragma once
#ifndef BSG_TILE_GROUP_X_DIM
#define BSG_TILE_GROUP_X_DIM bsg_tiles_X
#endif
#ifndef BSG_TILE_GROUP_Y_DIM
#define BSG_TILE_GROUP_Y_DIM bsg_tiles_Y
#endif
#include "bsg_mcs_mutex.hpp"
#include "sparse_matrix.h"
#include "sparse_matrix_partition.h"
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
extern bsg_mcs_mutex_t mtx;
#ifdef DEBUG
#define pr_dbg(fmt, ...)                                                \
    do {                                                                \
        bsg_mcs_mutex_node_t lcl;                                       \
        bsg_mcs_mutex_node_t *lcl_as_glbl = (bsg_mcs_mutex_node_t*)bsg_tile_group_remote_ptr(int, __bsg_x, __bsg_y, &lcl); \
        bsg_mcs_mutex_acquire(&mtx, &lcl, lcl_as_glbl);                 \
        bsg_printf("thread %3d: " fmt, __bsg_id, ##__VA_ARGS__);                \
        bsg_fence();                                                    \
        bsg_mcs_mutex_release(&mtx, &lcl, lcl_as_glbl);                 \
    } while (0)
#else
#define pr_dbg(fmt, ...)
#endif

#ifdef PRINT_INT_DBG
#define spmm_print_int(i)                       \
    do {                                        \
        bsg_mcs_mutex_node_t lcl;                                       \
        bsg_mcs_mutex_node_t *lcl_as_glbl = (bsg_mcs_mutex_node_t*)bsg_tile_group_remote_ptr(int, __bsg_x, __bsg_y, &lcl); \
        bsg_mcs_mutex_acquire(&mtx, &lcl, lcl_as_glbl);                 \
        bsg_print_int(i);                                               \
        bsg_fence();                                                    \
        bsg_mcs_mutex_release(&mtx, &lcl, lcl_as_glbl);                 \
    }while (0)
#else
#define spmm_print_int(i)
#endif

#include <atomic>
#include <cstdint>

#define THREADS                                 \
    (bsg_tiles_X*bsg_tiles_Y)

extern thread sparse_matrix_partition_t A_part_lcl;
extern thread sparse_matrix_partition_t B_part_lcl;
extern thread sparse_matrix_partition_t C_part_lcl;

#define A_lcl                                   \
    (A_part_lcl.matrix)
#define B_lcl                                   \
    (B_part_lcl.matrix)
#define C_lcl                                   \
    (C_part_lcl.matrix)

extern thread sparse_matrix_partition_t *A_part_glbl_p;
extern thread sparse_matrix_partition_t *B_part_glbl_p;
extern thread sparse_matrix_partition_t *C_part_glbl_p;

#define A_glbl_p                                \
    (&A_part_glbl_p->matrix)
#define B_glbl_p                                \
    (&B_part_glbl_p->matrix)
#define C_glbl_p                                \
    (&C_part_glbl_p->matrix)

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

void spmm_init(sparse_matrix_partition_t *__restrict__ A_ptr, // csr
               sparse_matrix_partition_t *__restrict__ B_ptr, // csr
               sparse_matrix_partition_t *__restrict__ C_ptr, // csr
               std::atomic<intptr_t> *mem_pool_arg); // mem pool


#ifdef TAG_ROW_SOLVE
#undef TAG_ROW_SOLVE
#endif
#define TAG_ROW_SOLVE 0x1

#ifdef TAG_OFFSET_COMPUTE
#undef TAG_OFFSET_COMPUTE
#endif
#define TAG_OFFSET_COMPUTE 0x2

#ifdef TAG_RESULTS_COPY
#undef TAG_RESULTS_COPY
#endif
#define TAG_RESULTS_COPY 0x3

#ifdef TAG_ROW_SORT
#undef TAG_ROW_SORT
#endif
#define TAG_ROW_SORT 0x4
