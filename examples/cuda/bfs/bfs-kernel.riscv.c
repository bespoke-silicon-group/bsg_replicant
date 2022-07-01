#include "bsg_manycore.h"
#include "bsg_set_tile_x_y.h"
#include "bfs/graph.h"
#include "bfs/sparse_set.h"
#include "bsg_cuda_lite_barrier.h"

int bfs(graph_t *G_ptr,
        bsg_attr_remote sparse_set_t *frontier_in,
        bsg_attr_remote int *frontier_out,
        bsg_attr_remote int *visited)
{
    // init barrier
    bsg_barrier_hw_tile_group_init();
    bsg_barrier_hw_tile_group_sync();    
    bsg_cuda_print_stat_kernel_start();

    graph_t G = *G_ptr;
    for (int src_i = __bsg_id;
         src_i < frontier_in->set_size;
         src_i += bsg_tiles_X*bsg_tiles_Y) {
        // update all neibs
        int src = frontier_in->members[src_i];
        kernel_vertex_data_ptr_t src_data = &G.vertex_data[src];
        int degree = src_data->degree;
        kernel_edge_data_ptr_t neib = src_data->neib;
        for (int dst_i = 0; dst_i < degree; dst_i++) {
            int dst = neib[dst_i];
            if (visited[dst] == 0) {
                frontier_out[dst] = 1;
                visited[dst] = 1;
            }
        }
    }
    bsg_barrier_hw_tile_group_sync();    
    bsg_cuda_print_stat_kernel_end();
    bsg_fence();
    bsg_barrier_hw_tile_group_sync();
    return 0;
}
