#include <bsg_manycore.h>
#include <bsg_set_tile_x_y.h>
#include <cstdint>
#include <cstring>
#include <bsg_tile_group_barrier.hpp>

bsg_barrier<bsg_tiles_X, bsg_tiles_Y> barrier;


template <typename T, typename F>
__attribute__ ((noinline)) 
int function (F OP, const T *A, const T *B, T *C,
              uint32_t HEIGHT, uint32_t WIDTH,
              uint32_t BLOCK_Y, uint32_t BLOCK_X) {
              
	int start_y = BLOCK_Y * __bsg_y;
	int start_x = BLOCK_X * __bsg_x;
        for(int y_i = start_y; y_i < start_y + BLOCK_Y; ++y_i) {
                for(int x_i = start_x; x_i < start_x + BLOCK_X; ++x_i) {
                        uint32_t idx = WIDTH * y_i + x_i;
                        C[idx] = OP(A[idx], B[idx]);
                        bsg_print_float(A[idx]);
                        bsg_print_float(B[idx]);
                        bsg_print_float(C[idx]);
                }
        }
        return 0;
}

extern "C"
int kernel_profiler(float *A, float *B, float *C,
                    uint32_t HEIGHT, uint32_t WIDTH,
                    uint32_t BLOCK_Y, uint32_t BLOCK_X) {
        int rc;
        bsg_print_stat_kernel_start();
        auto add = [](const float A, const float B){return (A + B);};
        rc = function(add, A, B, C, HEIGHT, WIDTH, BLOCK_Y, BLOCK_X);
        bsg_print_stat_kernel_end();
	barrier.sync();

        return rc;
}
