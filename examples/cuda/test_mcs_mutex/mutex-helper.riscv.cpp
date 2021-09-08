#include "bsg_manycore.h"
#include "bsg_tile_config_vars.h"
#include "bsg_mcs_mutex.h"
#include <atomic>

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

// global lock
__attribute__((section(".dram")))
bsg_mcs_mutex_t mtx = 0;

// local lock nodes
bsg_mcs_mutex_node_t lcl;
bsg_mcs_mutex_node_t *lcl_as_glbl;

extern "C" void test_mutex_asm(bsg_mcs_mutex_t *mtx
                               , bsg_mcs_mutex_node_t *lcl
                               , bsg_mcs_mutex_node_t *lcl_as_glbl);

extern "C" int test_mcs_mutex()
{
    int px = 0;
    int py = 0;
    lcl_as_glbl = (bsg_mcs_mutex_node_t*)bsg_global_pod_ptr(px, py, __bsg_x, __bsg_y, &lcl);
    bsg_cuda_print_stat_kernel_start();
    test_mutex_asm(&mtx, &lcl, lcl_as_glbl);
    bsg_cuda_print_stat_kernel_end();
    return 0;
}

extern "C" void test_spin_lock_asm(int *lockptr);

__attribute__((section(".dram")))
int locked = 0;

extern "C" int test_spin_mutex()
{
    bsg_cuda_print_stat_kernel_start();
    test_spin_lock_asm(&locked);
    bsg_cuda_print_stat_kernel_end();
    return 0;
}
