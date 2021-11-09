#include "bsg_manycore.h"
#include "bsg_set_tile_x_y.h"
#include "bfs/graph.h"
#include "bfs/sparse_set.h"
#include <atomic>
#include "bsg_cuda_lite_barrier.h"

#define GRANULARITY_PULL 20
#define GRANULARITY_PUSH 5

__attribute__((section(".dram")))                                                                                                                            
std::atomic<int> workq;

extern "C" int bfs(graph_t *G_csr_ptr,
        graph_t *G_csc_ptr,
        bsg_attr_remote sparse_set_t *frontier_in_sparse,
        bsg_attr_remote int *frontier_in_dense,
        bsg_attr_remote int *frontier_out,
        bsg_attr_remote int *visited,
        bsg_attr_remote int *direction,
        bsg_attr_remote int *num_pods){

    //bsg_cuda_print_stat_start(*ite);
    //bsg_barrier_hw_tile_group_init();
    bsg_cuda_print_stat_kernel_start();
    //bsg_cuda_print_stat_start(ite);
    //if(__bsg_tile_group_id == 0){
        //bsg_printf("===== enter device,direction is========================================%d\n",*direction);
        //bsg_printf("g_csr is %d, g_csc is %d, frontier_sp is %d,frontier_d is %d, frontierout is %d, visited is %d, ite is %d\n",(unsigned)G_csr_ptr,(unsigned)G_csc_ptr,(unsigned)frontier_in_sparse,(unsigned)frontier_in_dense,(unsigned)frontier_out,(unsigned)visited,*ite);
    //}
    
    if (*direction == 0){
        graph_t G = *G_csr_ptr;
        int num_nodes = G.V;
        for (int src_base_i = workq.fetch_add(GRANULARITY_PULL, std::memory_order_relaxed); src_base_i < num_nodes; src_base_i = workq.fetch_add(GRANULARITY_PULL, std::memory_order_relaxed)) {
           // update all neibs
           
            //bsg_printf("===== src_base_i is %d, from tile %d=======================================\n",src_base_i,__bsg_tile_group_id);    
           
            int stop = (src_base_i + GRANULARITY_PULL > num_nodes) ? num_nodes : src_base_i + GRANULARITY_PULL;
            for (int src_i = src_base_i; src_i<stop; src_i++){
                //use mask to check vertex[src_i] has not been visted yet
                if (visited[src_i]==0){
                    //int temp_frontier_idx=0
                    kernel_vertex_data_ptr_t src_data = &G.vertex_data[src_i];
                    int degree = src_data->degree;
                    kernel_edge_data_ptr_t neib = src_data->neib;
                    //check each incoming edge,if one of the incoming edge matches the frontier, set match =1 and break
                    for (int dst_i = 0; dst_i < degree; dst_i++) {
                        //dst is the incoming edge currently being checked
                        int dst = neib[dst_i];
                   
                        //if find a match in the coming edge, stop matching other coming edges
                        if (frontier_in_dense[dst] == 1){
                            visited[src_i] = 1;
                            frontier_out[src_i] = 1;
                            break;
                        }
                    }
                }
            }
        }

    }
    else{
        graph_t G = *G_csc_ptr;
        int num_nodes = G.V;
        for (int src_base_i = workq.fetch_add(GRANULARITY_PUSH, std::memory_order_relaxed); src_base_i < frontier_in_sparse->set_size; src_base_i = workq.fetch_add(GRANULARITY_PUSH, std::memory_order_relaxed)) {
        // update all neibs
            int stop = (src_base_i + GRANULARITY_PUSH > frontier_in_sparse->set_size) ? frontier_in_sparse->set_size : src_base_i + GRANULARITY_PUSH;
            for (int src_i = src_base_i; src_i<stop; src_i++){
                int src = frontier_in_sparse->members[src_i];
                kernel_vertex_data_ptr_t src_data = &G.vertex_data[src];
                int degree = src_data->degree;
                kernel_edge_data_ptr_t neib = src_data->neib;
                int dst_i = 0;
                for (; dst_i+7 < degree; dst_i +=8) {
                    int dst1 = neib[dst_i];
                    int dst2 = neib[dst_i+1];
                    int dst3 = neib[dst_i+2];
                    int dst4 = neib[dst_i+3];
                    int dst5 = neib[dst_i+4];
                    int dst6 = neib[dst_i+5];
                    int dst7 = neib[dst_i+6];
                    int dst8 = neib[dst_i+7];

                    int visited1 = visited[dst1];
                    int visited2 = visited[dst2];
                    int visited3 = visited[dst3];
                    int visited4 = visited[dst4];
                    int visited5 = visited[dst5];
                    int visited6 = visited[dst6];
                    int visited7 = visited[dst7];
                    int visited8 = visited[dst8];

                    if (visited1 == 0) {
                        frontier_out[dst1] = 1;
                        visited[dst1] = 1;
                    }
                    if (visited2 == 0) {
                        frontier_out[dst2] = 1;
                        visited[dst2] = 1;
                    }
                    if (visited3 == 0) {
                        frontier_out[dst3] = 1;
                        visited[dst3] = 1;
                    }
                    if (visited4 == 0) {
                        frontier_out[dst4] = 1;
                        visited[dst4] = 1;
                    }
                    if (visited5 == 0) {
                        frontier_out[dst5] = 1;
                        visited[dst5] = 1;
                    }
                    if (visited6 == 0) {
                        frontier_out[dst6] = 1;
                        visited[dst6] = 1;
                    }
                    if (visited7 == 0) {
                        frontier_out[dst7] = 1;
                        visited[dst7] = 1;
                    }
                    if (visited8 == 0) {
                        frontier_out[dst8] = 1;
                        visited[dst8] = 1;
                    }
                }
                if (dst_i+6 < degree){
                    int dst1 = neib[dst_i];
                    int dst2 = neib[dst_i+1];
                    int dst3 = neib[dst_i+2];
                    int dst4 = neib[dst_i+3];
                    int dst5 = neib[dst_i+4];
                    int dst6 = neib[dst_i+5];
                    int dst7 = neib[dst_i+6];

                    int visited1 = visited[dst1];
                    int visited2 = visited[dst2];
                    int visited3 = visited[dst3];
                    int visited4 = visited[dst4];
                    int visited5 = visited[dst5];
                    int visited6 = visited[dst6];
                    int visited7 = visited[dst7];             

                    if (visited1 == 0) {
                        frontier_out[dst1] = 1;
                        visited[dst1] = 1;
                    }
                    if (visited2 == 0) {
                        frontier_out[dst2] = 1;
                        visited[dst2] = 1;
                    }
                    if (visited3 == 0) {
                        frontier_out[dst3] = 1;
                        visited[dst3] = 1;
                    }
                    if (visited4 == 0) {
                        frontier_out[dst4] = 1;
                        visited[dst4] = 1;
                    }
                    if (visited5 == 0) {
                        frontier_out[dst5] = 1;
                        visited[dst5] = 1;
                    }
                    if (visited6 == 0) {
                        frontier_out[dst6] = 1;
                        visited[dst6] = 1;
                    }
                    if (visited7 == 0) {
                        frontier_out[dst7] = 1;
                        visited[dst7] = 1;
                    }
                }
                else if (dst_i+5 < degree){
                    int dst1 = neib[dst_i];
                    int dst2 = neib[dst_i+1];
                    int dst3 = neib[dst_i+2];
                    int dst4 = neib[dst_i+3];
                    int dst5 = neib[dst_i+4];
                    int dst6 = neib[dst_i+5];
                    //int dst7 = neib[dst_i+6];

                    int visited1 = visited[dst1];
                    int visited2 = visited[dst2];
                    int visited3 = visited[dst3];
                    int visited4 = visited[dst4];
                    int visited5 = visited[dst5];
                    int visited6 = visited[dst6];
                    //int visited7 = visited[dst7];             

                    if (visited1 == 0) {
                        frontier_out[dst1] = 1;
                        visited[dst1] = 1;
                    }
                    if (visited2 == 0) {
                        frontier_out[dst2] = 1;
                        visited[dst2] = 1;
                    }
                    if (visited3 == 0) {
                        frontier_out[dst3] = 1;
                        visited[dst3] = 1;
                    }
                    if (visited4 == 0) {
                        frontier_out[dst4] = 1;
                        visited[dst4] = 1;
                    }
                    if (visited5 == 0) {
                        frontier_out[dst5] = 1;
                        visited[dst5] = 1;
                    }
                    if (visited6 == 0) {
                        frontier_out[dst6] = 1;
                        visited[dst6] = 1;
                    }
                }
                else if (dst_i+4 < degree){
                    int dst1 = neib[dst_i];
                    int dst2 = neib[dst_i+1];
                    int dst3 = neib[dst_i+2];
                    int dst4 = neib[dst_i+3];
                    int dst5 = neib[dst_i+4];
                    //int dst6 = neib[dst_i+5];
                    //int dst7 = neib[dst_i+6];

                    int visited1 = visited[dst1];
                    int visited2 = visited[dst2];
                    int visited3 = visited[dst3];
                    int visited4 = visited[dst4];
                    int visited5 = visited[dst5];
                    //int visited6 = visited[dst6];
                    //int visited7 = visited[dst7];             

                    if (visited1 == 0) {
                        frontier_out[dst1] = 1;
                        visited[dst1] = 1;
                    }
                    if (visited2 == 0) {
                        frontier_out[dst2] = 1;
                        visited[dst2] = 1;
                    }
                    if (visited3 == 0) {
                        frontier_out[dst3] = 1;
                        visited[dst3] = 1;
                    }
                    if (visited4 == 0) {
                        frontier_out[dst4] = 1;
                        visited[dst4] = 1;
                    }
                    if (visited5 == 0) {
                        frontier_out[dst5] = 1;
                        visited[dst5] = 1;
                    }
                }
                else if (dst_i+3 < degree){
                    int dst1 = neib[dst_i];
                    int dst2 = neib[dst_i+1];
                    int dst3 = neib[dst_i+2];
                    int dst4 = neib[dst_i+3];
                    //int dst5 = neib[dst_i+4];
                    //int dst6 = neib[dst_i+5];
                    //int dst7 = neib[dst_i+6];

                    int visited1 = visited[dst1];
                    int visited2 = visited[dst2];
                    int visited3 = visited[dst3];
                    int visited4 = visited[dst4];
                    //int visited5 = visited[dst5];
                    //int visited6 = visited[dst6];
                    //int visited7 = visited[dst7];             

                    if (visited1 == 0) {
                        frontier_out[dst1] = 1;
                        visited[dst1] = 1;
                    }
                    if (visited2 == 0) {
                        frontier_out[dst2] = 1;
                        visited[dst2] = 1;
                    }
                    if (visited3 == 0) {
                        frontier_out[dst3] = 1;
                        visited[dst3] = 1;
                    }
                    if (visited4 == 0) {
                        frontier_out[dst4] = 1;
                        visited[dst4] = 1;
                    }
                }
                else if (dst_i+2 < degree){
                    int dst1 = neib[dst_i];
                    int dst2 = neib[dst_i+1];
                    int dst3 = neib[dst_i+2];
                    

                    int visited1 = visited[dst1];
                    int visited2 = visited[dst2];
                    int visited3 = visited[dst3];
                                 

                    if (visited1 == 0) {
                        frontier_out[dst1] = 1;
                        visited[dst1] = 1;
                    }
                    if (visited2 == 0) {
                        frontier_out[dst2] = 1;
                        visited[dst2] = 1;
                    }
                    if (visited3 == 0) {
                        frontier_out[dst3] = 1;
                        visited[dst3] = 1;
                    }
                }
                else if (dst_i+1 < degree){
                    int dst1 = neib[dst_i];
                    int dst2 = neib[dst_i+1];
                    

                    int visited1 = visited[dst1];
                    int visited2 = visited[dst2];
                    
                                 

                    if (visited1 == 0) {
                        frontier_out[dst1] = 1;
                        visited[dst1] = 1;
                    }
                    if (visited2 == 0) {
                        frontier_out[dst2] = 1;
                        visited[dst2] = 1;
                    }
                    
                }
                else if (dst_i < degree){
                    int dst = neib[dst_i];
                    if (visited[dst] ==0){
                        frontier_out[dst] = 1;
                        visited[dst] = 1;
                    }
                }
            }
        }   
    }
    
    bsg_cuda_print_stat_kernel_end();
    return 0;
}
