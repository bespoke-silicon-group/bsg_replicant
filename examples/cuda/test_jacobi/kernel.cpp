//This kernel adds 2 vectors

#define BSG_TILE_GROUP_X_DIM bsg_tiles_X
#define BSG_TILE_GROUP_Y_DIM bsg_tiles_Y
#include <math.h>
#include "bsg_manycore.h"
#include "bsg_set_tile_x_y.h"
#include "bsg_group_strider.hpp"
#include "bsg_cuda_lite_barrier.h"

extern "C" void compute(int c0, int c1, float * bsg_attr_noalias A0, float * bsg_attr_noalias Anext, float * bsg_attr_noalias a_left, float * bsg_attr_noalias a_right, float * bsg_attr_noalias a_up, float * bsg_attr_noalias a_down, float * bsg_attr_noalias a_self, bool x_l_bound, bool x_h_bound, bool y_l_bound, bool y_h_bound, const int nx, const int ny, const int nz, const int j, const int k);

extern "C" __attribute__ ((noinline)) __attribute__((used))
int kernel_jacobi(int c0, int c1, float *A0, float * Anext,
                  const int nx, const int ny, const int nz) {

  const bool x_l_bound = (__bsg_x == 0);
  const bool x_h_bound = (__bsg_x == (bsg_tiles_X-1));
  const bool y_l_bound = (__bsg_y == 0);
  const bool y_h_bound = (__bsg_y == (bsg_tiles_Y-1));
  const int j = __bsg_x + 1;
  const int k = __bsg_y + 1;

        bsg_nonsynth_saif_start();
  bsg_barrier_hw_tile_group_init();
  bsg_fence();
  bsg_barrier_hw_tile_group_sync();
  bsg_cuda_print_stat_kernel_start();

  // Calculate 2D XY distribution. One output per tile (temp).
  // Idea - unroll Z-axis (k). By 64, which is the input size

  // Check if additional load from DRAM is necessary

  // Buffer for A0
  float a_self[64] = {0.0f};

  // Auxillary buffers
  float aux_left[64];
  float aux_right[64];
  float aux_up[64];
  float aux_down[64];

  // Construct remote pointers
  float* a_up, *a_down, *a_left, *a_right;

  if (x_l_bound) {
    a_left = aux_left;
  } else {
    bsg_tile_group_strider<BSG_TILE_GROUP_X_DIM, 0, BSG_TILE_GROUP_Y_DIM, 0, float> r_left(a_self,  __bsg_x-1, __bsg_y);
    a_left = r_left.ptr;
  }
  if (x_h_bound) {
    a_right = aux_right;
  } else {
    bsg_tile_group_strider<BSG_TILE_GROUP_X_DIM, 0, BSG_TILE_GROUP_Y_DIM, 0, float> r_right(a_self,  __bsg_x+1, __bsg_y);
    a_right = r_right.ptr;
  }
  if (y_l_bound) {
    a_up = aux_up;
  } else {
    bsg_tile_group_strider<BSG_TILE_GROUP_X_DIM, 0, BSG_TILE_GROUP_Y_DIM, 0, float> r_up(a_self,  __bsg_x, __bsg_y-1);
    a_up = r_up.ptr;
  }
  if (y_h_bound) {
    a_down = aux_down;
  } else {
    bsg_tile_group_strider<BSG_TILE_GROUP_X_DIM, 0, BSG_TILE_GROUP_Y_DIM, 0, float> r_down(a_self,  __bsg_x, __bsg_y+1);
    a_down = r_down.ptr;
  }

  compute(c0, c1, A0, Anext, a_left, a_right, a_up, a_down, a_self, x_l_bound, x_h_bound, y_l_bound, y_h_bound, nx, ny, nz, j, k);
  
  bsg_cuda_print_stat_kernel_end();
  bsg_fence();
  bsg_barrier_hw_tile_group_sync();
        bsg_nonsynth_saif_end();
	return 0;
}
