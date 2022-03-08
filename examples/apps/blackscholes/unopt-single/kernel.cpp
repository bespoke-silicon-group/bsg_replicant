#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <bsg_tile_group_barrier.hpp>
#include "bs.hpp"

extern "C" int kernel_black_scholes(float bsg_attr_remote * bsg_attr_noalias sptprice,
                                    float bsg_attr_remote * bsg_attr_noalias strike,
                                    float bsg_attr_remote * bsg_attr_noalias rate,
                                    float bsg_attr_remote * bsg_attr_noalias volatility,
                                    float bsg_attr_remote * bsg_attr_noalias time,
                                    int bsg_attr_remote * bsg_attr_noalias otype,
                                    float bsg_attr_remote * bsg_attr_noalias prices,
                                    int elems){
	int grid_idx = (__bsg_tile_group_id_y * __bsg_grid_dim_x + __bsg_tile_group_id_x);
        int start = grid_idx * elems;
        bsg_cuda_print_stat_kernel_start();
        
        for(int i = start; i < (start + elems); ++i){
                prices[i] = BlkSchlsEqEuroNoDiv(sptprice[i], strike[i],
                                                rate[i], volatility[i], time[i], 
                                                otype[i], 0);
        }

        bsg_cuda_print_stat_kernel_end();
        return 0;
}
