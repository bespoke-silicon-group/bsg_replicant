#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_cuda_lite_barrier.h>
#include "bs.hpp"

#define TILE_GROUP_SIZE (TILE_GROUP_DIM_X * TILE_GROUP_DIM_Y)

extern "C" int kernel_black_scholes(OptionData bsg_attr_remote * bsg_attr_noalias data,
                                    float bsg_attr_remote * bsg_attr_noalias puts,
                                    float bsg_attr_remote * bsg_attr_noalias calls,
                                    int elems){
#define CHUNK_SIZE 2
        int start = __bsg_id * CHUNK_SIZE;
        bsg_barrier_hw_tile_group_init();
        bsg_barrier_hw_tile_group_sync();
        // Prefetch first option
        asm("lw x0, %0": : "m" (data[start]));
        asm("lw x0, %0": : "m" (puts[start]));
        asm("lw x0, %0": : "m" (calls[start]));
        asm volatile("" ::: "memory");
        bsg_fence();
        bsg_cuda_print_stat_kernel_start();
        //bsg_fence();
        // Use chunk size to load a cache line
        OptionData _data[CHUNK_SIZE];
        float _puts[CHUNK_SIZE];
        float _calls[CHUNK_SIZE];
        for(int i = 0; i < elems; i += CHUNK_SIZE){
                for(int ci = 0; ci < CHUNK_SIZE; ++ci){
                        _data[ci].t = data[start + i * TILE_GROUP_SIZE + ci].t;
                        _data[ci].s = data[start + i * TILE_GROUP_SIZE + ci].s;
                        _data[ci].strike = data[start + i * TILE_GROUP_SIZE + ci].strike;
                        _data[ci].r = data[start + i * TILE_GROUP_SIZE + ci].r;
                        _data[ci].v = data[start + i * TILE_GROUP_SIZE + ci].v;
                }

                // Prefetch to cache
                asm("lw x0, %0": : "m" (data[start + i * TILE_GROUP_SIZE + CHUNK_SIZE]));
                asm("lw x0, %0": : "m" (puts[start + i * TILE_GROUP_SIZE]));
                asm("lw x0, %0": : "m" (calls[start + i * TILE_GROUP_SIZE]));
                asm volatile("" ::: "memory");
                
                for(int ci = 0; ci < CHUNK_SIZE; ++ci){
                        BlkSchlsEqEuroNoDiv(_data[ci].s, _data[ci].strike,
                                            _data[ci].r, _data[ci].v, _data[ci].t,
                                            _puts[ci], _calls[ci], 0);
                }

                for(int ci = 0; ci < CHUNK_SIZE; ++ci){
                        puts[start + i * TILE_GROUP_SIZE + ci] = _puts[ci];
                        calls[start + i * TILE_GROUP_SIZE + ci] = _calls[ci];
                }
        }
        bsg_cuda_print_stat_kernel_end();
        bsg_fence();
        bsg_barrier_hw_tile_group_sync();
        return 0;
}
