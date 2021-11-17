#pragma once
#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_cuda_lite_barrier.h>
#include <bsg_barrier_amoadd.h>

#ifdef __cplusplus
extern "C" {
#endif
static inline void local_range(int n, int *start, int *end)
{
        int sz = (n / (bsg_tiles_X * bsg_tiles_Y)) + 1;
        *start = bsg_id * sz;
        *end = *start + sz;
        *end = *end < n ? *end : n;
}
#ifdef __cplusplus
}
#endif

int __attribute__((section(".dram"))) start_idx[bsg_tiles_X * bsg_tiles_Y] = {0};
int __attribute__((section(".dram"))) end_idx[bsg_tiles_X * bsg_tiles_Y] = {0};

void recursive_range(int n, int e, int grain_size, int start, int end, int * idx, int * edge_index)
{
  if(*idx == bsg_tiles_X * bsg_tiles_Y)
    return;
  //bsg_printf("beginning of recursive call, idx: %i, start: %i, end: %i, grain size: %i, total vertices: %i\n", *idx, start, end, grain_size, n);
  if ((start == end-1) || ((edge_index[end] - edge_index[start]) < grain_size)){
    //bsg_printf("inside if statement idx: %i start: %i end %i\n", *idx, start, end);
    start_idx[*idx] = start;
    end_idx[*idx] = end;
    *idx += 1;
  }
  else {
    recursive_range(n, e, grain_size, start, start + ((end-start) >> 1), idx, edge_index);
    recursive_range(n, e, grain_size, start + ((end-start)>>1), end, idx, edge_index);
  }

}

#ifdef __cplusplus
extern "C" {
#endif
static inline void edge_aware_local_range(int n, int e, int *start, int *end, int *edge_index)
{
        bsg_barrier_hw_tile_group_init();
        //TODO(Emily): need to implement this function to distribute work based on grain size
        int grain_size = (e/ (bsg_tiles_X * bsg_tiles_Y)) * 1.5f;
        if(bsg_id == 0) {
          int idx = 0;
          recursive_range(n, e, grain_size, 0, n, &idx, edge_index);
          if(end_idx[(bsg_tiles_X * bsg_tiles_Y) - 1] != n) end_idx[(bsg_tiles_X * bsg_tiles_Y) - 1] = n;
        }
        bsg_barrier_hw_tile_group_sync();
//        barrier.sync();
        *start = start_idx[bsg_id];
        *end = end_idx[bsg_id];
	//bsg_printf("start: %i, end: %i id: %i, edges: end %i - start %i, total edge: %i,  grain size: %i\n", start_idx[bsg_id], end_idx[bsg_id], bsg_id, edge_index[*end], edge_index[*start], e, grain_size);
}
#ifdef __cplusplus
}
#endif
