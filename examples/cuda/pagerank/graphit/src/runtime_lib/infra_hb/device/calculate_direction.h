#pragma once
#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>

int __attribute__((section(".dram"))) pull = 0;

static inline int calculate_direction(int * frontier, int * out_indices, int V, int E)
{
  int n = 0;
  int outEdges = 0;
  if(bsg_id == 0) {
    for(int i = 0; i < V; i++) {
      if(frontier[i] == 1) {
        n++;
        outEdges += out_indices[i+1] - out_indices[i];
      }
//      if(i % 10000 == 0) bsg_printf("iter: %i / %i \n", i, V);
    }
    if(n + outEdges > E / 20) pull = 1;
  }
  barrier.sync();
  return pull;
}
