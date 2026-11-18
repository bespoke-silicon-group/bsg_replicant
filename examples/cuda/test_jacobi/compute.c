#include <bsg_manycore.h>
#include "bsg_cuda_lite_barrier.h"
#include <stdbool.h>
#define Index3D(_nx,_ny,_i,_j,_k) ((_i)+_nx*((_j)+_ny*(_k)))
// copy 64 elements along X axis
void copyXAxis64(float bsg_attr_remote * bsg_attr_noalias src, float bsg_attr_remote * bsg_attr_noalias dst) {
  for (int i = 0; i < 4; i++) {
    float tmp00 =  src[0];
    float tmp01 =  src[1];
    float tmp02 =  src[2];
    float tmp03 =  src[3];
    float tmp04 =  src[4];
    float tmp05 =  src[5];
    float tmp06 =  src[6];
    float tmp07 =  src[7];
    float tmp08 =  src[8];
    float tmp09 =  src[9];
    float tmp10 = src[10];
    float tmp11 = src[11];
    float tmp12 = src[12];
    float tmp13 = src[13];
    float tmp14 = src[14];
    float tmp15 = src[15];
    asm volatile("": : :"memory");
     dst[0] = tmp00;
     dst[1] = tmp01;
     dst[2] = tmp02;
     dst[3] = tmp03;
     dst[4] = tmp04;
     dst[5] = tmp05;
     dst[6] = tmp06;
     dst[7] = tmp07;
     dst[8] = tmp08;
     dst[9] = tmp09;
    dst[10] = tmp10;
    dst[11] = tmp11;
    dst[12] = tmp12;
    dst[13] = tmp13;
    dst[14] = tmp14;
    dst[15] = tmp15;
    dst += 16;
    src += 16;
  }
  return;
}


void compute(int c0, int c1, float bsg_attr_remote * bsg_attr_noalias A0, float bsg_attr_remote * bsg_attr_noalias Anext, float bsg_attr_remote * bsg_attr_noalias a_left, float bsg_attr_remote * bsg_attr_noalias a_right, float bsg_attr_remote * bsg_attr_noalias a_up, float bsg_attr_remote * bsg_attr_noalias a_down, float bsg_attr_remote * bsg_attr_noalias a_self, bool x_l_bound, bool x_h_bound, bool y_l_bound, bool y_h_bound, const int nx, const int ny, const int nz, const int j, const int k){
  for (int ii = 1; ii < nx-1; ii += 62) {

    // Inital load -- we load 64 and produce 62
    if (x_l_bound) {
      copyXAxis64(&(A0[Index3D (nx, ny, ii-1, j-1, k)]), a_left);
    }
    if (x_h_bound) {
      copyXAxis64(&(A0[Index3D (nx, ny, ii-1, j+1, k)]), a_right);
    }
    if (y_l_bound) {
      copyXAxis64(&(A0[Index3D (nx, ny, ii-1, j, k-1)]), a_up);
    }
    if (y_h_bound) {
      copyXAxis64(&(A0[Index3D (nx, ny, ii-1, j, k+1)]), a_down);
    }

    copyXAxis64(&(A0[Index3D (nx, ny, ii-1, j, k)]), a_self);
    bsg_barrier_hw_tile_group_sync();

    bsg_unroll(8)
    for (int i = 1; i < 63; i++) {
      // Load top
      // top = A0[Index3D (nx, ny, i+1, j, k)];
      float    top = a_self[i+1];
      float bottom = a_self[i-1];

      float left  = a_left[i];
      float right = a_right[i];
      float    up = a_up[i];
      float  down = a_down[i];
      float self = a_self[i];

      // Jacobi
      float next = (top + bottom + left + right + up + down) * c1 - self * c0;
      Anext[Index3D (nx, ny, ii-1+i, j, k)] = next;
    }
    bsg_barrier_hw_tile_group_sync();
  }
}
