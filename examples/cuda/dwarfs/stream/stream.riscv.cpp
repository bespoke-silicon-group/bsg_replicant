#include "bsg_manycore.h"
#include "bsg_manycore_arch.h"
#include "bsg_tile_config_vars.h"

#define BSG_TILE_GROUP_X_DIM TILE_GROUP_DIM_X
#define BSG_TILE_GROUP_Y_DIM TILE_GROUP_DIM_Y
#include "bsg_tile_group_barrier.h"
#include "bsg_mcs_mutex.hpp"

#define GROUPS \
    ((bsg_global_X * bsg_global_Y)/(bsg_global_Y/2))

#define GROUP_BLOCK_WORDS \
    (VCACHE_STRIPE_WORDS)

#define GROUP_STRIDE_WORDS \
    (GROUP_BLOCK_WORDS * GROUPS)

#define GROUP_THREADS                           \
    (bsg_global_Y/2) // four on a 16x8 pod

#define THREAD_BLOCK_WORDS                      \
    ((GROUP_BLOCK_WORDS)/(GROUP_THREADS))


// this is the number of threads per group that should
// actually participate in the stream
// non-particpant threads have their data skipped
#ifndef THREADS_PER_GROUP
#error "Define THREAD_PER_GROUP"
#endif

// this is the number of words each thread should stream
// per block (per array)
// when WORDS_PER_THREAD < THREAD_BLOCK_WORDS, some
// data words are skipped
#ifndef BLOCK_WORDS_PER_THREAD
#error "Define BLOCK_WORDS_PER_THREAD"
#endif

// size of the tables to stream from
#ifndef TABLE_WORDS
#error  "Define TABLE_WORDS"
#endif

//#define DEBUG
#ifdef DEBUG
static __attribute__((section(".dram"))) bsg_mcs_mutex_t mtx;
#endif

INIT_TILE_GROUP_BARRIER(rbar, cbar, 0, bsg_tiles_X-1, 0, bsg_tiles_Y-1);

template <int WORDS>
int read_block(bsg_attr_remote int *__restrict A)
{
    int sum = 0;
    for (int i = 0; i < WORDS; i++)
        sum += A[i];
    return sum;
}

extern "C" int read(bsg_attr_remote int *__restrict A)
{
#ifdef DEBUG    
    int global_x = __bsg_grp_org_x + __bsg_x - 16;
    int global_y = __bsg_grp_org_y + __bsg_y - 8;    
    bsg_mcs_mutex_node_t lcl, *lcl_as_glbl =
        (bsg_mcs_mutex_node_t*)bsg_global_pod_ptr(0, 0, global_x, global_y, &lcl);

    bsg_mcs_mutex_acquire(&mtx, &lcl, lcl_as_glbl);
    bsg_print_int(__bsg_x);
    bsg_print_int(__bsg_y);
    bsg_print_int(__bsg_tile_group_id_x);
    bsg_print_int(global_x);
    bsg_print_int(global_y);
    bsg_mcs_mutex_release(&mtx, &lcl, lcl_as_glbl);
#endif
    // calculate
    // (1) which vcache your group streams from
    // (2) which words from your groups block you stream from
    int south_not_north = __bsg_y >= (bsg_global_Y/2);
    int group_y = __bsg_y % (bsg_global_Y/2);
    int group = __bsg_x + south_not_north * bsg_global_X;    

    // offset in group
    int off = THREAD_BLOCK_WORDS * group_y;

    // check if we should participate in stream
    int participate
        = south_not_north
        ? group_y >= (GROUP_THREADS-THREADS_PER_GROUP)
        : group_y < THREADS_PER_GROUP;

    bsg_tile_group_barrier(&rbar, &cbar);
    bsg_cuda_print_stat_kernel_start();
    int sum = 0;            
    if (participate) {
        bsg_unroll(16/BLOCK_WORDS_PER_THREAD)
        for (int grp_block_off = group * GROUP_BLOCK_WORDS;
             grp_block_off < TABLE_WORDS;
             grp_block_off += GROUP_STRIDE_WORDS) {
            int base = grp_block_off + off;
            sum += read_block<BLOCK_WORDS_PER_THREAD>(&A[base]);
        }
        //asm volatile ("fence" ::: "memory");
    }
    bsg_tile_group_barrier(&rbar, &cbar);    
    bsg_cuda_print_stat_kernel_end();
    return sum;
}


// extern "C" int read_no_hits(int *A)
// {
//     // calculate
//     // (1) which vcache your group streams from
//     // (2) which words from your groups block you stream from
//     int south_not_north = __bsg_y >= (bsg_global_Y/2);
//     int group_y = __bsg_y % (bsg_global_Y/2);
//     int group = __bsg_x + south_not_north * bsg_global_X;    

//     // offset in group
//     int off = THREAD_BLOCK_WORDS * group_y;

//     int do_reads
//         = south_not_north
//         ? (group_y == (bsg_global_Y/2)-1)
//         : (group_y == 0);
    
//     bsg_tile_group_barrier(&rbar, &cbar);
//     bsg_cuda_print_stat_kernel_start();
//     if (do_reads) {
//         for (int grp_block_off = group * GROUP_BLOCK_WORDS;
//              grp_block_off < TABLE_WORDS;
//              grp_block_off += GROUP_STRIDE_WORDS) {
//             int base = grp_block_off + off;
//             asm volatile ("lw x0, %[addr]" :: [addr] "m" (A[base]));
//         }
//         asm volatile ("fence" ::: "memory");        
//     }
//     bsg_tile_group_barrier(&rbar, &cbar);    
//     bsg_cuda_print_stat_kernel_end();
//     return 0;
// }
