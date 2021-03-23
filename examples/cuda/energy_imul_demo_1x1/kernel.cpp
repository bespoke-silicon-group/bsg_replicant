//This kernel adds 2 vectors 

#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_tile_group_barrier.hpp>

bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;

extern "C" __attribute__ ((noinline))
int kernel_energy_imul_demo(uint32_t *A, uint32_t *B, uint32_t *C, int N) {
  //------------------------------------
  // load data into SPM
  uint32_t A_spm[255];
  uint32_t B_spm[255];
  uint32_t A_tmp, B_tmp, C_tmp = 0.0f;
  uint32_t* A_ptr = &A_spm[0];
  uint32_t* B_ptr = &B_spm[0];
  for(uint32_t i = 0; i < 255; i++) {
    asm volatile("lw     %0,    0(%1)"   : "=r"(A_tmp) : "r"(A)     : "memory");
    asm volatile("sw     %0,    0(%1)"   :: "r"(A_tmp) , "r"(A_ptr) : "memory");
    asm volatile("lw     %0,    0(%1)"   : "=r"(B_tmp) : "r"(B)     : "memory");
    asm volatile("sw     %0,    0(%1)"   :: "r"(B_tmp) , "r"(B_ptr) : "memory");
    A++;
    B++;
    A_ptr++;
    B_ptr++;
  }
  A_ptr = &A_spm[0];
  B_ptr = &B_spm[0];
  //------------------------------------
  bsg_saif_start();
  //------------------------------------
  asm volatile("lw     %0,    0(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,    0(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,    4(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,    4(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,    8(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,    8(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   12(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   12(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   16(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   16(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   20(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   20(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   24(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   24(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   28(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   28(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   32(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   32(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   36(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   36(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   40(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   40(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   44(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   44(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   48(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   48(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   52(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   52(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   56(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   56(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   60(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   60(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   64(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   64(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   68(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   68(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   72(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   72(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   76(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   76(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   80(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   80(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   84(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   84(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   88(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   88(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   92(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   92(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,   96(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,   96(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  100(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  100(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  104(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  104(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  108(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  108(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  112(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  112(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  116(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  116(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  120(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  120(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  124(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  124(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  128(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  128(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  132(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  132(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  136(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  136(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  140(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  140(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  144(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  144(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  148(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  148(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  152(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  152(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  156(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  156(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  160(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  160(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  164(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  164(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  168(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  168(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  172(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  172(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  176(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  176(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  180(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  180(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  184(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  184(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  188(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  188(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  192(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  192(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  196(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  196(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  200(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  200(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  204(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  204(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  208(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  208(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  212(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  212(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  216(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  216(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  220(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  220(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  224(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  224(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  228(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  228(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  232(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  232(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  236(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  236(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  240(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  240(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  244(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  244(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  248(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  248(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  252(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  252(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  256(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  256(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  260(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  260(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  264(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  264(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  268(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  268(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  272(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  272(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  276(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  276(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  280(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  280(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  284(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  284(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  288(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  288(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  292(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  292(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  296(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  296(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  300(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  300(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  304(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  304(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  308(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  308(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  312(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  312(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  316(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  316(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  320(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  320(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  324(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  324(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  328(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  328(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  332(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  332(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  336(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  336(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  340(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  340(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  344(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  344(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  348(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  348(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  352(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  352(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  356(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  356(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  360(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  360(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  364(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  364(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  368(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  368(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  372(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  372(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  376(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  376(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  380(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  380(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  384(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  384(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  388(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  388(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  392(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  392(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  396(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  396(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  400(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  400(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  404(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  404(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  408(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  408(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  412(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  412(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  416(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  416(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  420(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  420(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  424(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  424(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  428(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  428(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  432(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  432(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  436(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  436(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  440(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  440(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  444(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  444(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  448(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  448(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  452(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  452(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  456(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  456(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  460(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  460(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  464(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  464(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  468(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  468(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  472(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  472(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  476(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  476(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  480(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  480(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  484(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  484(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  488(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  488(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  492(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  492(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  496(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  496(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  500(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  500(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  504(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  504(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  508(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  508(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  512(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  512(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  516(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  516(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  520(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  520(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  524(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  524(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  528(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  528(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  532(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  532(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  536(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  536(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  540(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  540(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  544(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  544(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  548(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  548(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  552(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  552(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  556(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  556(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  560(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  560(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  564(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  564(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  568(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  568(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  572(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  572(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  576(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  576(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  580(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  580(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  584(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  584(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  588(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  588(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  592(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  592(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  596(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  596(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  600(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  600(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  604(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  604(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  608(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  608(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  612(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  612(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  616(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  616(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  620(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  620(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  624(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  624(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  628(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  628(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  632(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  632(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  636(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  636(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  640(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  640(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  644(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  644(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  648(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  648(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  652(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  652(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  656(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  656(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  660(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  660(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  664(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  664(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  668(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  668(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  672(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  672(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  676(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  676(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  680(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  680(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  684(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  684(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  688(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  688(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  692(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  692(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  696(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  696(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  700(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  700(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  704(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  704(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  708(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  708(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  712(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  712(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  716(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  716(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  720(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  720(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  724(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  724(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  728(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  728(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  732(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  732(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  736(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  736(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  740(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  740(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  744(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  744(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  748(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  748(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  752(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  752(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  756(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  756(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  760(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  760(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  764(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  764(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  768(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  768(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  772(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  772(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  776(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  776(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  780(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  780(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  784(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  784(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  788(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  788(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  792(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  792(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  796(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  796(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  800(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  800(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  804(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  804(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  808(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  808(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  812(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  812(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  816(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  816(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  820(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  820(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  824(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  824(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  828(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  828(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  832(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  832(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  836(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  836(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  840(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  840(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  844(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  844(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  848(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  848(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  852(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  852(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  856(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  856(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  860(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  860(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  864(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  864(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  868(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  868(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  872(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  872(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  876(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  876(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  880(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  880(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  884(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  884(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  888(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  888(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  892(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  892(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  896(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  896(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  900(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  900(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  904(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  904(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  908(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  908(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  912(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  912(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  916(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  916(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  920(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  920(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  924(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  924(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  928(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  928(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  932(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  932(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  936(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  936(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  940(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  940(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  944(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  944(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  948(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  948(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  952(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  952(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  956(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  956(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  960(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  960(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  964(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  964(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  968(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  968(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  972(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  972(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  976(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  976(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  980(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  980(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  984(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  984(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  988(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  988(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  992(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  992(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0,  996(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0,  996(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0, 1000(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0, 1000(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0, 1004(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0, 1004(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0, 1008(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0, 1008(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0, 1012(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0, 1012(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  asm volatile("lw     %0, 1016(%1)"   : "=r"(A_tmp) : "r"(A_ptr) : "memory");
  asm volatile("lw     %0, 1016(%1)"   : "=r"(B_tmp) : "r"(B_ptr) : "memory");
  asm volatile("mul     %0, %1, %2"    : "+r"(C_tmp) : "r"(A_tmp),  "r"(B_tmp));
  //------------------------------------
  bsg_saif_end();
  //------------------------------------
  C[0] = C_tmp;
	barrier.sync();

	return 0;
}
