#include "bsg_manycore.h"
#include "bsg_set_tile_x_y.h"
#include "bfs/graph.h"
#include "bfs/sparse_set.h"
#include "bsg_manycore_atomic.h"
#include <atomic>

#include "bsg_cuda_lite_barrier.h"



#define GRANULARITY_PULL 20
#define GRANULARITY_PUSH 5
#define GRANULARITY_INDEX 16

//bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;

__attribute__((section(".dram"))) std::atomic<int> workq;

__attribute__((section(".dram"))) std::atomic<int> index_rd;

__attribute__((section(".dram"))) std::atomic<int> index_wr;

extern "C" int bfs(graph_t * bsg_attr_noalias G_csr_ptr,
        graph_t * bsg_attr_noalias G_csc_ptr,
        sparse_set_t bsg_attr_remote * bsg_attr_noalias frontier_in_sparse,
        int bsg_attr_remote * bsg_attr_noalias frontier_in_dense,
        int bsg_attr_remote * bsg_attr_noalias frontier_out_sparse,
        int bsg_attr_remote * bsg_attr_noalias frontier_out_dense,
        int bsg_attr_remote * bsg_attr_noalias visited,
        int bsg_attr_remote * bsg_attr_noalias direction,
        //bsg_attr_remote int *ite_id,
        int bsg_attr_remote * bsg_attr_noalias outlen,
        int bsg_attr_remote * bsg_attr_noalias cachewarm   ){

    //bsg_cuda_print_stat_start(*ite);
    //bsg_cuda_print_stat_kernel_start();
    bsg_barrier_hw_tile_group_init();
    
    //pseduo read to warm up LLC with input frontier for testing road maps
    //shold be commented if input frontier size is larger than 512KB
    int cmp = 0;
    if(*cachewarm==1){
      for(int i=0; i<frontier_in_sparse->set_size;i++){
          int tmp = frontier_in_sparse->members[i];
          if (tmp > cmp) cmp = tmp;
      }
    }

    bsg_cuda_print_stat_start(0);
    //PULL direction
    if (*direction == 0){
        
        graph_t G = *G_csr_ptr;
        int num_nodes = G.V;
        for (int src_base_i = workq.fetch_add(GRANULARITY_PULL, std::memory_order_relaxed); src_base_i < num_nodes; src_base_i = workq.fetch_add(GRANULARITY_PULL, std::memory_order_relaxed)) {
           // update all neibs
           
            //bsg_printf("===== src_base_i is %d, from tile %d=======================================\n",src_base_i,__bsg_tile_group_id);    
           
            int stop = (src_base_i + GRANULARITY_PULL > num_nodes) ? num_nodes : src_base_i + GRANULARITY_PULL;
            for (int src_i = src_base_i; src_i<stop; src_i++){
                //use mask to check vertex[src_i] has not been visted yet
                int bit_chunk = src_i/32;
                int bit_position = 1<< (src_i % 32);
                int visited_check = visited[bit_chunk] & bit_position;
                if (!visited_check){
                    //int temp_frontier_idx=0
                    kernel_vertex_data_ptr_t src_data = &G.vertex_data[src_i];
                    int degree = src_data->degree;
                    kernel_edge_data_ptr_t neib = src_data->neib;
                    //check each incoming edge,if one of the incoming edge matches the frontier, set match =1 and break
                    for (int dst_i = 0; dst_i < degree; dst_i++) {
                        //dst is the incoming edge currently being checked
                        int dst = neib[dst_i];
                   
                        //if find a match in the coming edge, stop matching other coming edges
                        
                        if (frontier_in_dense[dst/32] & (1<<(dst%32))){
                            int result_visit = bsg_amoor(&visited[bit_chunk],bit_position);
                            //visited[src_i] = 1;
                            int result_frontier = bsg_amoor(&frontier_out_dense[bit_chunk],bit_position);
                            //frontier_out_dense[src_i] = 1;
                            break;
                        }
                    }
                }
            }
        }
        //bsg_cuda_print_stat_end(1);

    }
    else{
        //bsg_cuda_print_stat_start(2);
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

                bsg_unroll(16)
                for(int dst_i=0; dst_i<degree; dst_i ++){
                  int dst = neib[dst_i];
                  if(!(visited[dst/32]&(1<<(dst%32)))){
                    int result_visit = bsg_amoor(&visited[dst/32],1<<(dst%32));  
                    if (! (result_visit & (1<<(dst%32)) ) ){
                      int out_idx = index_wr.fetch_add(1, std::memory_order_relaxed);
                      frontier_out_sparse[out_idx] = dst;  
                    }
                    //int result_frontier = bsg_amoor(&frontier_out_dense[dst/32],1<<(dst%32));
                  }
                }


            }
        }
        //bsg_cuda_print_stat_end(2);   
    }
    bsg_cuda_print_stat_end(0);
    // the phase which generates output frontier in sparse set format
    //#############################################################
    bsg_barrier_hw_tile_group_sync();
    //barrier.sync();
    //#############################################################
    //bsg_cuda_print_stat_start(1);
    
    
    /*for (int src_base_i = index_rd.fetch_add(GRANULARITY_INDEX, std::memory_order_relaxed); src_base_i < *outlen; src_base_i = index_rd.fetch_add(GRANULARITY_INDEX, std::memory_order_relaxed)) {
        int stop = (src_base_i + GRANULARITY_INDEX > *outlen) ? *outlen : src_base_i + GRANULARITY_INDEX;
        for (int src_i = src_base_i; src_i<stop; src_i++){
            if(frontier_out_dense[src_i/32]&(1<<(src_i%32))){
                int out_idx = index_wr.fetch_add(1, std::memory_order_relaxed);
                frontier_out_sparse[out_idx] = src_i;
                //int result_frontier = bsg_amoor(&frontier_out_dense[src_i/32],1<<(src_i%32));
                //frontier_out_dense[src_i] = 0;
                //bsg_printf("========================= output frontier element : %d, src_base_i: %d, tile id: %d=======================================\n",src_i,src_base_i,__bsg_id);
            }
        }
    }
    */
    //bsg_cuda_print_stat_end(1);
    //write the output frontier length
    //#############################################################
    bsg_barrier_hw_tile_group_sync();
    //#############################################################
    //bsg_cuda_print_stat_start(3);
    if(__bsg_id == 0){
        *outlen = index_wr.load();   
    //    bsg_printf("========================= output frontier size : %d, work_q value: %d, rd_idx value: %d=======================================\n",index_wr.load(),workq.load(),index_rd.load());
        *direction = cmp; // write cmp so that the pseduo code is not optimized away
    }

    //bsg_cuda_print_stat_end(1);
    bsg_barrier_hw_tile_group_sync();

    //bsg_cuda_print_stat_end(1);
    //bsg_cuda_print_stat_kernel_end();
    return 0;
}
