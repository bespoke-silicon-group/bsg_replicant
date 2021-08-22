#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __KERNEL__
    typedef void*  kernel_void_ptr_t;    
    typedef float* kernel_float_ptr_t;
    typedef int*   kernel_int_ptr_t;
#else
    typedef hb_mc_eva_t kernel_void_ptr_t;
    typedef hb_mc_eva_t kernel_float_ptr_t;
    typedef hb_mc_eva_t kernel_int_ptr_t;
#endif
    
#ifdef __cplusplus
}
#endif
