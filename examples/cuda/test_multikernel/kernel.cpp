// This file defines N number of mostly identical kernels (up to 16 currently)
// These kernels will
// - print hello from a tile
// - process an input buffer of size N by adding 100*its kernel id
// - sync using a global AMOADD, waiting for all K kernels to reach

#include "bsg_manycore.h"
#include "bsg_set_tile_x_y.h"
#include "bsg_manycore_atomic.h"

#define MAKE_KERNEL_DEF(x) kernel##x
#define KERNEL_DEF(x) \
  extern "C" __attribute__ ((noinline))                                               \
  int MAKE_KERNEL_DEF(x)(int *buffer, int N, int *sync, int K) {                      \
  bsg_printf("Hello from kernel %d -- Tile X: %d Tile Y: %d\n", x, __bsg_x, __bsg_y); \
                                                                                      \
  for (int i = 0; i < N; i++) {                                                       \
      buffer[x*N+i] += 100*(x+1);                                                     \
      bsg_printf("kernel %d Process[%d]: %d\n", x, i, buffer[x*N+i]);                 \
  }                                                                                   \
                                                                                      \
  volatile int *sync_ptr = sync;                                                      \
  int sync_val;                                                                       \
  bsg_printf("[kernel %d] Trying to sync to EVA: %x", x, sync);                       \
  bsg_amoadd(sync, 1);                                                                \
  while ((sync_val = *sync_ptr) < K) {                                                \
     bsg_printf("[kernel %d] Waiting for sync_ptr == %d (%d)", x, K, sync_val);       \
  }                                                                                   \
                                                                                      \
  return 0;                                                                           \
}

KERNEL_DEF(0)
KERNEL_DEF(1)
KERNEL_DEF(2)
KERNEL_DEF(3)
KERNEL_DEF(4)
KERNEL_DEF(5)
KERNEL_DEF(6)
KERNEL_DEF(7)
KERNEL_DEF(8)
KERNEL_DEF(9)
KERNEL_DEF(10)
KERNEL_DEF(11)
KERNEL_DEF(12)
KERNEL_DEF(13)
KERNEL_DEF(14)
KERNEL_DEF(15)
KERNEL_DEF(16)

