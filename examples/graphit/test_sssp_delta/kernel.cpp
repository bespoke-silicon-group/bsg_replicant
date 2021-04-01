#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>

//#define BSG_TILE_GROUP_X_DIM 16 
//#define BSG_TILE_GROUP_Y_DIM 2
//#define bsg_tiles_X BSG_TILE_GROUP_X_DIM
//#define bsg_tiles_Y BSG_TILE_GROUP_Y_DIM
#include <bsg_tile_group_barrier.hpp>
bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;
#include <sssp.hpp>

//#define DEBUG
#ifdef DEBUG
#define pr_dbg(fmt, ...)	\
	bsg_printf(fmt, ##__VA_ARGS__)
#else
#define pr_dbg(fmt, ...)
#endif


__attribute__((section(".dram"))) int  * __restrict dist;

template <typename APPLY_FUNC > int edgeset_apply_pull_parallel_weighted_deduplicated_from_vertexset_with_frontier(int *in_indices , WNode *in_neighbors, int* from_vertexset, int * next_frontier, APPLY_FUNC apply_func, int V, int E, int block_size_x) 
{ 
  bsg_cuda_print_stat_start(1);
  bsg_saif_start();
  int start, end;
  local_range(V, &start, &end);
  if(bsg_id == 0) pr_dbg("elem 1: %i and dist: %i and random weight: %i\n", from_vertexset[5], dist[5], in_neighbors[in_indices[5]].weight);
  for ( int d = start; d < end; d++) {
    int degree = in_indices[d + 1] - in_indices[d];
    WNode * neighbors = &in_neighbors[in_indices[d]];
    for(int s = 0; s < degree; s++) { 
      if(from_vertexset[neighbors[s].vertex]) {
        if( apply_func ( neighbors[s].vertex, d, neighbors[s].weight )) { 
          next_frontier[d] = 1;
        }
      }
    } //end of loop on in neighbors
  } //end of outer for loop
  bsg_saif_end();
  bsg_cuda_print_stat_end(1);
  barrier.sync();
  return 0;
} //end of edgeset apply function 


struct dist_generated_vector_op_apply_func_0
{
  void operator() (int v)
  {
    dist[v] = (2147483647) ;
  };
};
struct updateEdge
{
  bool operator() (int src, int dst, int weight)
  {
    bool output3 = false;
    int new_dist = (dist[src] + weight);
    if(dist[dst] > new_dist) {
      dist[dst] = new_dist;
      output3 = true;
    }
    return output3;
  };
};
struct reset
{
  void operator() (int v)
  {
    dist[v] = (2147483647) ;
  };
};

extern "C" int  __attribute__ ((noinline)) dist_generated_vector_op_apply_func_0_kernel(int V) {
	int start, end;
	local_range(V, &start, &end);
	for (int iter_x = start; iter_x < end; iter_x++) {
		dist_generated_vector_op_apply_func_0()(iter_x);
	}
	barrier.sync();
	return 0;
}
extern "C" int  __attribute__ ((noinline)) reset_kernel(int V) {
	int start, end;
	local_range(V, &start, &end);
	for (int iter_x = start; iter_x < end; iter_x++) {
		reset()(iter_x);
	}
	barrier.sync();
	return 0;
}
extern "C" int __attribute__ ((noinline)) edgeset_apply_pull_parallel_weighted_deduplicated_from_vertexset_with_frontier_call(int *in_indices, WNode *in_neighbors, int *frontier, int *modified_vertexsubset1, int V, int E, int block_size_x) {
	edgeset_apply_pull_parallel_weighted_deduplicated_from_vertexset_with_frontier(in_indices, in_neighbors, frontier, modified_vertexsubset1, updateEdge(), V, E, block_size_x);
	return 0;
}


