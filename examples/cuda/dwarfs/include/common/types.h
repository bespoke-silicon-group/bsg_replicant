#pragma once
#include "bsg_manycore.h"
#ifdef __cplusplus
#include <atomic>
#endif
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __KERNEL__
    typedef void*  kernel_void_ptr_t;    
    typedef float* kernel_float_ptr_t;
    typedef int*   kernel_int_ptr_t;

    typedef bsg_attr_remote int* kernel_remote_int_ptr_t;
    typedef bsg_attr_remote float* kernel_remote_float_ptr_t;
    typedef bsg_attr_remote void* kernel_remote_void_ptr_t;
    // atomic pointers
#ifdef __cplusplus
    typedef std::atomic<int> *   kernel_aint_ptr_t;
    typedef bsg_attr_remote std::atomic<int>* kernel_remote_aint_ptr_t;
#else
    typedef kernel_int_ptr_t         kernel_aint_ptr_t;
    typedef kernelt_remote_int_ptr_t kernel_remote_aint_ptr_t;
#endif

#else
    typedef hb_mc_eva_t kernel_void_ptr_t;
    typedef hb_mc_eva_t kernel_float_ptr_t;
    typedef hb_mc_eva_t kernel_int_ptr_t;

    typedef hb_mc_eva_t kernel_remote_int_ptr_t;
    typedef hb_mc_eva_t kernel_remote_float_ptr_t;
    typedef hb_mc_eva_t kernel_remote_void_ptr_t;    
#endif
    
#ifdef __cplusplus
}
#endif
