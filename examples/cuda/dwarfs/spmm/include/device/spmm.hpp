#pragma once
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

//#define DEBUG
#ifdef DEBUG
#ifdef __BUILD_FOR_HOST__
#define pr_dbg(fmt, ...)
#else
#define pr_dbg(fmt, ...)                        \
    do { bsg_printf(fmt, ##__VA_ARGS__); } while (0)
#endif
#else
#define pr_dbg(fmt, ...)
#endif

#include <atomic>
#include <cstdint>

#define THREADS                                 \
    (bsg_tiles_X*bsg_tiles_Y)

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
