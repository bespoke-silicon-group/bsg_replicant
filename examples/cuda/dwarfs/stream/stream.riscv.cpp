#include "bsg_manycore.h"
#include "bsg_manycore_arch.h"
#include "bsg_tile_config_vars.h"

#define BSG_TILE_GROUP_X_DIM TILE_GROUP_DIM_X
#define BSG_TILE_GROUP_Y_DIM TILE_GROUP_DIM_Y
#include "bsg_mcs_mutex.hpp"

//#define DEBUG
#ifndef BLOCK_WORDS
#error  "Define BLOCK_WORDS"
#endif
#ifndef TABLE_WORDS
#error  "Define TABLE_WORDS"
#endif

#define GROUP_STRIDE_WORDS                      \
    (BLOCK_WORDS * TILE_GROUP_DIM_X * TILE_GROUP_DIM_Y)

#ifdef DEBUG
static __attribute__((section(".dram"))) bsg_mcs_mutex_t mtx;
#endif

typedef struct done {
    std::atomic<int> done;
    int padding [31];
} done_t;

//__attribute__((section(".dram"))) std::atomic<int> done[GROUPS];
__attribute__((section(".dram"))) done_t done[GROUPS];

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
    
    // offset in group
    int off = BLOCK_WORDS * (__bsg_x * TILE_GROUP_DIM_Y + __bsg_y);
    
    bsg_cuda_print_stat_kernel_start();
    for (int grp_block_off = __bsg_tile_group_id_x * GROUP_STRIDE_WORDS;
         grp_block_off < TABLE_WORDS;
         grp_block_off += GROUP_STRIDE_WORDS * GROUPS) {
        int base = grp_block_off + off;
        for (int i = base; i < base + BLOCK_WORDS; i++) {
#ifdef DEBUG
            bsg_mcs_mutex_acquire(&mtx, &lcl, lcl_as_glbl);
            bsg_print_hexadecimal(reinterpret_cast<unsigned>(&A[i]));
            bsg_mcs_mutex_release(&mtx, &lcl, lcl_as_glbl);
#endif
            asm volatile ("lw x0, %[addr]" :: [addr] "m" (A[i]));
        }
    }
    bsg_cuda_print_stat_kernel_end();
    
    done[__bsg_tile_group_id_x].done++;
    if (__bsg_x == 0 && __bsg_y == 0) {
        while (done[__bsg_tile_group_id_x].done != TILE_GROUP_DIM_X*TILE_GROUP_DIM_Y);
    }
    return 0;
}
