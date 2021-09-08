#include "bsg_manycore.h"
#include "bsg_tile_config_vars.h"
#define BSG_TILE_GROUP_X_DIM bsg_tiles_X
#define BSG_TILE_GROUP_Y_DIM bsg_tiles_Y
#include "bsg_mcs_mutex.h"
#include "bsg_tile_group_barrier.h"
#include "bsg_manycore_atomic.h"
#include <algorithm>

#ifndef CRL
#error "define CRL"
#endif

#ifndef NCRL
#error "define NCRL"
#endif

template <int N>
static inline void nop_loop(void)
{
    for (int i = 0; i < N; i++)
        asm volatile ("nop");
}

extern "C" void critical_region()
{
    nop_loop<CRL>();
}

extern "C" void noncritical_region()
{
    nop_loop<NCRL>();
}

INIT_TILE_GROUP_BARRIER(rbar, cbar, 0, bsg_tiles_X-1, 0, bsg_tiles_Y-1);

// global lock
__attribute__((section(".dram")))
bsg_mcs_mutex_t mtx;

// local lock nodes
bsg_mcs_mutex_node_t lcl;
bsg_mcs_mutex_node_t *lcl_as_glbl;

extern "C" int test_mcs_mutex()
{
    lcl_as_glbl = (bsg_mcs_mutex_node_t*)bsg_tile_group_remote_ptr(int, __bsg_x, __bsg_y, &lcl);
    bsg_tile_group_barrier(&rbar, &cbar);
    bsg_cuda_print_stat_kernel_start();
    for (int i = 0; i < ITERS; i++) {
        noncritical_region();
        bsg_mcs_mutex_acquire(&mtx, &lcl, lcl_as_glbl);
        critical_region();
        bsg_mcs_mutex_release(&mtx, &lcl, lcl_as_glbl);
    }
    bsg_cuda_print_stat_kernel_end();
    return 0;
}

extern "C" void test_spin_lock_asm(int *lockptr);

__attribute__((section(".dram")))
int locked = 0;

#define MAX_WAIT 256

static
void simple_mutex_acquire()
{
    int v;
    int wait = 1;
    while (1) {
        // acquire lock on (v = 0)
        v = bsg_amoswap_aq(&locked, 1);        
        if (v == 0)
            break;
        // wait some time
        for (volatile int w = 0; w < MAX_WAIT; w++); 
        // calculate next wait time
        wait = std::max(wait << 1, MAX_WAIT);       
    }
}

static
void simple_mutex_release()
{
    bsg_fence();
    locked = 0;
}

extern "C" int test_simple_mutex()
{
    bsg_tile_group_barrier(&rbar, &cbar);
    bsg_cuda_print_stat_kernel_start();
    for (int i = 0; i < ITERS; i++) {
        noncritical_region();
        simple_mutex_acquire();
        critical_region();
        simple_mutex_release();
    }
    bsg_cuda_print_stat_kernel_end();
    return 0;
}
