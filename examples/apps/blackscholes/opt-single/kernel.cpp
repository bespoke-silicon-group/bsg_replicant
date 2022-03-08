#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_tile_group_barrier.hpp>
#include "bs.hpp"
#include <string.h>

extern "C"  int kernel_black_scholes(OptionData bsg_attr_remote * bsg_attr_noalias data,
                                    float bsg_attr_remote * bsg_attr_noalias prices,
                                    int elems){
	int grid_idx = (__bsg_tile_group_id_y * __bsg_grid_dim_x + __bsg_tile_group_id_x);
        int start = grid_idx * elems;
        bsg_cuda_print_stat_kernel_start();
        //bsg_fence();
#define CHUNK_SIZE 4
        OptionData _data[CHUNK_SIZE];
        float _prices[CHUNK_SIZE];

        for(int i = start; i < (start + elems); i += CHUNK_SIZE){

                for(int ci = 0; ci < CHUNK_SIZE; ++ci){
                        _data[ci] = data[i + ci];
                }

                for(int ci = 0; ci < CHUNK_SIZE; ++ci){
                        _prices[ci] = BlkSchlsEqEuroNoDiv(_data[ci].s, _data[ci].strike,
                                                          _data[ci].r, _data[ci].v, _data[ci].t,
                                                          _data[ci].OptionType, 0);
                }
                for(int ci = 0; ci < CHUNK_SIZE; ++ci){
                        prices[i + ci] = _prices[ci];
                }
        }

        bsg_cuda_print_stat_kernel_end();
        return 0;
}
