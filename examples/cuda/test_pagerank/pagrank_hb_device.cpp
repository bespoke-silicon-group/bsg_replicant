#include "bsg_manycore.h"
#include "bsg_set_tile_x_y.h"
#define BSG_TILE_GROUP_X_DIM bsg_tiles_X
#define BSG_TILE_GROUP_Y_DIM bsg_tiles_Y
#include "bsg_tile_group_barrier.hpp"
bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;
#include <local_range.h>
#include <vertex_struct.h>
#include <atomics.h>
__attribute__((section(".dram"))) double  * __restrict old_rank;
__attribute__((section(".dram"))) double  * __restrict new_rank;
__attribute__((section(".dram"))) int  * __restrict out_degree;
__attribute__((section(".dram"))) double  * __restrict contrib;
__attribute__((section(".dram"))) double  * __restrict error;
__attribute__((section(".dram"))) int  * __restrict generated_tmp_vector_2;
__attribute__((section(".dram"))) double damp; 
__attribute__((section(".dram"))) double beta_score; 

template <typename APPLY_FUNC > int edgeset_apply_push_parallel(int *out_indices , int *out_neighbors, APPLY_FUNC apply_func, int V, int E, int block_size_x) 
{ 
  int start, end;
  local_range(V, &start, &end);
  for ( int s = start; s < end; s++) {
    int degree = out_indices[s + 1] - out_indices[s];
    int * neighbors = &out_neighbors[out_indices[s]];
    for(int d = 0; d < degree; d++) { 
      apply_func ( s, neighbors[d] );
    } //end of for loop on neighbors
  }
  barrier.sync();
  return 0;
} //end of edgeset apply function 


struct error_generated_vector_op_apply_func_5
{
  void operator() (int v)
  {
    error[v] = ((float) 0) ;
  };
};
struct contrib_generated_vector_op_apply_func_4
{
  void operator() (int v)
  {
    contrib[v] = ((float) 0) ;
  };
};
struct generated_vector_op_apply_func_3
{
  void operator() (int v)
  {
    out_degree[v] = generated_tmp_vector_2[v];
  };
};
struct new_rank_generated_vector_op_apply_func_1
{
  void operator() (int v)
  {
    new_rank[v] = ((float) 0) ;
  };
};
struct old_rank_generated_vector_op_apply_func_0
{
  void operator() (int v)
  {
    old_rank[v] = (((float) 1)  / hammerblade::builtin_getVerticesHB(edges));
  };
};
struct computeContrib
{
  void operator() (int v)
  {
    contrib[v] = (old_rank[v] / out_degree[v]);
  };
};
struct updateEdge
{
  void operator() (int src, int dst)
  {
    writeAdd( &new_rank[dst], contrib[src] ); 
  };
};
struct updateVertex
{
  void operator() (int v)
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
  void operator() (int v)
  {
    old_rank[v] = (((float) 1)  / hammerblade::builtin_getVerticesHB(edges));
    new_rank[v] = ((float) 0) ;
  };
};

extern "C" int  __attribute__ ((noinline)) old_rank_generated_vector_op_apply_func_0_kernel(int V) {
	int start, end;
	local_range(V, &start, &end);
	for (int iter_x = start; iter_x < end; iter_x++) {
		old_rank_generated_vector_op_apply_func_0()(iter_x);
	}
	barrier.sync();
	return 0;
}
extern "C" int  __attribute__ ((noinline)) new_rank_generated_vector_op_apply_func_1_kernel(int V) {
	int start, end;
	local_range(V, &start, &end);
	for (int iter_x = start; iter_x < end; iter_x++) {
		new_rank_generated_vector_op_apply_func_1()(iter_x);
	}
	barrier.sync();
	return 0;
}
extern "C" int  __attribute__ ((noinline)) generated_vector_op_apply_func_3_kernel(int V) {
	int start, end;
	local_range(V, &start, &end);
	for (int iter_x = start; iter_x < end; iter_x++) {
		generated_vector_op_apply_func_3()(iter_x);
	}
	barrier.sync();
	return 0;
}
extern "C" int  __attribute__ ((noinline)) contrib_generated_vector_op_apply_func_4_kernel(int V) {
	int start, end;
	local_range(V, &start, &end);
	for (int iter_x = start; iter_x < end; iter_x++) {
		contrib_generated_vector_op_apply_func_4()(iter_x);
	}
	barrier.sync();
	return 0;
}
extern "C" int  __attribute__ ((noinline)) error_generated_vector_op_apply_func_5_kernel(int V) {
	int start, end;
	local_range(V, &start, &end);
	for (int iter_x = start; iter_x < end; iter_x++) {
		error_generated_vector_op_apply_func_5()(iter_x);
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
extern "C" int  __attribute__ ((noinline)) computeContrib_kernel(int V) {
	int start, end;
	local_range(V, &start, &end);
	for (int iter_x = start; iter_x < end; iter_x++) {
		computeContrib()(iter_x);
	}
	barrier.sync();
	return 0;
}
extern "C" int __attribute__ ((noinline)) edgeset_apply_push_parallel_call(int *out_indices, int *out_neighbors, int V, int E, int block_size_x) {
	edgeset_apply_push_parallel(out_indices, out_neighbors, updateEdge(), V, E, block_size_x);
	return 0;
}
extern "C" int  __attribute__ ((noinline)) updateVertex_kernel(int V) {
	int start, end;
	local_range(V, &start, &end);
	for (int iter_x = start; iter_x < end; iter_x++) {
		updateVertex()(iter_x);
	}
	barrier.sync();
	return 0;
}


