#include "bsg_manycore.h"
#include "bsg_manycore_arch.h"
#include "bsg_tile_config_vars.h"

#define BSG_TILE_GROUP_X_DIM TILE_GROUP_DIM_X
#define BSG_TILE_GROUP_Y_DIM TILE_GROUP_DIM_Y
#include "bsg_tile_group_barrier.h"
#include "bsg_mcs_mutex.hpp"

#define GROUPS ((bsg_global_X * bsg_global_Y)/(bsg_global_Y/2))

//#define DEBUG
#ifndef BLOCK_WORDS
#error  "Define BLOCK_WORDS"
#endif
#ifndef TABLE_WORDS
#error  "Define TABLE_WORDS"
#endif

#define GROUP_STRIDE_WORDS                      \
    (BLOCK_WORDS * (bsg_global_Y/2))

#ifdef DEBUG
static __attribute__((section(".dram"))) bsg_mcs_mutex_t mtx;
#endif

INIT_TILE_GROUP_BARRIER(rbar, cbar, 0, bsg_tiles_X-1, 0, bsg_tiles_Y-1);

typedef struct done {
    std::atomic<int> done;
    int padding [VCACHE_STRIPE_WORDS-1];
} done_t;

extern "C" int read(int *A, done_t *done)
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
    int off = BLOCK_WORDS * group_y;
    
    bsg_tile_group_barrier(&rbar, &cbar);
    bsg_cuda_print_stat_kernel_start();
    for (int grp_block_off = group * GROUP_STRIDE_WORDS;
         grp_block_off < TABLE_WORDS;
         grp_block_off += GROUP_STRIDE_WORDS * GROUPS) {
        int base = grp_block_off + off;
        for (int i = base; i < base + BLOCK_WORDS; i++) {
#ifdef DEBUG
            bsg_mcs_mutex_acquire(&mtx, &lcl, lcl_as_glbl);
            bsg_print_hexadecimal(reinterpret_cast<unsigned>(&A[i]));
            bsg_fence();            
            bsg_mcs_mutex_release(&mtx, &lcl, lcl_as_glbl);
#endif
            asm volatile ("lw x0, %[addr]" :: [addr] "m" (A[i]));
        }
    }
    asm volatile ("fence" ::: "memory");    
    bsg_tile_group_barrier(&rbar, &cbar);    
    bsg_cuda_print_stat_kernel_end();
    return 0;
}


extern "C" int read_no_hits(int *A, done_t *done)
{
    // calculate
    // (1) which vcache your group streams from
    // (2) which words from your groups block you stream from
    int south_not_north = __bsg_y >= (bsg_global_Y/2);
    int group_y = __bsg_y % (bsg_global_Y/2);
    int group = __bsg_x + south_not_north * bsg_global_X;    

    // offset in group
    int off = BLOCK_WORDS * group_y;

    int do_reads
        = south_not_north
        ? (group_y == (bsg_global_Y/2)-1)
        : (group_y == 0);
    
    bsg_tile_group_barrier(&rbar, &cbar);
    bsg_cuda_print_stat_kernel_start();
    if (do_reads) {
        for (int grp_block_off = group * GROUP_STRIDE_WORDS;
             grp_block_off < TABLE_WORDS;
             grp_block_off += GROUP_STRIDE_WORDS * GROUPS) {
            int base = grp_block_off + off;
            asm volatile ("lw x0, %[addr]" :: [addr] "m" (A[base]));
        }
        asm volatile ("fence" ::: "memory");        
    }
    bsg_tile_group_barrier(&rbar, &cbar);    
    bsg_cuda_print_stat_kernel_end();
    return 0;
}
