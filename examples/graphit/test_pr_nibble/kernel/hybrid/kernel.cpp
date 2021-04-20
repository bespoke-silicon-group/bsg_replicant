//#define DEBUG
#include <bsg_manycore.h>

#ifdef DEBUG
#define BSG_TILE_GROUP_X_DIM 1 
#define BSG_TILE_GROUP_Y_DIM 1
#define bsg_tiles_X BSG_TILE_GROUP_X_DIM 
#define bsg_tiles_Y BSG_TILE_GROUP_Y_DIM 
#else
#include <bsg_set_tile_x_y.h>
// #define BSG_TILE_GROUP_X_DIM 16 
// #define BSG_TILE_GROUP_Y_DIM 8
#endif

#include <bsg_tile_group_barrier.hpp>
bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;

#include <pr_nibble.hpp>
#include <cstring>

#ifdef DEBUG
#define pr_dbg(fmt, ...)			\
		bsg_printf(fmt, ##__VA_ARGS__)
#else
#define pr_dbg(fmt, ...)
#endif

__attribute__((section(".dram"))) float  * __restrict p;
__attribute__((section(".dram"))) float  * __restrict old_rank;
__attribute__((section(".dram"))) float  * __restrict new_rank;
__attribute__((section(".dram"))) int  * __restrict out_degree;
__attribute__((section(".dram"))) int  * __restrict generated_tmp_vector_3;
//__attribute__((section(".dram"))) double alpha = 0.15; 
//__attribute__((section(".dram"))) double epsilon = (double) 1e-6; 

template <typename APPLY_FUNC > int edgeset_apply_pull_parallel_from_vertexset(int *in_indices , int *in_neighbors, int* from_vertexset, APPLY_FUNC apply_func, int V, int E, int block_size_x) 
{
  //if(bsg_id == 0) pr_dbg("val of root front: %i\n", from_vertexset[6]); 
  //if(bsg_id == 0) pr_dbg("size of graph: %i\n", V);
  int start, end;
  local_range(V, &start, &end);
  for ( int d = start; d < end; d++) {
    int degree = in_indices[d + 1] - in_indices[d];
    int * neighbors = &in_neighbors[in_indices[d]];
    for(int s = 0; s < degree; s++) { 
      if(from_vertexset[neighbors[s]]) {
        //pr_dbg("found a vertex to update: %i %i\n", neighbors[s], d); 
        apply_func (neighbors[s] , d);
      }
    } //end of loop on in neighbors
  } //end of outer for loop
  return 0;
} //end of edgeset apply function 

template <typename APPLY_FUNC > int edgeset_apply_push_parallel_from_vertexset(int *out_indices , int *out_neighbors, int* from_vertexset, APPLY_FUNC apply_func, int V, int E, int block_size_x) 
{
  //if(bsg_id == 0) pr_dbg("val of root front: %i\n", from_vertexset[6]); 
  //if(bsg_id == 0) pr_dbg("size of graph: %i\n", V);
  int start, end;
  local_range(V, &start, &end);
  for ( int s = start; s < end; s++) {
    if(from_vertexset[s]) {
      int degree = out_indices[s + 1] - out_indices[s];
      int * neighbors = &out_neighbors[out_indices[s]];
      for(int d = 0; d < degree; d++) { 
        apply_func (s, neighbors[d]);
	//if (new_rank[neighbors[d]] != 0.0){ pr_dbg("value updated in iteration: %i\n", neighbors[d]); }
	
      }
    } //end of loop on in neighbors
  } //end of outer for loop
  //barrier.sync();
  return 0;
} //end of edgeset apply function 


struct generated_vector_op_apply_func_4
{
  void operator() (int v)
  {
    out_degree[v] = generated_tmp_vector_3[v];
  };
};
struct new_rank_generated_vector_op_apply_func_2
{
  void operator() (int v)
  {
    new_rank[v] = ((float) 0) ;
  };
};
struct old_rank_generated_vector_op_apply_func_1
{
  void operator() (int v)
  {
    old_rank[v] = ((float) 0) ;
  };
};
struct p_generated_vector_op_apply_func_0
{
  void operator() (int v)
  {
    p[v] = ((float) 0) ;
  };
};
struct updateEdge
{
  void operator() (int src, int dst)
  {
    float alpha = 0.15; 
    new_rank[dst] = (new_rank[dst] + (((((1)  - alpha) / ((1)  + alpha)) * old_rank[src]) / out_degree[src]));
  };
};
struct updateSelf
{
  void operator() (int v)
  {
    float alpha = 0.15; 
    p[v] = (p[v] + ((((2)  * alpha) / ((1)  + alpha)) * old_rank[v]));
    new_rank[v] = (0) ;
  };
};
struct filter_frontier
{
  bool operator() (int v)
  {
    float epsilon = (float) 1e-6; 
    bool output ;
    //if(old_rank[v] == 0) return 0;
    if(new_rank[v] == 0) return 0;
    //output = (old_rank[v]) > ((out_degree[v] * epsilon));
    output = (new_rank[v]) > ((out_degree[v] * epsilon));
    return output;
  };
};

extern "C" int  __attribute__ ((noinline)) p_generated_vector_op_apply_func_0_kernel(int V) {
	int start, end;
	local_range(V, &start, &end);
	for (int iter_x = start; iter_x < end; iter_x++) {
		p_generated_vector_op_apply_func_0()(iter_x);
	}
	barrier.sync();
	return 0;
}
extern "C" int  __attribute__ ((noinline)) old_rank_generated_vector_op_apply_func_1_kernel(int V) {
	int start, end;
	local_range(V, &start, &end);
	for (int iter_x = start; iter_x < end; iter_x++) {
		old_rank_generated_vector_op_apply_func_1()(iter_x);
	}
	barrier.sync();
	return 0;
}
extern "C" int  __attribute__ ((noinline)) new_rank_generated_vector_op_apply_func_2_kernel(int V) {
	int start, end;
	local_range(V, &start, &end);
	for (int iter_x = start; iter_x < end; iter_x++) {
		new_rank_generated_vector_op_apply_func_2()(iter_x);
	}
	barrier.sync();
	return 0;
}
extern "C" int  __attribute__ ((noinline)) generated_vector_op_apply_func_4_kernel(int V) {
	int start, end;
	local_range(V, &start, &end);
	for (int iter_x = start; iter_x < end; iter_x++) {
		generated_vector_op_apply_func_4()(iter_x);
	}
	barrier.sync();
	return 0;
}
extern "C" int  __attribute__ ((noinline)) updateSelf_kernel(int * frontier, int V, int tag_c) {
  bsg_cuda_print_stat_start(tag_c);
	barrier.sync();
	int start, end;
	local_range(V, &start, &end);
	for (int iter_x = start; iter_x < end; iter_x++) {
		if(frontier[iter_x]) { updateSelf()(iter_x); }
	}
  bsg_cuda_print_stat_end(tag_c);
	barrier.sync();
	return 0;
}
extern "C" int __attribute__ ((noinline)) edgeset_apply_pull_parallel_from_vertexset_call(int *in_indices, int *in_neighbors, int *frontier, int V, int E, int block_size_x, int tag_c) {
	barrier.sync();
        //pr_dbg("%i: on update edges %i\n", bsg_id, tag_c);
  bsg_cuda_print_stat_start(tag_c);
  bsg_saif_start();
	edgeset_apply_pull_parallel_from_vertexset(in_indices, in_neighbors, frontier, updateEdge(), V, E, block_size_x);
  bsg_saif_end();
  bsg_cuda_print_stat_end(tag_c);
	barrier.sync();
	return 0;
}

 extern "C" int __attribute__ ((noinline)) edgeset_apply_push_parallel_from_vertexset_call(int *out_indices, int *out_neighbors, int *frontier, int V, int E, int block_size_x, int tag_c) {
	barrier.sync(); 
  bsg_cuda_print_stat_start(tag_c);
  bsg_saif_start();
	edgeset_apply_push_parallel_from_vertexset(out_indices, out_neighbors, frontier, updateEdge(), V, E, block_size_x);
  bsg_saif_end();
  bsg_cuda_print_stat_end(tag_c);
	barrier.sync();
	return 0;
}

extern "C" int __attribute__ ((noinline)) filter_frontier_where_call(int * next5, int V, int block_size_x, int tag_c) { 
        //if(bsg_id == 0) pr_dbg("0x%08x next, %i tag\n", next5, tag_c);
        //pr_dbg("%i: on frontier filter %i\n", bsg_id, tag_c);
  bsg_cuda_print_stat_start(tag_c);
	barrier.sync();
	int start, end;
	local_range(V, &start, &end);
	for (int iter_x = start; iter_x < end; iter_x++) {
		if (iter_x < V) {
			next5[iter_x] = 0;
			if ( filter_frontier()( iter_x ) ) {
				next5[iter_x] = 1;
				//pr_dbg("added vertex %i to frontier\n", iter_x);
			}
     		}
		else { break; }
	} //end of loop
  bsg_cuda_print_stat_end(tag_c);
	barrier.sync();
	return 0;
}


