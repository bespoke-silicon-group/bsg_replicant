//This kernel adds 2 vectors 

#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_tile_group_barrier.hpp>

bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;

extern "C" __attribute__ ((noinline))
int kernel_energy_spm_store_demo(float *A, float *B, float *C, int N) {
  //------------------------------------
  // load data into SPM
  float A_spm[255];
  float B_spm[255];
  float A_tmp, B_tmp, C_tmp = 0.0f;
  float* A_ptr = &A_spm[0];
  float* B_ptr = &B_spm[0];
  for(uint32_t i = 0; i < 255; i++) {
    asm volatile("flw     %0,    0(%1)"   : "+f"(A_tmp) : "r"(A)     : "memory");
    asm volatile("fsw     %0,    0(%1)"   :: "f"(A_tmp) , "r"(A_ptr) : "memory");
    asm volatile("flw     %0,    0(%1)"   : "+f"(B_tmp) : "r"(B)     : "memory");
    asm volatile("fsw     %0,    0(%1)"   :: "f"(B_tmp) , "r"(B_ptr) : "memory");
    A++;
    B++;
    A_ptr++;
    B_ptr++;
  }
  A_ptr = &A_spm[0];
  B_ptr = &B_spm[0];
  A_tmp = A_spm[254];
  B_tmp = B_spm[254];
  //------------------------------------
  bsg_saif_start();
  //------------------------------------
  asm volatile("fsw     %0,    0(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,    0(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,    4(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,    4(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,    8(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,    8(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   12(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   12(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   16(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   16(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   20(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   20(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   24(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   24(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   28(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   28(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   32(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   32(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   36(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   36(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   40(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   40(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   44(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   44(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   48(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   48(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   52(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   52(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   56(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   56(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   60(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   60(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   64(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   64(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   68(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   68(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   72(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   72(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   76(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   76(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   80(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   80(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   84(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   84(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   88(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   88(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   92(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   92(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,   96(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,   96(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  100(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  100(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  104(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  104(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  108(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  108(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  112(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  112(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  116(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  116(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  120(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  120(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  124(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  124(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  128(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  128(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  132(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  132(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  136(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  136(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  140(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  140(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  144(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  144(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  148(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  148(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  152(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  152(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  156(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  156(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  160(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  160(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  164(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  164(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  168(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  168(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  172(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  172(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  176(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  176(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  180(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  180(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  184(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  184(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  188(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  188(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  192(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  192(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  196(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  196(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  200(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  200(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  204(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  204(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  208(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  208(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  212(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  212(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  216(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  216(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  220(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  220(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  224(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  224(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  228(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  228(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  232(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  232(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  236(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  236(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  240(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  240(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  244(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  244(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  248(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  248(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  252(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  252(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  256(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  256(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  260(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  260(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  264(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  264(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  268(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  268(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  272(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  272(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  276(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  276(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  280(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  280(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  284(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  284(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  288(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  288(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  292(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  292(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  296(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  296(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  300(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  300(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  304(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  304(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  308(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  308(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  312(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  312(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  316(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  316(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  320(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  320(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  324(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  324(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  328(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  328(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  332(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  332(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  336(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  336(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  340(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  340(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  344(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  344(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  348(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  348(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  352(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  352(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  356(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  356(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  360(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  360(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  364(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  364(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  368(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  368(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  372(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  372(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  376(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  376(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  380(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  380(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  384(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  384(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  388(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  388(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  392(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  392(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  396(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  396(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  400(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  400(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  404(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  404(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  408(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  408(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  412(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  412(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  416(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  416(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  420(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  420(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  424(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  424(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  428(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  428(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  432(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  432(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  436(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  436(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  440(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  440(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  444(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  444(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  448(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  448(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  452(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  452(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  456(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  456(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  460(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  460(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  464(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  464(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  468(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  468(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  472(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  472(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  476(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  476(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  480(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  480(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  484(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  484(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  488(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  488(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  492(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  492(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  496(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  496(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  500(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  500(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  504(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  504(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  508(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  508(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  512(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  512(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  516(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  516(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  520(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  520(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  524(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  524(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  528(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  528(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  532(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  532(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  536(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  536(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  540(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  540(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  544(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  544(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  548(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  548(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  552(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  552(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  556(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  556(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  560(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  560(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  564(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  564(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  568(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  568(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  572(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  572(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  576(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  576(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  580(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  580(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  584(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  584(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  588(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  588(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  592(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  592(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  596(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  596(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  600(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  600(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  604(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  604(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  608(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  608(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  612(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  612(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  616(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  616(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  620(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  620(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  624(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  624(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  628(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  628(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  632(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  632(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  636(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  636(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  640(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  640(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  644(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  644(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  648(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  648(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  652(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  652(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  656(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  656(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  660(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  660(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  664(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  664(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  668(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  668(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  672(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  672(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  676(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  676(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  680(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  680(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  684(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  684(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  688(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  688(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  692(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  692(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  696(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  696(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  700(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  700(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  704(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  704(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  708(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  708(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  712(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  712(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  716(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  716(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  720(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  720(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  724(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  724(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  728(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  728(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  732(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  732(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  736(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  736(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  740(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  740(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  744(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  744(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  748(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  748(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  752(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  752(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  756(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  756(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  760(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  760(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  764(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  764(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  768(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  768(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  772(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  772(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  776(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  776(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  780(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  780(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  784(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  784(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  788(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  788(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  792(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  792(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  796(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  796(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  800(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  800(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  804(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  804(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  808(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  808(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  812(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  812(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  816(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  816(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  820(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  820(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  824(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  824(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  828(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  828(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  832(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  832(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  836(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  836(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  840(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  840(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  844(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  844(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  848(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  848(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  852(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  852(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  856(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  856(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  860(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  860(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  864(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  864(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  868(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  868(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  872(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  872(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  876(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  876(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  880(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  880(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  884(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  884(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  888(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  888(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  892(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  892(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  896(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  896(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  900(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  900(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  904(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  904(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  908(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  908(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  912(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  912(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  916(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  916(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  920(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  920(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  924(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  924(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  928(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  928(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  932(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  932(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  936(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  936(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  940(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  940(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  944(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  944(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  948(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  948(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  952(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  952(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  956(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  956(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  960(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  960(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  964(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  964(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  968(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  968(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  972(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  972(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  976(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  976(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  980(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  980(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  984(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  984(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  988(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  988(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  992(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  992(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0,  996(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0,  996(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0, 1000(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0, 1000(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0, 1004(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0, 1004(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0, 1008(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0, 1008(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0, 1012(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0, 1012(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  asm volatile("fsw     %0, 1016(%1)"   :: "f"(A_tmp), "r"(A_ptr) : "memory");
  asm volatile("fsw     %0, 1016(%1)"   :: "f"(B_tmp), "r"(B_ptr) : "memory");
  
  //------------------------------------
  bsg_saif_end();
  //------------------------------------
  C[0] = B_tmp;
	barrier.sync();

	return 0;
}
