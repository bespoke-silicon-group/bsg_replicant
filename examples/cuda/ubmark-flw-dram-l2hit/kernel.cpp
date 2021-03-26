//This kernel adds 2 vectors 

#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_tile_group_barrier.hpp>

bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;

extern "C" __attribute__ ((noinline))
int kernel_ubmark_flw_dram_hit(float *A, float *B, float *C, int N) {
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
  // ^^^^ doing so will warmup the cache
  //------------------------------------
  bsg_saif_start();
  //------------------------------------
  asm volatile("flw     %0,     0(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,     4(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,     8(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    12(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    16(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    20(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    24(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    28(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    32(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    36(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    40(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    44(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    48(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    52(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    56(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    60(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    64(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    68(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    72(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    76(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    80(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    84(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    88(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    92(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    96(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   100(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   104(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   108(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   112(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   116(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   120(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   124(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,     0(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,     4(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,     8(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    12(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    16(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    20(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    24(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    28(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    32(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    36(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    40(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    44(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    48(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    52(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    56(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    60(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    64(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    68(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    72(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    76(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    80(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    84(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    88(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    92(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    96(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   100(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   104(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   108(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   112(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   116(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   120(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   124(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,     0(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,     4(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,     8(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    12(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    16(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    20(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    24(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    28(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    32(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    36(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    40(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    44(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    48(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    52(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    56(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    60(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    64(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    68(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    72(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    76(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    80(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    84(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    88(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    92(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    96(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   100(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   104(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   108(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   112(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   116(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   120(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   124(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,     0(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,     4(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,     8(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    12(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    16(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    20(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    24(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    28(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    32(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    36(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    40(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    44(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    48(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    52(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    56(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    60(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    64(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    68(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    72(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    76(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    80(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    84(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    88(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    92(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,    96(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   100(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   104(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   108(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   112(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   116(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   120(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  asm volatile("flw     %0,   124(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  //------------------------------------
  bsg_saif_end();
  //------------------------------------
  C[0] = B_tmp;
	barrier.sync();

	return 0;
}
