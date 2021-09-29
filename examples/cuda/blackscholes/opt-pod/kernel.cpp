#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_tile_group_barrier.hpp>
#include "bs.hpp"

extern "C" int kernel_black_scholes(OptionData bsg_attr_remote * bsg_attr_noalias data,
                                    float bsg_attr_remote * bsg_attr_noalias puts,
                                    float bsg_attr_remote * bsg_attr_noalias calls,
                                    int elems){
	int grid_idx = (__bsg_tile_group_id_y * __bsg_grid_dim_x + __bsg_tile_group_id_x);
        int start = grid_idx * elems;
        bsg_cuda_print_stat_kernel_start();
        //bsg_fence();
#define CHUNK_SIZE 4
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
        return 0;
}
