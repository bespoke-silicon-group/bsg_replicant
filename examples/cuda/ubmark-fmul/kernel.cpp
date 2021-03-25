//This kernel adds 2 vectors 

#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_tile_group_barrier.hpp>

bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;

extern "C" __attribute__ ((noinline))
int kernel_ubmark_fmul(float *A, float *B, float *C, int N) {
  //------------------------------------
  // load data into SPM
  float A_spm[255];
  float B_spm[255];
  float A_tmp, B_tmp, C_tmp = 0.0f;
  float* A_ptr = &A_spm[0];
  float* B_ptr = &B_spm[0];
  float* A_base = A;
  float* B_base = B;
  for(uint32_t i = 0; i < 255; i++) {
    asm volatile("flw     %0,    0(%1)"   : "+f"(A_tmp) : "r"(A_base)     : "memory");
    asm volatile("fsw     %0,    0(%1)"   :: "f"(A_tmp) , "r"(A_ptr) : "memory");
    asm volatile("flw     %0,    0(%1)"   : "+f"(B_tmp) : "r"(B_base)     : "memory");
    asm volatile("fsw     %0,    0(%1)"   :: "f"(B_tmp) , "r"(B_ptr) : "memory");
    A_base++;
    B_base++;
    A_ptr++;
    B_ptr++;
  }
  A_ptr = &A_spm[0];
  B_ptr = &B_spm[0];
  //------------------------------------
  bsg_saif_start();
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  asm volatile("fmul.s  %0, %1, %2"     : "+f"(C_tmp) : "f"(A_tmp),  "f"(B_tmp));
  //------------------------------------
  bsg_saif_end();
  //------------------------------------
  C[0] = B_tmp;
	barrier.sync();

	return 0;
}
