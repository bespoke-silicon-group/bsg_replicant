#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_cuda_lite_barrier.h>
#include "bs.hpp"

extern "C" int kernel_black_scholes(OptionData bsg_attr_remote * bsg_attr_noalias data,
                                    float bsg_attr_remote * bsg_attr_noalias puts,
                                    float bsg_attr_remote * bsg_attr_noalias calls,
                                    int elems){
        int start = __bsg_id * elems;
        bsg_barrier_hw_tile_group_init();
        bsg_barrier_hw_tile_group_sync();
        bsg_cuda_print_stat_kernel_start();
        //bsg_fence();
        // Use chunk size to load a cache line
#define CHUNK_SIZE 2
        OptionData _data[CHUNK_SIZE];
        float _puts[CHUNK_SIZE];
        float _calls[CHUNK_SIZE];
        for(int i = start; i < (start + elems); i += CHUNK_SIZE){

                for(int ci = 0; ci < CHUNK_SIZE; ++ci){
                        _data[ci] = data[i + ci];
                }

                for(int ci = 0; ci < CHUNK_SIZE; ++ci){
                        BlkSchlsEqEuroNoDiv(_data[ci].s, _data[ci].strike,
                                            _data[ci].r, _data[ci].v, _data[ci].t,
                                            _puts[ci], _calls[ci], 0);
                }
                for(int ci = 0; ci < CHUNK_SIZE; ++ci){
                        puts[i + ci] = _puts[ci];
                        calls[i + ci] = _calls[ci];
                }
        }
        bsg_cuda_print_stat_kernel_end();
        bsg_fence();
        bsg_barrier_hw_tile_group_sync();
        return 0;
}
