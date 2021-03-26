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
                }
        }
        return 0;
}
template <typename T>
__attribute__ ((noinline))
int transpose (const T *A,  T *B,
               uint32_t HEIGHT, uint32_t WIDTH,
               uint32_t BLOCK_Y, uint32_t BLOCK_X) {

        int start_y = BLOCK_Y * __bsg_y;
        int start_x = BLOCK_X * __bsg_x;
        for(int y_i = start_y; y_i < start_y + BLOCK_Y; ++y_i) {
                for(int x_i = start_x; x_i < start_x + BLOCK_X; ++x_i) {
                        uint32_t idx = WIDTH * y_i + x_i;
                        uint32_t idx_T = HEIGHT * x_i + y_i;
                        B[idx_T] = A[idx];
                }
        }
        return 0;
}


extern "C"
int kernel_profiler(float *A, float *B, float *C,
                    uint32_t HEIGHT, uint32_t WIDTH,
                    uint32_t BLOCK_Y, uint32_t BLOCK_X) {
        int rc;
        auto add = [](const float A, const float B){return (A + B);};
        auto sub = [](const float A, const float B){return (A - B);};
        auto mul = [](const float A, const float B){return (A * B);};
        bsg_cuda_print_stat_kernel_start();
        bsg_cuda_print_stat_start(0);
        rc = function(add, A, B, C, HEIGHT, WIDTH, BLOCK_Y, BLOCK_X);
        bsg_cuda_print_stat_start(1);
        barrier.sync();
        bsg_cuda_print_stat_end(1);
        rc = transpose(C, A, HEIGHT, WIDTH, BLOCK_Y, BLOCK_X);
        barrier.sync();
        bsg_cuda_print_stat_end(0);
        bsg_cuda_print_stat_start(15);
        bsg_cuda_print_stat_start(0);
        rc = function(sub, A, B, C, HEIGHT, WIDTH, BLOCK_Y, BLOCK_X);
        bsg_cuda_print_stat_end(0);
        rc = function(mul, B, C, A, HEIGHT, WIDTH, BLOCK_Y, BLOCK_X);
        bsg_cuda_print_stat_end(15);
        barrier.sync();
        rc = transpose(A, C, HEIGHT, WIDTH, BLOCK_Y, BLOCK_X);
        barrier.sync();
        bsg_cuda_print_stat_kernel_end();
        return rc;
}
