//This kernel adds 2 vectors 

#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_tile_group_barrier.hpp>

bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;

extern "C" __attribute__ ((noinline))
int kernel_ubmark_fmadd(float *A, float *B, float *C, int N) {
  //------------------------------------
  // load data into SPM
  float A_spm[255];
  float B_spm[255];
  float A_tmp, B_tmp;
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
  float tmp0 = A_spm[0];
  float tmp1 = A_spm[1];
  float tmp2 = A_spm[2];
  float tmp3 = A_spm[3];
  float tmp4 = A_spm[4];
  float tmp5 = A_spm[5];
  float tmp6 = A_spm[6];
  float tmp7 = A_spm[7];
  float tmp8 = A_spm[8];
  float tmp9 = A_spm[9];
  //------------------------------------
  bsg_saif_start();
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp5) : "f"(tmp6),  "f"(tmp7), "f"(tmp8));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp6) : "f"(tmp7),  "f"(tmp8), "f"(tmp9));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp7) : "f"(tmp8),  "f"(tmp9), "f"(tmp0));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp8) : "f"(tmp9),  "f"(tmp0), "f"(tmp1));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp9) : "f"(tmp0),  "f"(tmp1), "f"(tmp2));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp0) : "f"(tmp1),  "f"(tmp2), "f"(tmp3));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp1) : "f"(tmp2),  "f"(tmp3), "f"(tmp4));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp2) : "f"(tmp3),  "f"(tmp4), "f"(tmp5));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp3) : "f"(tmp4),  "f"(tmp5), "f"(tmp6));
  //------------------------------------
  asm volatile("fmadd.s %0, %1, %2, %3" : "=f"(tmp4) : "f"(tmp5),  "f"(tmp6), "f"(tmp7));
  //------------------------------------
  bsg_saif_end();
  //------------------------------------
  C[0] = B_tmp;
	barrier.sync();

	return 0;
}
