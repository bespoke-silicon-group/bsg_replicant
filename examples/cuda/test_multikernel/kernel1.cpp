// This launches two kernels and synchronizes them

#include "bsg_manycore.h"
#include "bsg_set_tile_x_y.h"

#include "bsg_tile_group_barrier.hpp"

extern "C" __attribute__ ((noinline)) 
int kernel1(int *buffer, int N, int K, int *sync) {

  bsg_printf("Hello from kernel 1: Tile X: %x Tile Y: %x\n", __bsg_x, __bsg_y);

  for (int i = 0; i < N; i++) {
      buffer[1*N+i] += 200;
      bsg_printf("kernel 1 Process[%d]: %d\n", i, buffer[1*N+i]);
  } 

  return 0;
}
