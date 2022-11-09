// This launches two kernels and synchronizes them

#include "bsg_manycore.h"
#include "bsg_set_tile_x_y.h"

#include "bsg_tile_group_barrier.hpp"

extern "C" __attribute__ ((noinline)) 
int kernel0(int *buffer, int N) {

  bsg_printf("Hello from kernel 0: Tile X: %x Tile Y: %x\n", __bsg_x, __bsg_y);

  for (int i = 0; i < N; i++) {
      buffer[i] += 100;
      bsg_printf("kernel 0 Process[%x]: %x\n", i, buffer[i]);
  } 

  return 0;
}
