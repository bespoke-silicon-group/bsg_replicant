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
#include <bsg_group_strider.hpp>

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
                      bsg_tile_group_strider<BSG_TILE_GROUP_X_DIM, 1, BSG_TILE_GROUP_Y_DIM, 0, float> &prow,
                      bsg_tile_group_strider<BSG_TILE_GROUP_X_DIM, 0, BSG_TILE_GROUP_Y_DIM, 1, float> &pcol
                      ) {

        static_assert((BX % SBX) == 0, "X Block-Dimension must be a multiple of the X Sub-Block Dimension");
        static_assert((BY % SBY) == 0, "Y Block-Dimension must be a multiple of the Y Sub-Block Dimension");
        // Iterate through the SBY-by-SBX sub-blocks in the BY-by-BX block.
        for (int by_i = 0; by_i < BY/SBY; ++by_i) {
                for (int bx_i = 0; bx_i < BX/SBX; ++bx_i) {

                        // Compute the y,x location of the sub-block corner
                        int sb_anchor_y = (by_i * SBY);
                        int sb_anchor_x = (bx_i * SBX);

                        // Load in a SBY-by-SBX sub-block of the
                        // output matrix into psum for accumulation.
                        // Location: Registers (for SBX == 4, SBY == 4)
                        float psum[SBY][SBX];

                        // The sub-block is "anchored" by the
                        // upper-right corner at by_i, bx_i
                        // Location: (pointer to) Scratchpad
                        float * bsg_attr_noalias sb_anchor = &(dest[sb_anchor_y * BX + sb_anchor_x]);

                        bsg_unroll(16)
                        for(int sby_i = 0; sby_i < SBY; ++sby_i){
                                bsg_unroll(16)
                                for(int sbx_i = 0; sbx_i < SBX; ++sbx_i){
                                        psum[sby_i][sbx_i] = sb_anchor[sby_i * BX + sbx_i];
                                }
                        }

                        for(int b_i = 0; b_i < BSG_TILE_GROUP_X_DIM; b_i ++){
                                // Compute an SBY-by-SBX output sub-block by
                                // performing BX, SBY-by-1 x 1-by-SBX
                                // vector-vector multiplies, and accumulate
                                // with the result
                                float *mat1 = prow.ptr;
                                float *mat2 = pcol.ptr;
                                for(int sbx_i = 0; sbx_i < BX; sbx_i += SBZ) {
                                        // Load an SBY-by-SBZ sub-column of mat1,
                                        // SBZ-by-SBX sub-row of mat2, and perform a
                                        // SBY-by-SBZ x SBZ-by-SBX vector-vector multiply
                                        // that produces an SBY-by-SBX output matrix.

                                        // *** SBX = 4, SBY = 4, SBZ = 2 USES 32 REGISTERS ***

                                        // Locations:
                                        //     col should be in registers (for SBY == 4)
                                        //     row should be in registers (for SBX == 4)
                                        float col[SBZ][SBY];
                                        float row[SBZ][SBX];
                                        bsg_unroll(2)
                                        for(int j = 0; j < SBZ; ++j){
                                                // Load a column of m1, and a row of m2.
                                                float * bsg_attr_noalias col_anchor;
                                                if (!M1_TRANSPOSE) {
                                                        // Location: (pointer to) Scratchpad
                                                        col_anchor = &(mat1[sb_anchor_y * BX + sbx_i + j]);
                                                } else {
                                                        // Location: (pointer to) Scratchpad
                                                        col_anchor = &(mat1[sb_anchor_y + (sbx_i + j) * BY]);
                                                }

                                
                                                // Load an SBX-by-1 sub-column of mat2
                                                // If M1 is transposed, then adjust
                                                // the indexing to row major.
                                                // Transposing can improve performance
                                                // by using using immediates instead
                                                // of a register offset.
                                                float * bsg_attr_noalias row_anchor = &(mat2[(sbx_i + j) * BY + sb_anchor_x]);
                                                bsg_unroll(16)
                                                for(int i = 0; i < SBX; ++i){
                                                        if (!M1_TRANSPOSE) {
                                                                col[j][i] = col_anchor[i * BX];
                                                        } else {
                                                                col[j][i] = col_anchor[i];
                                                        }
                                                }
                                                bsg_unroll(16)
                                                for(int i = 0; i < SBX; ++i){
                                                        row[j][i] = row_anchor[i];
                                                }
                                        }


                                        // Perform a SBY-by-1 x 1-by-SBX
                                        // vector-vector multiply to produce
                                        // an SBY-by-SBX output matrix

                                        // Add the result to the partial sum
                                        // This could be done in two steps,
                                        // but we do it in one to use FMA
                                        // instructions

                                        // The code expects that psum, col,
                                        // and row are all allocated in
                                        // registers so that there are SBY *
                                        // SBX fused-multiply-add instructions
                                        // in a row.
                                        bsg_unroll(2)
                                        for(int j = 0; j< SBZ; ++j){
                                                bsg_unroll(16)
                                                for(int sby_i = 0; sby_i < SBY; ++sby_i){
                                                        bsg_unroll(16)
                                                        for(int sbx_i = 0; sbx_i < SBX; ++sbx_i){
                                                                psum[sby_i][sbx_i] = fmaf(col[j][sby_i], row[j][sbx_i], psum[sby_i][sbx_i]);
                                                        }
                                                }
                                        }
                                }
                                pcol.stride();
                                prow.stride();
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

        // TODO: The compiler doesn't know that c1 is always (or
        // should always be) nonzero. This adds an extra BNE
        // instruction in the inner-loop. There should be some way to
        // signal to the compiler, perhaps with hb_assert?

        // Algorithm: To maximize locality, each tile should compute a
        // number of BX-by-BY output blocks by computing on blocks of
        // input data. In this implementation, the tile cooperates
        // with a group of nearby tiles to increase data locality and
        // reuse.

        // A single tile strides between output blocks in row-major
        // order. For this implementation, the stride between output
        // blocks is the grid dimension, times the tile group dimension.

        // TODO: Allocate tile groups in row-major order.

        int bx_blocks = c2 / (BSG_TILE_GROUP_X_DIM * BX);
        int by_blocks = r1 / (BSG_TILE_GROUP_Y_DIM * BY);
        int bz_blocks = c1 / (BSG_TILE_GROUP_X_DIM * BX); // r2 / BX

        int by_stride = __bsg_grid_dim_y;
        int bx_stride = __bsg_grid_dim_x;
        int bz_stride = 1;
                
        // Local Storage for input/output blocks. Output blocks are
        // sometimes called partial sums, too.
        // Locations:
        //    block_row is in Local Scratchpad
        //    block_col is in Local Scratchpad
        //    block_out is in Local Scratchpad
        float block_row[BY * BX];
        float block_col[BX * BY];
        float block_out[BY * BX];

        // Initialize local output to 0.0f
        for (int i = 0; i < BY; i++) {
                // Unroll by 16 because it is the maximum block size a
                // tile can handle. Any smaller value will be
                // completely unrolled
                bsg_unroll(16) 
                for (int j = 0 ; j < BX; j ++){
                        block_out[i * BX + j] = 0.0f;
                }
        }

        // Note: This only works for N x N tile groups where N is a
        // power of two. To put it in the symbols below:

        // log2(BSG_TILE_GROUP_X_DIM) == log2(BSG_TILE_GROUP_Y_DIM) == 0.

        // Scale-up, cooperative, matrix multiply: using groups of
        // size BSG_TILE_GROUP_X_DIM x BSG_TILE_GROUP_Y_DIM tiles,
        // compute matrix multiply of matrices m1 (r1 x c1), and m2
        // (r2 x c2)

        // The number of tile groups launched is defined by the grid
        // dimension: __bsg_grid_dim_x/y
        
        // Each group is uniquely defined by its tile group X/Y
        // index, __bsg_tile_group_id_x/y

        // 0 < __bsg_tile_group_id_x < __bsg_grid_dim_x
        // 0 < __bsg_tile_group_id_x < __bsg_grid_dim_Y

        // Logically, split the output matrix into hierarchical blocks:

        // - TBX x TBY group blocks that a group computes, composed of:
        // -  BX x  BY tile blocks that a single tile computes

        // Where TBX = BSG_TILE_GROUP_X_DIM * BX and TBY = BSG_TILE_GROUP_Y_DIM * BY

        // This means that there are r1 / TBY "row" blocks in the
        // output, and c2 / TBX "column" blocks in the output
        
        // Each group will iterate through unprocessed output group
        // blocks in the X and Y dimension, computing the group block
        // at coordinate tx_i, ty_i.
        //
        // The initial value for tx_i i __bsg_tile_group_id_x, and
        // ty_i is __bsg_tile_group_id_y. After computing a TBX x TBY
        // output block, the tile group moves to the next output block
        // that is __bsg_grid_dim_x, and then __bsg_grid_dim_y blocks
        // away.

        // From the group perspective, the row-major MM algorithm with
        // striding looks like this:

        // block_y_loop:
        // for(ty_i = __bsg_tile_group_id_x; ty_i < r1 / TBY; ty_i += __bsg_grid_dim_y)
        //     block_x_loop:
        //     for(tx_i = __bsg_tile_group_id_x; tx_i < c2 / TBX; tx_i += __bsg_grid_dim_x)
        //         Initialize block_out to 0, a TBX x TBY block
        //         block_z_loop:
        //         for(tz_i = 0; c1 / TBX; tz_i++)
        //             Load TBX x TBY block from m1 and TBY x TBX block from m2
        //             Perform multiply-accumulate into block_out
        //         Store block_out into output

        // This is the end of the high-level algorithm. It is critical
        // to understand the lines above before proceeding.

        // (Note it is possible to do a stride of 1, in which case,
        // the iteration limits are r1/(__bsg_grid_dim_y * TBY) and
        // c2/(__bsg_grid_dim_x * TBX).  We believe that striding by
        // the grid dimension produces more cache locality)

        // ******************** ********************

        // The following is a description of inner loop of the group.

        // In the "z" loop above, each tile collaborates with its
        // group to load a TBX x TBY from m1 and a TBY x TBX block
        // from m2 into their scratchpads. A multiply accumulation is
        // performed until there are no blocks in the row of m1 and
        // column of m2 remaining.

        // Each group computes a TBX x TBY block composed of
        // individual BX x BY blocks that are computed by individual
        // tiles.

        // A tile's x,y location within its group is defined by the
        // variables __bsg_X and __bsg_Y.

        // Each group compute computes an output group block. The
        // output group block location is defined by its "anchor", the
        // coordinate of the upper-left corner of the TBX x TBY block
        // in the output matrix. This will be the variable
        // tile_group_output_anchor_x/y (Shorthand: OTAX/OTAY):

        // OTAX = TBX * tx_i
        // OTAY = TBY * ty_i

        // The group block anchor coordinates for M1 and M2 are:

        // TM1AX = TBX * tz_i
        // TM1AY = TBY * ty_i

        // TM2AX = TBY * tx_i
        // TM2AY = TBX * tz_i

        // Each tile is responsible for loading a BX x BY block from
        // each of m1 and m2. Their tile block anchor coordinates are:

        // M1AX = TM1AX + __bsg_x * BX
        // M1AY = TM1AY + __bsg_y * BY
        // (Row-major order for M1)

        // M2AX = TM2AX + __bsg_y * BX
        // M2AY = TM2AY + __bsg_x * BY
        // (Column-major order for M2)

        // (The reason we do row major order for m1, and column major
        // order for m2 is so that tiles within a group only access
        // data from tiles within their row/column)

        // Using these anchors we can load the TBX x TBY block into
        // the group.

        // At this point all data is loaded into the scratchpads of
        // the tile group, so we can ignore global indexing for the
        // moment and just use tm1 and tM2, where tM* is the group block
        // in group memory. Each group now needs to update it's
        // partial sum using its own blocks and blocks from its group.
        // This is effectively the computation that is done in the
        // inner loop of the group:

        // for(y_i = 0; y_i < TBY; y_i ++){
        //    for(x_i = 0; x_i < TBX; x_i ++){
        ///      for(z_i = 0; z_i < TBX; z_i ++){
        //          block_out[y_i][x_i] += tM1[y_i][z_i] + tM2[z_i][x_i];

        // However, the computation and data is split across tiles
        // into BSG_TILE_GROUP_X_DIM x BSG_TILE_GROUP_Y_DIM pieces, where each tile computes a BX x BY
        // output block. We can rewrite the for loops like this: 

        // for(ty_i = 0 ; ty_i < BSG_TILE_GROUP_Y_DIM; ty_i ++){
        //    for(tx_i = 0 ; tx_i < BSG_TILE_GROUP_X_DIM; tx_i ++){
        //       for(y_i = 0; y_i < BY; y_i ++){
        //          for(x_i = 0; x_i < BX; x_i ++){
        //             for(z_i = 0; z_i < TBX; z_i ++){
        //                block_out[ty_i * BY + y_i][tx_i * BX + x_i] += tM1[ty_i * BY + y_i][z_i] + tM2[z_i][tx_i * BX + x_i];

        // If we assign each iteration of the outer two loops to tiles
        // we can replace ty_i and tx_i with the tile group variables
        // __bsg_x and __bsg_y and start to simplify this code:

        // for(y_i = 0; y_i < BY; y_i ++){
        //    for(x_i = 0; x_i < BX; x_i ++){
        //       for(z_i = 0; z_i < TBX; z_i ++){
        //          block_out[__bsg_y * BY + y_i][__bsg_x * BX + x_i] += tM1[__bsg_y * BY + y_i][z_i] + tM2[z_i][__bsg_y * BX + x_i];

        // This code does not account for the distribution of BX x BY
        // blocks across tiles. z_i will eventually access data on
        // another tile. Ideally, we would write something like the
        // code above code, but since we don't support striding across
        // tiles we have to handle it in software.

        // To start, let's split the z loop up so that it accesses one
        // block, then another:

        // for(y_i = 0; y_i < BY; y_i ++){
        //    for(x_i = 0; x_i < BX; x_i ++){
        //       for(b_i = 0; b_i < TBX/BX; b_i ++){
        //          for(z_i = 0; z_i < BX; z_i ++){
        //             block_out[__bsg_y * BY + y_i][__bsg_x * BX + x_i] += tM1[__bsg_y * BY + y_i][b_i * BX + z_i] + tM2[b_i * BX + z_i][__bsg_y * BX + x_i];

        // Let's reorder the loops so that the block access is the
        // outer most loop, and the inner loops do not cross between
        // blocks in tile scratchpads:

        // for(b_i = 0; b_i < TBX/BX; b_i ++){
        //    for(y_i = 0; y_i < BY; y_i ++){
        //       for(x_i = 0; x_i < BX; x_i ++){
        //          for(z_i = 0; z_i < BX; z_i ++){
        //             block_out[__bsg_y * BY + y_i][__bsg_x * BX + x_i] += tM1[__bsg_y * BY + y_i][b_i * BX + z_i] + tM2[b_i * BX + z_i][__bsg_y * BX + x_i];
        
        // As alluded to above, tM* is a group memory block in a
        // contiguous address space, but we don't have that
        // abstraction.

        // Instead, we have to handle jumping to BX x BY blocks in
        // neighboring scratchpads in software. We will abstract this
        // using pointers, where pM1 and pM2 are pointers to blocks of
        // data in individual scratchpads of tile group memory, and
        // lM1/lM2 are the arrays local to the current tile
        
        // pM1 = bsg_remote_ptr(lM1, __bsg_x, 0)
        // pM2 = bsg_remote_ptr(lM1, 0, __bsg_y)
        // for(b_i = 0; b_i < TBX/BX; b_i ++){
        //    for(y_i = 0; y_i < BY; y_i ++){
        //       for(x_i = 0; x_i < BX; x_i ++){
        //          for(z_i = 0; z_i < BX; z_i ++){
        //             block_out[__bsg_y * BY + y_i][__bsg_x * BX + x_i] += pM1[__bsg_y * BY + y_i][z_i] * pM2[z_i][__bsg_y * BX + x_i];
        //    pM1 = bsg_remote_ptr(lM1, __bsg_x + b_i, 0) -- row major
        //    pM2 = bsg_remote_ptr(lM1, 0, __bsg_y + b_i) -- column major

        // This is the computation that each tile in the group
        // performs. It can be re-written with the existing
        // accum_block function (see tile parallel version):

        // pM1 = bsg_remote_ptr(lM1, __bsg_x, 0)
        // pM2 = bsg_remote_ptr(lM1, 0, __bsg_y)
        // for(b_i = 0; b_i < TBX/BX; b_i ++){
        //    accum_block(block_out, pM1, pM2)
        //    pM1 = bsg_remote_ptr(lM1, __bsg_x + b_i, 0) -- row major
        //    pM2 = bsg_remote_ptr(lM1, 0, __bsg_y + b_i) -- column major

        // There are three optimizations that can be made. First,
        // calling bsg_remote_ptr is expensive since the compiler does
        // shifts and or's to compute the pointer. Moving between
        // tiles in the x direction is simply adding 0x40000 to a
        // pointer, and moving in the y direction is simply adding
        // 0x800000. With this update:

        // pM1 = bsg_remote_ptr(lM1, 0, __bsg_y)
        // pM2 = bsg_remote_ptr(lM2, __bsg_x, 0)
        // for(b_i = 0; b_i < TBX/BX; b_i ++){
        //    accum_block(block_out, pM1, pM2)
        //    pM1 = (char *) pM1 + 0x40000;
        //    pM2 = (char *) pM2 + 0x800000;
        
        // The second optimization is more tricky. When all tiles in a
        // group start at the same x offset (namely 0) they all access
        // the same tile to get data for M1. Likewise, all tiles in a
        // column will access the same tile to get data for M2.

        // If we offset the start location of each tile by __bsg_x so
        // that the accesses are split between tiles in a row and
        // column this (should) address the problem. Note: If TGX >
        // TGY this will be an issue. But as stated before TGX == TGY

        // However, one final thing. If we blindly increment the
        // address we will access an invalid EVA. Therefore it is
        // critical for us to wrap around when we reach the edge of
        // the tile group. If the tile group size is a power of 2,
        // this can be done with binary and:

        // pM1 = bsg_remote_ptr(lM1, __bsg_x, __bsg_y)
        // pM2 = bsg_remote_ptr(lM2, __bsg_x, __bsg_x)
        // for(b_i = 0; b_i < TBX/BX; b_i ++){
        //    accum_block(block_out, pM1, pM2)
        //    pM1 = ((char *) pM1 + 0x40000) & (MASK(BSG_TILE_GROUP_X_DIM + 18));
        //    pM2 = ((char *) pM2 + 0x800000) & (MASK(BSG_TILE_GROUP_Y_DIM + 23));

        // There is one further optmization, but it requires slight
        // restructuring of the code above. accum_block loads block_out
        // into registers, and stores it back to scratchpad every time
        // it is called. This can be optimized out by refactoring the
        // code so that b loop is back inside, next to the z loop:
        
        // for(y_i = 0; y_i < BY; y_i ++){
        //    for(x_i = 0; x_i < BX; x_i ++){
        //       for(b_i = 0; b_i < TBX/BX; b_i ++){
        //          for(z_i = 0; z_i < BX; z_i ++){
        //             block_out[__bsg_y * BY + y_i][__bsg_x * BX + x_i] += tM1[__bsg_y * BY + y_i][b_i * BX + z_i] + tM2[b_i * BX + z_i][__bsg_y * BX + x_i];

        // Applying the pointer transformation from above:

        // for(y_i = 0; y_i < BY; y_i ++){
        //    for(x_i = 0; x_i < BX; x_i ++){
        //       pM1 = bsg_remote_ptr(lM1, __bsg_x, __bsg_y)
        //       pM2 = bsg_remote_ptr(lM2, __bsg_x, __bsg_x)
        //       for(b_i = 0; b_i < TBX/BX; b_i ++){
        //          for(z_i = 0; z_i < BX; z_i ++){
        //             block_out[__bsg_y * BY + y_i][__bsg_x * BX + x_i] += pM1[__bsg_y * BY + y_i][z_i] + pM2[z_i][__bsg_y * BX + x_i];
        //          pM1 = ((char *) pM1 + 0x40000) & (MASK(BSG_TILE_GROUP_X_DIM + 18));
        //          pM2 = ((char *) pM2 + 0x800000) & (MASK(BSG_TILE_GROUP_Y_DIM + 23));

        // This code cannot reuse accum block, but it does remove an
        // unnecessary load and store of block_out that is inside of accum_block.

        // This can be re-written as:
        // for(y_i = 0; y_i < BY; y_i ++){
        //    for(x_i = 0; x_i < BX; x_i ++){
        //       pM1 = bsg_remote_ptr(lM1, __bsg_x, __bsg_y)
        //       pM2 = bsg_remote_ptr(lM2, __bsg_x, __bsg_x)
        //       accum_row(block_out, pm1, pm2)

        // Theoretical FLOP rates:
        //
        // Assuming BSG_TILE_GROUP_X_DIM = BSG_TILE_GROUP_Y_DIM, and BX = BY = 4 (to maximize register use)
        //
        // For the first tile-group based approach without the block_out
        // optimization, the instructions for the inner loop are:
        //     instr_fma: BSG_TILE_GROUP_X_DIM * (16^3)
        //     instr_load (remote): 2 * (16^2)
        //     instr_load (local/group): G*(8 * (16^2) + (16^2))
        //     instr_store (local): BSG_TILE_GROUP_X_DIM * (16^2)
        //
        // Thus, the flop/instr rate is: BSG_TILE_GROUP_X_DIM * 2 * (16^3) / (BSG_TILE_GROUP_X_DIM * 9 * (16^2) + 2 * (16^2) + BSG_TILE_GROUP_X_DIM * (16^3)) --> BSG_TILE_GROUP_X_DIM * 32 / (BSG_TILE_GROUP_X_DIM * 25  + 2)
        //
        // As BSG_TILE_GROUP_X_DIM approaches infinity the ratio becomes 32/25, or 1.28 flops/instr

        // For the second approach with the block_out optimization the
        // instructions for the inner loop are:
        //     instr_fma: BSG_TILE_GROUP_X_DIM * (16^3)
        //     instr_load (remote): 2 * (16^2)
        //     instr_load (local/group): G*(8 * (16^2))
        //     instr_store (local): (16^2)
        //
        // Thus, the flop/instr rate is: BSG_TILE_GROUP_X_DIM * 2 * (16^3) / (BSG_TILE_GROUP_X_DIM * 8 * (16^2) + (16^2) + BSG_TILE_GROUP_X_DIM * (16^3)) --> BSG_TILE_GROUP_X_DIM * 32 / (BSG_TILE_GROUP_X_DIM * 24  + 1)
        //
        // As BSG_TILE_GROUP_X_DIM approaches infinity the ratio becomes 32/25, or 1.33 flops/instr

        bsg_barrier_hw_tile_group_init();
        // Start profiling
        bsg_fence();
        bsg_barrier_hw_tile_group_sync();
        bsg_cuda_print_stat_kernel_start();
        //        bsg_cuda_print_stat_start(1);

        bsg_tile_group_strider<BSG_TILE_GROUP_X_DIM, 1, BSG_TILE_GROUP_Y_DIM, 0, float> prow(block_row, __bsg_x, __bsg_y);
        bsg_tile_group_strider<BSG_TILE_GROUP_X_DIM, 0, BSG_TILE_GROUP_Y_DIM, 1, float> pcol(block_col, __bsg_x, __bsg_x);
 block_y_loop:
        for (int by_i = __bsg_tile_group_id_y; by_i < by_blocks; by_i += by_stride) {
        block_x_loop:
                for (int bx_i = __bsg_tile_group_id_x; bx_i < bx_blocks; bx_i += bx_stride) {
                        // Multiply each pair of input blocks from a
                        // given m1 row, and m2 column. 
                block_z_loop:
                        for (int bz_i = 0; bz_i < bz_blocks; bz_i += bz_stride) {
                                load_block<BY, BX, LOAD_M1_TRANSPOSED>(block_row, mat1, mat1_strides, by_i * BSG_TILE_GROUP_Y_DIM + __bsg_y, bz_i * BSG_TILE_GROUP_X_DIM + __bsg_x);
                                load_block<BY, BX, false>(block_col, mat2, mat2_strides, bz_i * BSG_TILE_GROUP_Y_DIM + __bsg_y, bx_i * BSG_TILE_GROUP_X_DIM + __bsg_x);
                                bsg_barrier_hw_tile_group_sync();

                                // Multiply the blocks, and accumulate into the result
                                accum_row<BY, 4, BX, 4, 2, LOAD_M1_TRANSPOSED>(block_out, prow, pcol);
                                
                                bsg_barrier_hw_tile_group_sync();
                        }
                        // Store the result, AND zero the block_out array
                        // to leverage parallel remote and local
                        // stores.

                        // This prefetching seems to help by
                        // distributing misses a bit better that the
                        // store. Two questions remain
                        // unanswered. 1. Can we move the prefetch
                        // farther from the store? (yes, probably, but
                        // where) 2. Can we do it in a way that is low
                        // cost? Putting it in the inner-loop (above)
                        // adds an if-check and (likely) a branch
                        // miss.
                        prefetch<BY, BX>(result, result_strides, by_i * BSG_TILE_GROUP_Y_DIM + __bsg_y, bx_i * BSG_TILE_GROUP_X_DIM + __bsg_x);
                        store_block_and_reset<BY, BX>(block_out, result, result_strides, by_i * BSG_TILE_GROUP_Y_DIM + __bsg_y, bx_i * BSG_TILE_GROUP_X_DIM + __bsg_x);
                }
        }

        //bsg_cuda_print_stat_end(1);
        // End profiling
        bsg_barrier_hw_tile_group_sync();
        bsg_cuda_print_stat_kernel_end();

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

