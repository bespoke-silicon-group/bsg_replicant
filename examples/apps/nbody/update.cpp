#include <bsg_manycore.h>
#include <bsg_cuda_lite_barrier.h>

#include <Body.hpp>
#include <Config.hpp>


// HBBodies should be attr_remote, but LLVM complains about the address space cast.
extern "C" void update(Config *pcfg, int nBodies, HBBody *HBBodies){

        // Copy frequently used data to local
        Config cfg = *pcfg;

        bsg_barrier_hw_tile_group_init();
        bsg_barrier_hw_tile_group_sync();
        bsg_cuda_print_stat_kernel_start();
	for (int cur = __bsg_id; cur < nBodies; cur += TILE_GROUP_DIM_X * TILE_GROUP_DIM_Y) {
                HBBody b = HBBodies[cur];
                Point dvel(b.acc);
                Point velh(b.vel);
                dvel *= cfg.dthf;
                velh += dvel;
                b.pos += velh * cfg.dtime;
                b.vel = velh + dvel;
                HBBodies[cur] = b;
	}        
        bsg_barrier_hw_tile_group_sync();
        bsg_cuda_print_stat_kernel_end();
        bsg_fence();
        bsg_barrier_hw_tile_group_sync();
        return;
}
