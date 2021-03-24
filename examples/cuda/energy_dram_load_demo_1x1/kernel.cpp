//This kernel adds 2 vectors 

#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_tile_group_barrier.hpp>

bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;

extern "C" __attribute__ ((noinline))
int kernel_energy_dram_load_demo(float *A, float *B, float *C, int N) {
  //------------------------------------
  // load data into SPM
  float A_tmp, B_tmp, C_tmp = 0.0f;
  //------------------------------------
  bsg_saif_start();
  //------------------------------------
  asm volatile("flw     %0,    0(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,    0(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,    4(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,    4(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,    8(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,    8(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   12(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   12(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   16(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   16(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   20(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   20(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   24(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   24(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   28(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   28(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   32(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   32(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   36(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   36(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   40(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   40(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   44(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   44(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   48(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   48(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   52(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   52(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   56(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   56(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   60(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   60(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   64(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   64(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   68(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   68(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   72(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   72(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   76(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   76(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   80(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   80(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   84(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   84(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   88(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   88(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   92(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   92(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,   96(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,   96(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  100(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  100(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  104(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  104(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  108(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  108(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  112(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  112(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  116(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  116(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  120(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  120(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  124(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  124(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  128(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  128(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  132(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  132(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  136(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  136(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  140(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  140(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  144(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  144(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  148(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  148(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  152(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  152(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  156(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  156(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  160(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  160(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  164(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  164(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  168(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  168(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  172(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  172(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  176(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  176(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  180(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  180(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  184(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  184(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  188(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  188(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  192(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  192(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  196(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  196(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  200(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  200(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  204(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  204(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  208(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  208(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  212(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  212(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  216(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  216(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  220(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  220(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  224(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  224(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  228(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  228(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  232(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  232(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  236(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  236(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  240(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  240(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  244(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  244(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  248(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  248(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  252(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  252(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  256(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  256(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  260(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  260(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  264(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  264(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  268(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  268(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  272(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  272(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  276(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  276(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  280(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  280(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  284(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  284(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  288(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  288(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  292(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  292(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  296(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  296(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  300(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  300(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  304(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  304(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  308(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  308(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  312(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  312(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  316(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  316(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  320(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  320(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  324(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  324(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  328(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  328(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  332(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  332(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  336(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  336(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  340(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  340(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  344(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  344(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  348(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  348(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  352(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  352(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  356(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  356(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  360(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  360(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  364(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  364(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  368(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  368(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  372(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  372(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  376(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  376(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  380(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  380(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  384(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  384(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  388(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  388(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  392(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  392(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  396(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  396(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  400(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  400(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  404(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  404(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  408(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  408(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  412(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  412(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  416(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  416(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  420(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  420(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  424(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  424(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  428(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  428(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  432(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  432(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  436(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  436(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  440(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  440(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  444(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  444(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  448(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  448(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  452(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  452(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  456(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  456(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  460(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  460(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  464(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  464(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  468(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  468(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  472(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  472(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  476(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  476(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  480(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  480(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  484(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  484(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  488(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  488(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  492(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  492(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  496(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  496(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  500(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  500(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  504(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  504(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  508(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  508(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  512(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  512(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  516(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  516(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  520(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  520(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  524(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  524(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  528(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  528(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  532(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  532(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  536(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  536(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  540(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  540(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  544(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  544(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  548(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  548(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  552(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  552(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  556(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  556(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  560(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  560(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  564(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  564(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  568(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  568(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  572(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  572(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  576(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  576(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  580(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  580(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  584(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  584(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  588(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  588(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  592(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  592(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  596(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  596(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  600(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  600(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  604(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  604(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  608(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  608(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  612(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  612(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  616(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  616(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  620(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  620(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  624(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  624(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  628(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  628(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  632(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  632(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  636(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  636(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  640(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  640(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  644(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  644(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  648(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  648(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  652(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  652(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  656(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  656(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  660(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  660(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  664(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  664(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  668(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  668(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  672(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  672(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  676(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  676(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  680(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  680(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  684(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  684(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  688(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  688(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  692(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  692(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  696(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  696(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  700(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  700(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  704(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  704(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  708(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  708(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  712(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  712(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  716(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  716(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  720(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  720(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  724(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  724(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  728(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  728(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  732(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  732(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  736(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  736(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  740(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  740(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  744(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  744(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  748(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  748(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  752(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  752(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  756(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  756(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  760(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  760(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  764(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  764(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  768(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  768(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  772(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  772(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  776(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  776(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  780(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  780(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  784(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  784(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  788(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  788(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  792(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  792(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  796(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  796(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  800(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  800(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  804(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  804(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  808(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  808(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  812(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  812(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  816(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  816(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  820(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  820(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  824(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  824(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  828(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  828(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  832(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  832(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  836(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  836(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  840(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  840(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  844(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  844(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  848(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  848(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  852(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  852(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  856(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  856(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  860(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  860(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  864(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  864(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  868(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  868(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  872(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  872(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  876(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  876(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  880(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  880(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  884(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  884(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  888(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  888(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  892(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  892(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  896(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  896(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  900(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  900(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  904(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  904(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  908(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  908(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  912(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  912(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  916(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  916(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  920(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  920(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  924(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  924(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  928(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  928(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  932(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  932(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  936(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  936(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  940(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  940(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  944(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  944(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  948(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  948(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  952(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  952(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  956(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  956(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  960(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  960(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  964(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  964(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  968(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  968(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  972(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  972(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  976(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  976(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  980(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  980(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  984(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  984(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  988(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  988(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  992(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  992(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0,  996(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0,  996(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0, 1000(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0, 1000(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0, 1004(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0, 1004(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0, 1008(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0, 1008(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0, 1012(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0, 1012(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  asm volatile("flw     %0, 1016(%1)"   : "+f"(A_tmp) : "r"(A) : "memory");
  asm volatile("flw     %0, 1016(%1)"   : "+f"(B_tmp) : "r"(B) : "memory");
  
  //------------------------------------
  bsg_saif_end();
  //------------------------------------
  C[0] = B_tmp;
	barrier.sync();

	return 0;
}
