
#define BSG_TILE_GROUP_X_DIM 16
#define BSG_TILE_GROUP_Y_DIM 8
#define bsg_tiles_X BSG_TILE_GROUP_X_DIM
#define bsg_tiles_Y BSG_TILE_GROUP_Y_DIM
#define CACHE_LINE 16
#define GRANULARITY_PULL 10
#define KERNEL_CURRENT_POD SIM_KERNEL_CURRENT_POD
#define KERNEL_CURRENT_BLOCK SIM_KERNEL_CURRENT_BLOCK
#define KERNEL_NUM_PODS 64
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

__attribute__((section(".dram"))) std::atomic<int> workq[KERNEL_NUM_PODS];

int edgeset_apply_pull_serial(int *in_indices , int *in_neighbors, float * new_rank, float * contrib, int V, int E)
{
  bsg_barrier_hw_tile_group_init();
  pr_dbg("Enter the edge pull kernel\n"); 
  int rows_within_pod = (V % KERNEL_NUM_PODS) == 0 ? (V / KERNEL_NUM_PODS) : (V / KERNEL_NUM_PODS + 1);
  int start = KERNEL_CURRENT_POD * rows_within_pod;
  int end = (start + rows_within_pod) > V ? V : (start + rows_within_pod);
  int length = end - start;
  int tag_block_end = (KERNEL_CURRENT_BLOCK + 13) > KERNEL_NUM_PODS ? KERNEL_NUM_PODS : (KERNEL_CURRENT_BLOCK + 13);
  pr_dbg("Rows within block is %d, start is %d, and end is %d\n", rows_within_pod, start, end);
//  int start, end;
//  edge_aware_local_range(V,E, &start, &end, in_indices);
  int tag = 2;
//  pr_dbg("serial func %i start: %i end %i \n", bsg_id, start, end);
  for(int b = 0; b < KERNEL_NUM_PODS; b++) {
    if(b >= KERNEL_CURRENT_BLOCK && b < tag_block_end) {
      bsg_cuda_print_stat_start(tag);
//      pr_dbg("Tag is %d\n", tag);     
    }
//    for (int id = start + __bsg_id; id < end; id = id + bsg_tiles_X * bsg_tiles_Y) {
    for(int id = workq[b].fetch_add(GRANULARITY_PULL, std::memory_order_relaxed); id < length; id = workq[b].fetch_add(GRANULARITY_PULL, std::memory_order_relaxed)) {
//    pr_dbg("d is %d, start and end is %d and %d\n", d, in_indices[d], in_indices[d+1]);
      int stop = (id + GRANULARITY_PULL) > length ? length : (id + GRANULARITY_PULL);
      for (int d = start + id; d < start + stop; d++) {
        float temp = new_rank[d];
        int current_block = (b + KERNEL_CURRENT_POD) < KERNEL_NUM_PODS ? (b + KERNEL_CURRENT_POD) : (b + KERNEL_CURRENT_POD - KERNEL_NUM_PODS);
        int offset = (d - start) * (KERNEL_NUM_PODS+1)+current_block;
        int first = in_indices[offset];
        int last = in_indices[offset+1];
//        bsg_printf("Handling block %d of row %d, id is %d, current_block is %d, offset is %d, first is %d, last is %d, bsg_id is %d\n", b, d, id, current_block, offset, first, last, __bsg_id);
        int s = first;
        for(; s <= last - 4; s = s + 4) {
          register int idx0 = in_neighbors[s];
          register int idx1 = in_neighbors[s+1];
          register int idx2 = in_neighbors[s+2];
          register int idx3 = in_neighbors[s+3];
//        register int idx4 = in_neighbors[s+4];
//        register int idx5 = in_neighbors[s+5];
//        register int idx6 = in_neighbors[s+6];
//        register int idx7 = in_neighbors[s+7];
      
          register float tmp_0 = contrib[idx0];
          register float tmp_1 = contrib[idx1];
          register float tmp_2 = contrib[idx2];
          register float tmp_3 = contrib[idx3];
//        register float tmp_4 = contrib[idx4];
//        register float tmp_5 = contrib[idx5];
//        register float tmp_6 = contrib[idx6];
//        register float tmp_7 = contrib[idx7];
       
          temp = temp + tmp_0 + tmp_1 + tmp_2 + tmp_3; 
        } //end of loop on in neighbors
        for(; s < last; s++) {
          register int idx = in_neighbors[s];
          register float tmp = contrib[idx];
          temp = temp + tmp;
        }
        new_rank[d] = temp;
      }
    } //end of outer for loop
    bsg_barrier_hw_tile_group_sync();
    if(b >= KERNEL_CURRENT_BLOCK && b < tag_block_end) {
      bsg_cuda_print_stat_end(tag);
      tag = tag + 1;
    }
  } //end of edgeset apply function
  return 0;
}

static inline void align_range(int V, int* start, int * end) {
  *start = CACHE_LINE * bsg_id;
  int temp_end = *start + CACHE_LINE;
  *end = (temp_end > V) ? V : temp_end;  
}

extern "C" int  __attribute__ ((noinline)) computeContrib_kernel(float * contrib, float * old_rank, int * out_degree, int V) {
  bsg_cuda_print_stat_start(1);
  bsg_barrier_hw_tile_group_init();
  int start, end;
  int rows_within_pod = (V % KERNEL_NUM_PODS) == 0 ? (V / KERNEL_NUM_PODS) : (V / KERNEL_NUM_PODS + 1);
  int pod_start = KERNEL_CURRENT_POD * rows_within_pod;
  int pod_end = (pod_start + rows_within_pod) > V ? V : (pod_start + rows_within_pod);
  int length = pod_end - pod_start;
//  int rows_within_pod = (V % KERNEL_NUM_PODS) == 0 ? (V / KERNEL_NUM_PODS) : (V / KERNEL_NUM_PODS + 1);
  align_range(length, &start, &end);
  register float one = ((float) 1);
  start = start + pod_start;
  int cache_total = CACHE_LINE * bsg_tiles_X * bsg_tiles_Y;
  pr_dbg("compute contrib start: %i, end: %i, id: %i, and total number of nodes %d\n", start, end, bsg_id, V);
  for (; start < pod_end; start = start + cache_total) {
    end = start + CACHE_LINE;
    end = end > pod_end ? pod_end : end;
    int d = start;
    for(; d <= end - 8; d = d + 8) {
      register int od0 = out_degree[d];
      register int od1 = out_degree[d+1];
      register int od2 = out_degree[d+2];
      register int od3 = out_degree[d+3];
      register int od4 = out_degree[d+4];
      register int od5 = out_degree[d+5];
      register int od6 = out_degree[d+6];
      register int od7 = out_degree[d+7];

      register float tp_old0 = old_rank[d];
      register float tp_old1 = old_rank[d+1];
      register float tp_old2 = old_rank[d+2];
      register float tp_old3 = old_rank[d+3];
      register float tp_old4 = old_rank[d+4];
      register float tp_old5 = old_rank[d+5];
      register float tp_old6 = old_rank[d+6];
      register float tp_old7 = old_rank[d+7];     
     
      register float cont0 = tp_old0 / od0;
      register float cont1 = tp_old1 / od1;
      register float cont2 = tp_old2 / od2;
      register float cont3 = tp_old3 / od3;
      register float cont4 = tp_old4 / od4;
      register float cont5 = tp_old5 / od5;
      register float cont6 = tp_old6 / od6;
      register float cont7 = tp_old7 / od7;

      contrib[d] = cont0;
      contrib[d+1] = cont1;
      contrib[d+2] = cont2;
      contrib[d+3] = cont3;
      contrib[d+4] = cont4;
      contrib[d+5] = cont5;
      contrib[d+6] = cont6;
      contrib[d+7] = cont7;
    }
    for(; d < end; d++) {
      register int od = out_degree[d];
      register float tp_old = old_rank[d];
      register float cont = tp_old / od;
      contrib[d] = cont;
      
    }
  }
  bsg_cuda_print_stat_end(1);
  bsg_barrier_hw_tile_group_sync();
  return 0;
}
extern "C" int __attribute__ ((noinline)) edgeset_apply_pull_serial_call(int *out_indices, int *out_neighbors, float * new_rank, float * contrib, int V, int E) {
  edgeset_apply_pull_serial(out_indices, out_neighbors, new_rank, contrib, V, E);
  bsg_barrier_hw_tile_group_sync();
  return 0;
}
extern "C" int  __attribute__ ((noinline)) updateVertex_kernel(float * old_rank, float *new_rank, float * error, int V, int E) {
  bsg_cuda_print_stat_start(15);
  bsg_barrier_hw_tile_group_init();
  int start, end;
  int rows_within_pod = (V % KERNEL_NUM_PODS) == 0 ? (V / KERNEL_NUM_PODS) : (V / KERNEL_NUM_PODS + 1);
  int pod_start = KERNEL_CURRENT_POD * rows_within_pod;
  int pod_end = (pod_start + rows_within_pod) > V ? V : (pod_start + rows_within_pod);
  int length = pod_end - pod_start;
  align_range(length, &start, &end);
  register float zero = ((float) 0);
  int cache_total = CACHE_LINE * bsg_tiles_X * bsg_tiles_Y;
  start = start + pod_start;
  pr_dbg("update vertex start: %i, end: %i, id: %i\n", start, end, bsg_id);
  for (; start < pod_end; start = start + cache_total) {
    end = start + CACHE_LINE;
    end = end > pod_end ? pod_end : end;
    int d = start;
    for(; d <= end - 8; d = d + 8) {
      register float tp_new0 = new_rank[d];
      register float tp_new1 = new_rank[d+1];
      register float tp_new2 = new_rank[d+2];
      register float tp_new3 = new_rank[d+3];
      register float tp_new4 = new_rank[d+4];
      register float tp_new5 = new_rank[d+5];
      register float tp_new6 = new_rank[d+6];
      register float tp_new7 = new_rank[d+7];

      register float tp_old0 = old_rank[d];
      register float tp_old1 = old_rank[d+1];
      register float tp_old2 = old_rank[d+2];
      register float tp_old3 = old_rank[d+3];
      register float tp_old4 = old_rank[d+4];
      register float tp_old5 = old_rank[d+5];
      register float tp_old6 = old_rank[d+6];
      register float tp_old7 = old_rank[d+7];

      tp_new0 = beta_score + damp * tp_new0;
      tp_new1 = beta_score + damp * tp_new1;
      tp_new2 = beta_score + damp * tp_new2;
      tp_new3 = beta_score + damp * tp_new3;
      tp_new4 = beta_score + damp * tp_new4;
      tp_new5 = beta_score + damp * tp_new5;
      tp_new6 = beta_score + damp * tp_new6;
      tp_new7 = beta_score + damp * tp_new7;

      error[d] = fabs(tp_new0 - tp_old0);
      error[d+1] = fabs(tp_new1 - tp_old1);
      error[d+2] = fabs(tp_new2 - tp_old2);
      error[d+3] = fabs(tp_new3 - tp_old3);
      error[d+4] = fabs(tp_new4 - tp_old4);
      error[d+5] = fabs(tp_new5 - tp_old5);
      error[d+6] = fabs(tp_new6 - tp_old6);
      error[d+7] = fabs(tp_new7 - tp_old7);

      old_rank[d] = tp_new0;
      old_rank[d+1] = tp_new1;
      old_rank[d+2] = tp_new2;
      old_rank[d+3] = tp_new3;
      old_rank[d+4] = tp_new4;
      old_rank[d+5] = tp_new5;
      old_rank[d+6] = tp_new6;
      old_rank[d+7] = tp_new7;

      new_rank[d] = zero;
      new_rank[d+1] = zero;
      new_rank[d+2] = zero;
      new_rank[d+3] = zero;
      new_rank[d+4] = zero;
      new_rank[d+5] = zero;
      new_rank[d+6] = zero;
      new_rank[d+7] = zero;   
    }
    for(; d < end; d++) {
      register float tp_new = new_rank[d];
      register float tp_old = old_rank[d];
      tp_new = beta_score + damp * tp_new;
      error[d] = fabs(tp_new - tp_old);
      old_rank[d] = tp_new;
      new_rank[d] = zero;
    }
  }
  bsg_cuda_print_stat_end(15);
  bsg_barrier_hw_tile_group_sync();
  return 0;
}