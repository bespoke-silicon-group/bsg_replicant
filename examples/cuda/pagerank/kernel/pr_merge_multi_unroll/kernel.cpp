
#define BSG_TILE_GROUP_X_DIM 16
#define BSG_TILE_GROUP_Y_DIM 8
#define bsg_tiles_X BSG_TILE_GROUP_X_DIM
#define bsg_tiles_Y BSG_TILE_GROUP_Y_DIM
#define CACHE_LINE 16
#define GRANULARITY_PULL 10
#include <bsg_manycore.h>
#include <bsg_tile_group_barrier.hpp>
bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;
#include <atomic>
#include <pr_pull.hpp>
#include <cstring>

//#define DEBUG

#ifdef DEBUG
#define pr_dbg(fmt, ...)                        \
    do{ if (__bsg_id == 0) bsg_printf(fmt, ##__VA_ARGS__); } while (0)
#else
#define pr_dbg(fmt, ...)
#endif

//keep these:
__attribute__((section(".dram"))) float damp;
__attribute__((section(".dram"))) float beta_score;

__attribute__((section(".dram"))) std::atomic<int> workq;

extern "C" int __attribute__ ((noinline)) pr_merge_all(int bsg_attr_remote * bsg_attr_noalias in_indices, int bsg_attr_remote * bsg_attr_noalias in_neighbors, int bsg_attr_remote * bsg_attr_noalias out_degree, float bsg_attr_remote * bsg_attr_noalias old_rank, float bsg_attr_remote * bsg_attr_noalias new_rank, float bsg_attr_remote * bsg_attr_noalias contrib, float bsg_attr_remote * bsg_attr_noalias contrib_new, int V) {
  bsg_barrier_hw_tile_group_init();
  int tag = 2; 
  bsg_cuda_print_stat_start(tag);
//  pr_dbg("Enter the edge pull kernel, V is: %d\n", V);
  int start = 0;
  int end = V;
  int length = end - start;
//  pr_dbg("Rows within block is %d, start is %d, and end is %d\n", rows_within_pod, start, end);
//    for (int id = start + __bsg_id; id < end; id = id + bsg_tiles_X * bsg_tiles_Y) {
  for(int id = workq.fetch_add(GRANULARITY_PULL, std::memory_order_relaxed); id < length; id = workq.fetch_add(GRANULARITY_PULL, std::memory_order_relaxed)) {
    int stop = (id + GRANULARITY_PULL) > length ? length : (id + GRANULARITY_PULL);
    for (int d = start + id; d < start + stop; d++) {
      register float temp_new = 0.0;
      register float temp_old = old_rank[d];
      register float error = 0.0;
      register int first = in_indices[d];
      register int last = in_indices[d+1];
      register int od = out_degree[d];
//      bsg_printf("Handling row %d, first is %d, last is %d, bsg_id is %d\n", d, first, last, __bsg_id);
      int s = first;
      for(; s <= last - 12; s = s + 12) {
        register int idx0 = in_neighbors[s];
        register int idx1 = in_neighbors[s+1];
        register int idx2 = in_neighbors[s+2];
        register int idx3 = in_neighbors[s+3];
        register int idx4 = in_neighbors[s+4];
        register int idx5 = in_neighbors[s+5];
        register int idx6 = in_neighbors[s+6];
        register int idx7 = in_neighbors[s+7];
        register int idx8 = in_neighbors[s+8];
        register int idx9 = in_neighbors[s+9];
        register int idx10 = in_neighbors[s+10];
        register int idx11 = in_neighbors[s+11];
      
        register float tmp_0 = contrib[idx0];
        register float tmp_1 = contrib[idx1];
        register float tmp_2 = contrib[idx2];
        register float tmp_3 = contrib[idx3];
        register float tmp_4 = contrib[idx4];
        register float tmp_5 = contrib[idx5];
        register float tmp_6 = contrib[idx6];
        register float tmp_7 = contrib[idx7];
        register float tmp_8 = contrib[idx8];
        register float tmp_9 = contrib[idx9];
        register float tmp_10 = contrib[idx10];
        register float tmp_11 = contrib[idx11];
       
        temp_new = temp_new + tmp_0 + tmp_1 + tmp_2 + tmp_3
                    + tmp_4 + tmp_5 + tmp_6 + tmp_7
                    + tmp_8 + tmp_9 + tmp_10 + tmp_11; 
      } //end of loop on in neighbors
      for(; s <= last - 6; s = s + 6) {
        register int idx0 = in_neighbors[s];
        register int idx1 = in_neighbors[s+1];
        register int idx2 = in_neighbors[s+2];
        register int idx3 = in_neighbors[s+3];
        register int idx4 = in_neighbors[s+4];
        register int idx5 = in_neighbors[s+5];

        register float tmp_0 = contrib[idx0];
        register float tmp_1 = contrib[idx1];
        register float tmp_2 = contrib[idx2];
        register float tmp_3 = contrib[idx3];
        register float tmp_4 = contrib[idx4];
        register float tmp_5 = contrib[idx5];
        temp_new = temp_new + tmp_0 + tmp_1 + tmp_2 + tmp_3
                    + tmp_4 + tmp_5;
      }
      for(; s < last; s++) {
        register int idx = in_neighbors[s];
        register float tmp = contrib[idx];
        temp_new = temp_new + tmp;
      }

      temp_new = beta_score + damp * temp_new;
      error = fabs(temp_new - temp_old);
      old_rank[d] = temp_new;
      contrib_new[d] = temp_new / od;           
    }
  }
  bsg_cuda_print_stat_end(tag);
  bsg_fence();
  bsg_barrier_hw_tile_group_sync();
  return 0;
}


