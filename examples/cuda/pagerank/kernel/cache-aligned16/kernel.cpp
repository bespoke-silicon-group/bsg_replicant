
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
#define BLOCK 16

#ifdef DEBUG
#define pr_dbg(fmt, ...)                        \
    bsg_printf(fmt, ##__VA_ARGS__)
#else
#define pr_dbg(fmt, ...)
#endif

//keep these:
__attribute__((section(".dram"))) float damp;
__attribute__((section(".dram"))) float beta_score;

template <typename APPLY_FUNC > int edgeset_apply_pull_serial(int *in_indices , int *in_neighbors, float * new_rank, float * contrib, APPLY_FUNC apply_func, int V, int E)
{
  int blocks = V/BLOCK;
  if(V%BLOCK != 0) blocks += 1;
  bsg_cuda_print_stat_start(4);
  for(int i = bsg_id; i < blocks; i+= bsg_tiles_X*bsg_tiles_Y) {
    int start = BLOCK * i;
    int end = BLOCK * (i + 1);
    for ( int d=start; d < end; d++) {
      for(int s = in_indices[d]; s < in_indices[d+1]; s++) {
        apply_func( in_neighbors[s], d , new_rank, contrib);
      } //end of loop on in neighbors
    } //end of outer for loop
  }
  //barrier.sync();
  bsg_cuda_print_stat_end(4);
  return 0;
} //end of edgeset apply function


struct error_generated_vector_op_apply_func_5
{
  void operator() (int v, float * error)
  {
    error[v] = ((float) 0) ;
  };
};
struct contrib_generated_vector_op_apply_func_4
{
  void operator() (int v, float * contrib)
  {
    contrib[v] = ((float) 0) ;
  };
};
struct generated_vector_op_apply_func_3
{
  void operator() (int v, int * out_degree, int * generated_tmp_vector_2)
  {
    out_degree[v] = generated_tmp_vector_2[v];
  };
};
struct new_rank_generated_vector_op_apply_func_1
{
  void operator() (int v, float * new_rank)
  {
    new_rank[v] = ((float) 0) ;
  };
};
struct old_rank_generated_vector_op_apply_func_0
{
  void operator() (int v, float * old_rank, int E)
  {
    old_rank[v] = (((float) 1)  / E);
  };
};
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
    double old_score = old_rank[v];
    new_rank[v] = (beta_score + (damp * new_rank[v]));
    error[v] = fabs((new_rank[v] - old_rank[v])) ;
    old_rank[v] = new_rank[v];
    new_rank[v] = ((float) 0) ;
  };
};

struct reset
{
  void operator() (int v, float * old_rank, float * new_rank, int E)
  {
    old_rank[v] = (((float) 1)  / E);
    new_rank[v] = ((float) 0) ;
  };
};

extern "C" int  __attribute__ ((noinline)) old_rank_generated_vector_op_apply_func_0_kernel(float * old_rank, int V, int E) {
	int start, end;
  local_range(V, &start, &end);
	for (int d = start; d < end; d++) {
	   old_rank_generated_vector_op_apply_func_0()(d, old_rank, E);
	}
  barrier.sync();
	return 0;
}
extern "C" int  __attribute__ ((noinline)) new_rank_generated_vector_op_apply_func_1_kernel(int V, int E, int block_size_x) {
	int start_x = block_size_x * (__bsg_tile_group_id_y * __bsg_grid_dim_x + __bsg_tile_group_id_x);
	for (int iter_x = __bsg_id; iter_x < block_size_x; iter_x += bsg_tiles_X * bsg_tiles_Y) {
		if ((start_x + iter_x) < V) {
	//		new_rank_generated_vector_op_apply_func_1()(start_x + iter_x);
		}
		else {
			break;
		}
	}
  barrier.sync();
	return 0;
}
extern "C" int  __attribute__ ((noinline)) generated_vector_op_apply_func_3_kernel(int V, int E, int block_size_x) {
	int start_x = block_size_x * (__bsg_tile_group_id_y * __bsg_grid_dim_x + __bsg_tile_group_id_x);
	for (int iter_x = __bsg_id; iter_x < block_size_x; iter_x += bsg_tiles_X * bsg_tiles_Y) {
		if ((start_x + iter_x) < V) {
//			generated_vector_op_apply_func_3()(start_x + iter_x);
		}
		else {
			break;
		}
	}
  barrier.sync();
	return 0;
}
extern "C" int  __attribute__ ((noinline)) contrib_generated_vector_op_apply_func_4_kernel(int V, int E, int block_size_x) {
	int start_x = block_size_x * (__bsg_tile_group_id_y * __bsg_grid_dim_x + __bsg_tile_group_id_x);
	for (int iter_x = __bsg_id; iter_x < block_size_x; iter_x += bsg_tiles_X * bsg_tiles_Y) {
		if ((start_x + iter_x) < V) {
//			contrib_generated_vector_op_apply_func_4()(start_x + iter_x);
		}
		else {
			break;
		}
	}
  barrier.sync();
	return 0;
}
extern "C" int  __attribute__ ((noinline)) error_generated_vector_op_apply_func_5_kernel(int V, int E, int block_size_x) {
	int start_x = block_size_x * (__bsg_tile_group_id_y * __bsg_grid_dim_x + __bsg_tile_group_id_x);
	for (int iter_x = __bsg_id; iter_x < block_size_x; iter_x += bsg_tiles_X * bsg_tiles_Y) {
		if ((start_x + iter_x) < V) {
//			error_generated_vector_op_apply_func_5()(start_x + iter_x);
		}
		else {
			break;
		}
	}
  barrier.sync();
	return 0;
}
extern "C" int  __attribute__ ((noinline)) reset_kernel(int V, int E, int block_size_x) {
	int start_x = block_size_x * (__bsg_tile_group_id_y * __bsg_grid_dim_x + __bsg_tile_group_id_x);
	for (int iter_x = __bsg_id; iter_x < block_size_x; iter_x += bsg_tiles_X * bsg_tiles_Y) {
		if ((start_x + iter_x) < V) {
//			reset()(start_x + iter_x);
		}
		else {
			break;
		}
	}
  barrier.sync();
	return 0;
}
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
extern "C" int __attribute__ ((noinline)) edgeset_apply_pull_serial_call(int *out_indices, int *out_neighbors, float * new_rank, float * contrib, int V, int E) {
  barrier.sync();
  bsg_cuda_print_stat_start(3);
	edgeset_apply_pull_serial(out_indices, out_neighbors, new_rank, contrib, updateEdge(), V, E);
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
