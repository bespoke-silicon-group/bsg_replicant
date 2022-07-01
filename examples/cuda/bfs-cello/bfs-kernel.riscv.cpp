#include "bsg_manycore.h"
#include "bsg_set_tile_x_y.h"
#include "bfs/graph.h"
#include "bfs/sparse_set.h"
#include "bsg_cuda_lite_barrier.h"
#include "cello.hpp"

#define STEP 4

extern "C" void cello_start(void);

graph_t *G;
bsg_attr_remote sparse_set_t *frontier_in;
bsg_attr_remote int *frontier_out;
bsg_attr_remote int *visited;

extern "C" int bfs(graph_t *_G,
        bsg_attr_remote sparse_set_t *_frontier_in,
        bsg_attr_remote int *_frontier_out,
        bsg_attr_remote int *_visited) {
    // set globals
    G = _G;
    frontier_in = _frontier_in;
    frontier_out = _frontier_out;
    visited = _visited;
    // init barrier
    bsg_barrier_hw_tile_group_init();
    bsg_barrier_hw_tile_group_sync();
    // stat print start
    bsg_cuda_print_stat_kernel_start();
    // start cello
    cello_start();
    bsg_barrier_hw_tile_group_sync();
    // stat print stop
    bsg_cuda_print_stat_kernel_end();
    bsg_fence();
    bsg_barrier_hw_tile_group_sync();   
    return 0;
}

int cello_main()
{
    cello::foreach(0, frontier_in->set_size, [=](int src_i){
            int src = frontier_in->members[src_i];
            kernel_vertex_data_ptr_t src_data = &G->vertex_data[src];
            int degree = src_data->degree;
            kernel_edge_data_ptr_t neib = src_data->neib;
            int dst_i;
            // unrolled
            for (dst_i = 0; dst_i+STEP < degree; dst_i++) {
                // read in dst
                int dst[STEP];
                for (int k = 0; k < STEP; k++) {
                    dst[k] = neib[dst_i+k];
                }
                // read visited
                int vis[STEP];
                for (int k = 0; k < STEP; k++) {
                    vis[k] = visited[dst[k]];
                }
                // add to frontier and visited
                for (int k = 0; k < STEP; k++) {
                    if (vis[k] == 0) {
                        frontier_out[dst[k]] = 1;
                        visited[dst[k]] = 1;
                    }
                }
            }
            // strip mine
            for (; dst_i < degree; dst_i++) {
                int dst = neib[dst_i];
                if (visited[dst] == 0) {
                    frontier_out[dst] = 1;
                    visited[dst] = 1;
                }
            }
        });
    return 0;
}
