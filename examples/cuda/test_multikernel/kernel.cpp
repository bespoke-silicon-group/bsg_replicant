// This launches two kernels and synchronizes them

#include "bsg_manycore.h"
#include "bsg_set_tile_x_y.h"

#include "bsg_tile_group_barrier.hpp"

#define MAKE_FUNCTION_NAME(x) kernel##x
#define FUNCTION_NAME(x) \
  extern "C" __attribute__ ((noinline))                                               \
  int MAKE_FUNCTION_NAME(x)(int *buffer, int N, int *sync, int K) {                   \
  bsg_printf("Hello from kernel %d -- Tile X: %d Tile Y: %d\n", x, __bsg_x, __bsg_y); \
                                                                                      \
  for (int i = 0; i < N; i++) {                                                       \
      buffer[x*N+i] += 100*(x+1);                                                     \
      bsg_printf("kernel %d Process[%d]: %d\n", x, i, buffer[x*N+i]);                 \
  }                                                                                   \
                                                                                      \
  return 0;                                                                           \
}

FUNCTION_NAME(0)
FUNCTION_NAME(1)
FUNCTION_NAME(2)
FUNCTION_NAME(3)
FUNCTION_NAME(4)
FUNCTION_NAME(5)
FUNCTION_NAME(6)
FUNCTION_NAME(7)
FUNCTION_NAME(8)
FUNCTION_NAME(9)
FUNCTION_NAME(10)
FUNCTION_NAME(11)
FUNCTION_NAME(12)
FUNCTION_NAME(13)
FUNCTION_NAME(14)
FUNCTION_NAME(15)
FUNCTION_NAME(16)

