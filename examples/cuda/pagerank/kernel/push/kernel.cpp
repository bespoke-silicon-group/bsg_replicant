#define BSG_TILE_GROUP_X_DIM 16
#define BSG_TILE_GROUP_Y_DIM 8
#define bsg_tiles_X BSG_TILE_GROUP_X_DIM
#define bsg_tiles_Y BSG_TILE_GROUP_Y_DIM
#include <bsg_manycore.h>
#include <bsg_tile_group_barrier.hpp>
bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;

#include <pr_pull.hpp>
#include <cstring>

//#define DEBUG

#ifdef DEBUG
#define pr_dbg(fmt, ...)                        \
    bsg_printf(fmt, ##__VA_ARGS__)
#else
#define pr_dbg(fmt, ...)
#endif

//keep these:
__attribute__((section(".dram"))) float damp;
__attribute__((section(".dram"))) float beta_score;

template <typename APPLY_FUNC > int edgeset_apply_push_serial(int *out_indices , int *out_neighbors, float * new_rank, float * contrib, APPLY_FUNC apply_func, int V, int E)
{
    int start, end;
    local_range(V, &start, &end);
    pr_dbg("serial func %i start: %i end %i \n", bsg_id, start, end);
    for ( int s=start; s < end; s++) {
          int degree = out_indices[s + 1] - out_indices[s];
          int * neighbors = &out_neighbors[out_indices[s]];
          for(int d = 0; d < degree; d++) {
            apply_func ( s, neighbors[d], new_rank, contrib);
          } //end of for loop on neighbors
    }
    //barrier.sync();
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
  void operator() (int src, int dst, float * new_rank, float * contrib)
  {
    new_rank[dst] += contrib[src]; 
  };
};
struct updateVertex
{
  void operator() (int v, float * old_rank, float *new_rank, float * error)
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
extern "C" int __attribute__ ((noinline)) edgeset_apply_push_serial_call(int *out_indices, int *out_neighbors, float * new_rank, float * contrib, int V, int E) {
  barrier.sync();
  bsg_cuda_print_stat_start(3);
  edgeset_apply_push_serial(out_indices, out_neighbors, new_rank, contrib, updateEdge(), V, E);
  bsg_cuda_print_stat_end(3);
  barrier.sync();
	return 0;
}
extern "C" int  __attribute__ ((noinline)) updateVertex_kernel(float * old_rank, float *new_rank, float * error, int V, int E) {
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
