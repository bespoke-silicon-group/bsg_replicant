#include "bsg_manycore.h"
#include "bsg_set_tile_x_y.h"
#define BSG_TILE_GROUP_X_DIM bsg_tiles_X
#define BSG_TILE_GROUP_Y_DIM bsg_tiles_Y
#include "bsg_tile_group_barrier.hpp"
bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;
#include <local_range.h>
#include <vertex_struct.h>
__attribute__((section(".dram")))  * __restrict parent;

template <typename APPLY_FUNC > int edgeset_apply_push_parallel_from_vertexset_with_frontier(vertexdata *out_vertices , int *out_neighbors, int* from_vertexset, int * next_frontier, APPLY_FUNC apply_func, int V, int E, int block_size_x) 
{ 
  int BLOCK_SIZE = 32; //cache line size
  vertexdata lcl_nodes[ BLOCK_SIZE ];
  int lcl_frontier [ BLOCK_SIZE ];
  int blk_src_n = V/BLOCK_SIZE + (V%BLOCK_SIZE == 0 ? 0 : 1);
  for (int blk_src_i = bsg_id; blk_src_i < blk_src_n; blk_src_i += bsg_tiles_X*bsg_tiles_Y) {
    int block_off = blk_src_i * BLOCK_SIZE;
    memcpy(&lcl_frontier[0], &frontier[block_off], BLOCK_SIZE * sizeof(lcl_frontier[0]));
    memcpy(&lcl_nodes[0], &out_vertices[block_off], BLOCK_SIZE * sizeof(lcl_nodes[0]));
    for(int s = 0; s < BLOCK_SIZE; s++) {
      if(lcl_frontier[s] == 0) continue;
      const int * neighbors = &out_neighbors[lcl_nodes[s].offset];
      int dst_n = lcl_nodes[s].degree;
      for( int d = 0; d < dst_n; d++) {
        int dst = neighbors[d];
        if( apply_func ( (block_off + s), dst )) {
          next_frontier[dst] = 1;
        }
      } //end of for loop on neighbors
    } //end of for loop on source nodes
  } //end of loop on blocks
  barrier.sync();
  return 0;
} //end of edgeset apply function 


struct parent_generated_vector_op_apply_func_0
{
  void operator() (int v)
  {
    parent[v] =  -(1) ;
  };
};
struct updateEdge
{
  bool operator() (int src, int dst)
  {
    bool output1 ;
    parent[dst] = src;
    output1 = (bool) 1;
    return output1;
  };
};
struct toFilter
{
  bool operator() (int v)
  {
    bool output ;
    output = (parent[v]) == ( -(1) );
    return output;
  };
};

extern "C" int  __attribute__ ((noinline)) parent_generated_vector_op_apply_func_0_kernel(int V) {
	int start, end;
	local_range(V, &start, &end);
	for (int iter_x = start, iter_x < end; iter_x++) {
		parent_generated_vector_op_apply_func_0()(iter_x);
	}
	barrier.sync();
	return 0;
}
extern "C" int __attribute__ ((noinline)) edgeset_apply_push_parallel_from_vertexset_with_frontier_call(vertexdata *out_indices, int *out_neighbors, int *frontier, next_frontier, int V, int E, int block_size_x) {
	edgeset_apply_push_parallel_from_vertexset_with_frontier(out_indices, out_neighbors, frontier, updateEdge(), next_frontier);
	return 0;
}


