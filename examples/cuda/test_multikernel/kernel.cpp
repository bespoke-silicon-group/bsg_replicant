// This launches two kernels and synchronizes them

#include "bsg_manycore.h"
#include "bsg_set_tile_x_y.h"

#include "bsg_tile_group_barrier.hpp"

extern "C" __attribute__ ((noinline)) 
int kernel0(int *buffer, int N, int *sync, int K) {

  bsg_printf("Hello from kernel 0: Tile X: %x Tile Y: %x\n", __bsg_x, __bsg_y);

  for (int i = 0; i < N; i++) {
      buffer[0*N+i] += 100;
      bsg_printf("kernel 0 Process[%d]: %d\n", i, buffer[0*N+i]);
  } 

  return 0;
}

extern "C" __attribute__ ((noinline)) 
int kernel1(int *buffer, int N, int *sync, int K) {

  bsg_printf("Hello from kernel 1: Tile X: %x Tile Y: %x\n", __bsg_x, __bsg_y);

  for (int i = 0; i < N; i++) {
      buffer[1*N+i] += 200;
      bsg_printf("kernel 1 Process[%d]: %d\n", i, buffer[1*N+i]);
  } 

  return 0;
}

extern "C" __attribute__ ((noinline)) 
int kernel2(int *buffer, int N, int *sync, int K) {

  bsg_printf("Hello from kernel 2: Tile X: %x Tile Y: %x\n", __bsg_x, __bsg_y);

  for (int i = 0; i < N; i++) {
      buffer[2*N+i] += 300;
      bsg_printf("kernel 2 Process[%d]: %d\n", i, buffer[2*N+i]);
  } 

  return 0;
}

extern "C" __attribute__ ((noinline)) 
int kernel3(int *buffer, int N, int *sync, int K) {

  bsg_printf("Hello from kernel 3: Tile X: %x Tile Y: %x\n", __bsg_x, __bsg_y);

  for (int i = 0; i < N; i++) {
      buffer[3*N+i] += 400;
      bsg_printf("kernel 3 Process[%d]: %d\n", i, buffer[3*N+i]);
  } 

  return 0;
}

