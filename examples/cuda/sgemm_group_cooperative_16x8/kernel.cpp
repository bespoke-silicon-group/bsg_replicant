// BSG_TILE_GROUP_X_DIM and BSG_TILE_GROUP_Y_DIM must be defined
// before bsg_manycore.h and bsg_cuda_lite_barrier.h are
// included.
#define BSG_TILE_GROUP_X_DIM bsg_tiles_X
#define BSG_TILE_GROUP_Y_DIM bsg_tiles_Y
#include <bsg_manycore.h>
#include <bsg_cuda_lite_barrier.h>
#include <hb_tensor.hpp>
#include <cstring>
#include <cstdint>
#include <math.h>
//#include <bsg_group_strider.hpp>
#include <bsg_hw_barrier.h>
#include <bsg_hw_barrier_config_init.h>


// NB: This is an interesting opportunity for optimization. Dual-loop
// unrolling could allow loads to be spread across caches. Ideally the
// inner loop is unrolled completely first (?), then the outer loop is
// unrolled until maximum NB Loads are achieved.

// Locations:
//     src should be in remote memory (DRAM)
//     dest should be in local scratchpad
template <unsigned int BY, unsigned int BX, bool TRANSPOSE>
inline void load_block(float * bsg_attr_noalias dest,
                float bsg_attr_remote * bsg_attr_noalias src,
                uint32_t * src_strides,
                int by_i, int bx_i)
#ifdef __clang__
__attribute__((no_builtin("memcpy", "memset")))
#endif
{

        // Move the raw pointer to the row/column start.
        src = src +
                (by_i * BY * src_strides[0]) +
                (bx_i * BX * src_strides[1]);

        //bsg_unroll(2)
        for (int i = 0; i < BY; i++) {
                // If the unroll factor > B, it will unroll by factor B
                // instead.
                bsg_unroll(16)
                for (int j = 0 ; j < BX; j ++){
                        if (!TRANSPOSE)
                                dest[BX * i + j] = src[i * src_strides[0] + j];
                        else
                                dest[i + BY * j] = src[i * src_strides[0] + j];
                }
        }
}

template <unsigned int BY, unsigned int BX>
inline void prefetch(float bsg_attr_remote * bsg_attr_noalias p,
                     uint32_t * strides, int by_i, int bx_i){
        // Move the raw pointer to the row/column start.
        p = p +
                (by_i * BY * strides[0]) +
                (bx_i * BX * strides[1]);

        for (int i = 0; i < BY; i++) {
                asm("lw x0, %0": : "m" (p[i * strides[0]]));
        }
        //        bsg_fence();
}

// Store the result (psum) to remote memory and reset the psum array
// in scratchpad.
// Locations:
//     src should be in local scratchpad
//     dest should be in remote memory (DRAM)
template <unsigned int BY, unsigned int BX>
inline void store_block_and_reset(float * bsg_attr_noalias src,
                           float bsg_attr_remote * bsg_attr_noalias dest,
                           uint32_t * dest_strides,
                           int by_i, int bx_i)
#ifdef __clang__
__attribute__((no_builtin("memcpy", "memset")))
#endif
{

        // Move the raw pointer to the row/column start.
        dest = dest +
                (by_i * BY * dest_strides[0]) +
                (bx_i * BX * dest_strides[1]);

        // Store from the source matrix, into the block.
        // TODO: In THEORY this can be more optimal. We should do
        // stores and zeros at the same time by issuing all stores,
        // then issuing all zeros, so that we have all available
        // credits by the time we load.
        for (int i = 0; i < BY; i++) {
                bsg_unroll(16)
                for (int j = 0 ; j < BX; j ++){
                        dest[i * dest_strides[0] + j] = src[i * BX + j];
                        src[i * BX + j] = 0.0f;
                }
        }
}

// Accumulate the product of two BY-by-BX input matrices into an
// output matrix.
//
// This is done by iteratively computing SBY-by-SBX sub-matrix
// outputs, and individually accumulating those into the output
// matrix.
// Locations:
//     dest should be in local scratchpad
//     mat1 should be in group scratchpad
//     mat2 should be in group scratchpad
template<unsigned int BY, unsigned int SBY, unsigned int BX, unsigned int SBX, unsigned int SBZ,bool M1_TRANSPOSE>
inline void accum_row(float* bsg_attr_noalias dest,
                      float* block_row,
                      float* block_col,                
                      int bw_i
) {

        static_assert((BX % SBX) == 0, "X Block-Dimension must be a multiple of the X Sub-Block Dimension");
        static_assert((BY % SBY) == 0, "Y Block-Dimension must be a multiple of the Y Sub-Block Dimension");


        for (int by_i = 0; by_i < BY/SBY; ++by_i) {
          for (int bx_i = 0; bx_i < BX/SBX; ++bx_i) {

            int sb_anchor_y = (by_i * SBY);
            int sb_anchor_x = (bx_i * SBX);
            // partial sum accumulator in regfile
            float psum[SBY][SBX];
            // pointer to small block in dest
            float * bsg_attr_noalias sb_anchor = &(dest[sb_anchor_y * BX + sb_anchor_x]);
            
            // load the small block to regfile
            bsg_unroll(16)
            for (int sby_i = 0; sby_i < SBY; ++sby_i){
              bsg_unroll(16)
              for(int sbx_i = 0; sbx_i < SBX; ++sbx_i){
                psum[sby_i][sbx_i] = sb_anchor[(sby_i * BX) + sbx_i];
              }
            }

            // compute
            for (int b_i = 0; b_i < BSG_TILE_GROUP_Y_DIM; b_i++) {
              // mat1 y,x
              int mat1_y = __bsg_y;
              int mat1_x = (BSG_TILE_GROUP_Y_DIM*bw_i) + ((__bsg_y+b_i)%BSG_TILE_GROUP_Y_DIM); 
              // mat2 y,x
              int mat2_y = (__bsg_y + b_i) % BSG_TILE_GROUP_Y_DIM;
              int mat2_x = __bsg_x;
              // tile-group ptr
              float *mat1 = (float*) (
                (1<<29) |
                (mat1_y << 24) |
                (mat1_x << 18) |
                (int) block_row
              );
              float *mat2 = (float*) (
                (1<<29) |
                (mat2_y << 24) |
                (mat2_x << 18) |
                (int) block_col
              );
                
              for (int sbx_i = 0; sbx_i < BX; sbx_i += SBZ) {
                float col[SBZ][SBY];  // in regfile
                float row[SBZ][SBX];  // in regfile

                bsg_unroll(2)
                for (int j = 0; j < SBZ; ++j) {
                  float * bsg_attr_noalias col_anchor = &(mat1[(sb_anchor_y * BX) + sbx_i + j]);
                  float * bsg_attr_noalias row_anchor = &(mat2[((sbx_i + j) * BX) + sb_anchor_x]);
                  // load values to register
                  bsg_unroll(4)
                  for (int i = 0; i < SBX; ++i) {
                    col[j][i] = col_anchor[i*BX];
                  }
                  bsg_unroll(4)
                  for (int i = 0; i < SBX; ++i) {
                    row[j][i] = row_anchor[i];
                  }
                }
                
                // perform vector multiply accumulate
                bsg_unroll(2)
                for(int j = 0; j< SBZ; ++j){
                  bsg_unroll(16)
                  for(int y = 0; y < SBY; ++y){
                    bsg_unroll(16)
                    for(int x = 0; x < SBX; ++x){
                      psum[y][x] = fmaf(col[j][y], row[j][x], psum[y][x]);
                    }
                  }
                }

              }
            }

            // Write the partial sum sub-block back into
            // the result.
            bsg_unroll(16)
            for(int sby_i = 0; sby_i < SBY; ++sby_i){
              bsg_unroll(16)
              for(int sbx_i = 0; sbx_i < SBX; ++sbx_i){
                sb_anchor[sby_i * BX + sbx_i] = psum[sby_i][sbx_i];
              }
            }

          }
        }

}

// BX is the X-dimension of the sub-block
// BY is the Y-dimension of the sub-block
// _mat1 is a r1 x c1 matrix
// _mat2 is a r2 x c2 matrix
// c1 == r2
// c1 % BX == 0, r2 % BX == 0
// c2 % BY == 0, r1 % BY == 0
template<unsigned int BX, unsigned int BY, bool LOAD_M1_TRANSPOSED, bool PROFILE>
inline int kernel_mm_opt(float bsg_attr_remote * bsg_attr_noalias result,
                  uint32_t *bsg_attr_noalias result_strides,
                  float bsg_attr_remote * bsg_attr_noalias mat1,
                  uint32_t * bsg_attr_noalias mat1_strides,
                  int r1, int c1,
                  float bsg_attr_remote * bsg_attr_noalias mat2,
                  uint32_t * bsg_attr_noalias mat2_strides,
                  int r2, int c2
                  )
// clang doesn't like using memcpy/memset on bsg_attr_remote, so
// disable them.
#ifdef __clang__
__attribute__((no_builtin("memcpy", "memset")))
#endif
{
        int bx_blocks = c2 / (BSG_TILE_GROUP_X_DIM * BX);
        int by_blocks = r1 / (BSG_TILE_GROUP_Y_DIM * BY);
        int bz_blocks = c1 / (BSG_TILE_GROUP_X_DIM * BX);
        int bw_blocks = BSG_TILE_GROUP_X_DIM / BSG_TILE_GROUP_Y_DIM;

        int by_stride = __bsg_grid_dim_y; // 1
        int bx_stride = __bsg_grid_dim_x; // 1
        int bz_stride = 1;
        int bw_stride = 1;
                
        // Local Storage for input/output blocks.
        float block_row[BY * BX]; // mat1
        float block_col[BX * BY]; // mat2
        float block_out[BY * BX];

        // Initialize local output to 0.0f
        for (int i = 0; i < BY; i++) {
          bsg_unroll(16) 
          for (int j = 0 ; j < BX; j ++){
            block_out[i * BX + j] = 0.0f;
          }
        }

        // Start profiling
        bsg_barrier_hw_tile_group_init();
        bsg_fence();
        bsg_barrier_hw_tile_group_sync();
        bsg_cuda_print_stat_kernel_start();

        //bsg_print_int(__bsg_id);

        block_y_loop:
        for (int by_i = 0; by_i < by_blocks; by_i++) {
          block_x_loop:
          for (int bx_i = 0; bx_i < bx_blocks; bx_i++) {
            block_z_loop:
            for (int bz_i = 0; bz_i < bz_blocks; bz_i++) {

              // load mat1
              load_block<BY,BX,false>(block_row, mat1, mat1_strides,
                (by_i*BSG_TILE_GROUP_Y_DIM)+__bsg_y,
                (bz_i*BSG_TILE_GROUP_X_DIM)+__bsg_x
              );

              block_w_loop:
              for (int bw_i = 0; bw_i < bw_blocks; bw_i++) {
                //if (__bsg_id == 0) bsg_print_int(bw_i);
                // load mat2
                load_block<BY,BX,false>(block_col, mat2, mat2_strides,
                  (bz_i*bw_blocks*BSG_TILE_GROUP_Y_DIM) + (bw_i*BSG_TILE_GROUP_Y_DIM) +__bsg_y, 
                  (bx_i*BSG_TILE_GROUP_X_DIM) + __bsg_x
                ); 
                bsg_barrier_hw_tile_group_sync();
                //if (__bsg_id == 0) bsg_print_int(bw_i);
                // compute
                accum_row<BY,4,BX,4,2,false>(block_out, block_row, block_col, bw_i);
                //if (__bsg_id == 0) bsg_print_int(bw_i);
                bsg_barrier_hw_tile_group_sync();
                //if (__bsg_id == 0) bsg_print_int(bw_i);
              }
            }
            // prefetch
            prefetch<BY, BX>(result, result_strides, by_i * BSG_TILE_GROUP_Y_DIM + __bsg_y, bx_i * BSG_TILE_GROUP_X_DIM + __bsg_x);
            // store_block_and_reset
            store_block_and_reset<BY, BX>(block_out, result, result_strides, by_i * BSG_TILE_GROUP_Y_DIM + __bsg_y, bx_i * BSG_TILE_GROUP_X_DIM + __bsg_x);
          }
        }

        // End profiling
        //bsg_barrier_hw_tile_group_sync();
        bsg_cuda_print_stat_kernel_end();
        bsg_fence();
        bsg_barrier_hw_tile_group_sync();

        return 0;
}

extern "C"
int kernel_mm_opt(
                  hb_tensor_t* _result,
                  hb_tensor_t* _mat1,
                  hb_tensor_t* _mat2) {


        auto mat1 = HBTensor<float, 2>(_mat1);
        auto mat2 = HBTensor<float, 2>(_mat2);
        auto result = HBTensor<float, 2>(_result);
        
        kernel_mm_opt<BLOCK_DIM,BLOCK_DIM, false, false>((float bsg_attr_remote * bsg_attr_noalias) result.data_ptr(),
                                        result.get_strides(),
                                        (float bsg_attr_remote * bsg_attr_noalias) mat1.data_ptr(),
                                        mat1.get_strides(),
                                        mat1.dim(0), mat1.dim(1),
                                        (float bsg_attr_remote * bsg_attr_noalias) mat2.data_ptr(),
                                        mat2.get_strides(),
                                        mat2.dim(0), mat2.dim(1));
        return 0;
}

