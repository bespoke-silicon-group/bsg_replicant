#define BSG_TILE_GROUP_X_DIM 16
#define BSG_TILE_GROUP_Y_DIM 8
#define bsg_tiles_X BSG_TILE_GROUP_X_DIM
#define bsg_tiles_Y BSG_TILE_GROUP_Y_DIM
#include <bsg_manycore.h>
#include <bsg_tile_group_barrier.hpp>
bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;

#include <pr_pull.hpp>
#include <cstring>
#ifdef DEBUG
#define pr_dbg(fmt, ...)                        \
    bsg_printf(fmt, ##__VA_ARGS__)
#else
#define pr_dbg(fmt, ...)
#endif

__attribute__((section(".dram"))) float damp;
__attribute__((section(".dram"))) float beta_score;

template <typename APPLY_FUNC > int edgeset_apply_pull_serial(vertexdata *in_vertices , int *in_neighbors, float * new_rank, float * contrib, APPLY_FUNC apply_func, int V, int E)
{
  int BLOCK_SIZE = 32;
  vertexdata lcl_nodes [ BLOCK_SIZE ];
  int lcl_rank [ BLOCK_SIZE ];
  int blk_dst_n = V/BLOCK_SIZE + (V%BLOCK_SIZE == 0 ? 0 : 1);
  for (int blk_dst_i = bsg_id; blk_dst_i < blk_dst_n; blk_dst_i += bsg_tiles_X * bsg_tiles_Y) {
    int blk_dst_base = blk_dst_i * BLOCK_SIZE;
    memcpy(&lcl_rank[0], &new_rank[blk_dst_base],sizeof(lcl_rank));
    memcpy(&lcl_nodes[0], &in_vertices[blk_dst_base], sizeof(lcl_nodes));
    for ( int d = 0; d < BLOCK_SIZE; d++) {
      int degree = lcl_nodes[d].degree;
      int * neighbors = &in_neighbors[lcl_nodes[d].offset];
      for(int s = 0; s < degree; s++) {
        int src = neighbors[s];
        lcl_rank[d] += contrib[src];
      } //end of loop on in neighbors
    } //end of dst node for loop
    memcpy(&new_rank[blk_dst_base], &lcl_rank[0], sizeof(lcl_rank));
  } //end of outer blocked loop
  return 0;
} //end of edgeset apply function

struct computeContrib
{
  void operator() (int v, float * contrib, float * old_rank, int * out_degree)
  {
    contrib[v] = (old_rank[v] / out_degree[v]);
  };
};
struct updateEdge
{
  void operator() (int src, int dst)
  {
 //   new_rank[dst] += (old_rank[src] / out_degree[src]);
  };
};
struct updateVertex
{
  void operator() (int v,float * old_rank, float *new_rank, float * error)
  {
    float old_score = old_rank[v];
    new_rank[v] = (beta_score + (damp * new_rank[v]));
    error[v] = fabs((new_rank[v] - old_rank[v])) ;
    old_rank[v] = new_rank[v];
    new_rank[v] = ((float) 0) ;
  };
};

extern "C" int  __attribute__ ((noinline)) computeContrib_kernel(float * contrib, float * old_rank, int * out_degree, int V) {
  bsg_cuda_print_stat_start(2);
  int start, end;
  local_range(V, &start, &end);
  pr_dbg("compute contrib start: %i, end: %i, id: %i\n", start, end, bsg_id);
  for (int d = start; d < end; d++) {
    computeContrib()(d, contrib, old_rank, out_degree);
  }
  bsg_cuda_print_stat_end(2);
  barrier.sync();
  return 0;
}
extern "C" int __attribute__ ((noinline)) edgeset_apply_pull_serial_call(vertexdata *in_indices, int *in_neighbors, float * new_rank, float * contrib, int V, int E) {
  barrier.sync();
  bsg_cuda_print_stat_start(3);
  edgeset_apply_pull_serial(in_indices, in_neighbors, new_rank, contrib, updateEdge(), V, E);
  bsg_cuda_print_stat_end(3);
  barrier.sync();
  return 0;
}
extern "C" int  __attribute__ ((noinline)) updateVertex_kernel(float * old_rank, float * new_rank, float * error, int V, int E) {
  bsg_cuda_print_stat_start(1);
  int start, end;
  local_range(V, &start, &end);
  pr_dbg("update vertex start: %i, end: %i, id: %i\n", start, end, bsg_id);
  for (int d = start; d < end; d++) {
      updateVertex()(d, old_rank, new_rank, error);
  }
  bsg_cuda_print_stat_end(1);
  barrier.sync();
  return 0;
}
