//====================================================================
// addmml kernel
// 03/09/2020 Kexin Zheng, Lin Cheng (kz73@cornell.edu, lc873@cornell.edu)
//====================================================================
#include <kernel_common.hpp>

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
        bsg_fence();
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
//     mat1 should be in local scratchpad
//     mat2 should be in local scratchpad
template<unsigned int BY, unsigned int SBY, unsigned int BX, unsigned int SBX, bool M1_TRANSPOSE>
inline void accum_block(float* bsg_attr_noalias dest,
                 float* bsg_attr_noalias mat1,
                 float* bsg_attr_noalias mat2) {

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

                        // Compute an SBY-by-SBX output sub-block by
                        // performing BX, SBY-by-1 x 1-by-SBX
                        // vector-vector multiplies, and accumulate
                        // with the result
                        for(int sbx_i = 0; sbx_i < BX; ++sbx_i) {
                                // Load an SBY-by-1 sub-column of mat1,
                                // 1-by-SBX sub-row of mat2, and perform a
                                // SBY-by-1 x 1-by-SBX vector-vector multiply
                                // that produces an SBY-by-SBX output matrix.
                                // Locations:
                                //     col should be in registers (for SBY == 4)
                                //     row should be in registers (for SBX == 4)
                                float col[SBY];
                                float row[SBX];

                                // Load an SBY-by-1 sub-column of mat1,
                                if (!M1_TRANSPOSE) {
                                        // Location: (pointer to) Scratchpad
                                        float * bsg_attr_noalias col_anchor = &(mat1[sb_anchor_y * BX + sbx_i]);
                                        bsg_unroll(16)
                                        for(int i = 0; i < SBY; ++i){
                                                col[i] = col_anchor[i * BX];
                                        }
                                } else {
                                        // Location: (pointer to) Scratchpad
                                        float * bsg_attr_noalias col_anchor = &(mat1[sb_anchor_y + sbx_i * BY]);
                                        bsg_unroll(16)
                                        for(int i = 0; i < SBY; ++i){
                                                col[i] = col_anchor[i];
                                        }
                                }

                                // Load an SBX-by-1 sub-column of mat2
                                // Location: (pointer to) Scratchpad
                                float * bsg_attr_noalias row_anchor = &(mat2[sbx_i * BY + sb_anchor_x]);
                                bsg_unroll(16)
                                for(int i = 0; i < SBX; ++i){
                                    row[i] = row_anchor[i];
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
                                bsg_unroll(16)
                                for(int sby_i = 0; sby_i < SBY; ++sby_i){
                                        bsg_unroll(16)
                                        for(int sbx_i = 0; sbx_i < SBX; ++sbx_i){
                                                psum[sby_i][sbx_i] = fmaf(col[sby_i], row[sbx_i], psum[sby_i][sbx_i]);
                                                // psum[sby_i][sbx_i] += col[sby_i] * row[sbx_i];
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
// Asserts use bsg_printf, which can pollute the icache
//#define USE_ASSERT
// TODO: X_GROUP, Y_GROUP parameters
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
#ifdef __clang__
__attribute__((no_builtin("memcpy", "memset")))
#endif
{
#ifdef USE_ASSERT

        // M1 columns must equal M2 Rows
        hb_assert(c1 == r2);

        // This MM implementation is blocked into BY-by-BX output
        // blocks. This implies the following dimension constraints:

        // M1 columns must be divisible by the Block X-dimension
        hb_assert(c1 % BX == 0);
        // M2 rows must be divisible by the Block X-dimension
        hb_assert(r2 % BX == 0);

        // M1 rows must be divisible by the Block Y-dimension
        hb_assert(r1 % BY == 0);
        // M2 columns must be divisible by the Block Y-dimension
        hb_assert(c2 % BY == 0);
#endif

        // TODO: The compiler doesn't know that c1 is always (or
        // should always be) nonzero. This adds an extra BNE
        // instruction in the inner-loop. There should be some way to
        // signal to the compiler, perhaps with hb_assert?
        int blocks = c1 / BX; // r2 / BX

        // Local Storage for input blocks
        // Locations:
        //    block_row is in Local Scratchpad
        //    block_col is in Local Scratchpad
        float block_row[BY * BX];
        float block_col[BX * BY];

        // Local storage for partial sums (output)
        // Location: Local Scratchpad
        float psum[BY * BX];

        for (int i = 0; i < BY; i++) {
                bsg_unroll(16)
                for (int j = 0 ; j < BX; j ++){
                        psum[i * BX + j] = 0.0f;
                }
        }

        // To maximize locality, each tile should compute a number of
        // output blocks with spatially local data.
        //
        // This works best when the size (bytes) of the mat2 matrix
        // row is a multiple of the number of bytes in a row of
        // caches.
        
        // Split the work up into contiguous chunks, where each tile
        // will compute on one chunk.
        int bx_blocks = c2/BX;
        int bx_each = bx_blocks / BSG_TILE_GROUP_X_DIM;
        int bx_start = __bsg_x * (bx_each);
        int bx_end = bx_start + bx_each;
        // hb_assert(bx_each == 0);

        // Start profiling
        bsg_cuda_print_stat_kernel_start();

        // Note: This only works for N x N tile groups where N is a
        // power of two. To put it in the symbols below:

        // log2(TDX) == log2(TDY) == 0.

        // Scale-up, matrix multiply: using groups of size TDX x TDY
        // tiles, compute matrix multiply of matrices m1 (r1 x c1),
        // and m2 (r2 x c2)

        // Logically, split the output matrix into hierarchical blocks:

        // - TBX x TBY group blocks that a group computes, composed of:
        // -  BX x  BY tile blocks that a single tile computes

        // Where TBX = TDX * BX and TBY = TDY * BY

        // This means that there are r1 / TBY "row" blocks, and c2 /
        // TBX "column" blocks.
        
        // The number of tile groups launched is defined by the grid
        // dimension: __bsg_grid_dim_x/y (Shorthand: GDX/GDY)

        // Each group is uniquely defined by its tile group X/Y
        // index, __bsg_tile_group_id_x/y (Shorthand: TIX/TIY).

        // 0 < TIX < GDX
        // 0 < TIY < GDY
        
        // Each group will iterate through unprocessed output group
        // blocks in the X and Y dimension, computing the group block
        // at coordinate tx_i, ty_i.
        //
        // The initial value for tx_i is TIX, and ty_i is TIY. After
        // computing an output block, tx_i moves to the next output
        // block that is GDX, and then GDY blocks away.

        // From the group perspective, the row-major MM algorithm with
        // striding looks like this:

        // for(ty_i = TIY; ty_i < r1 / TBY; ty_i += GDY)
        //     for(tx_i = TIX; tx_i < c2 / TBX; tx_i += GDX)
        //         Initialize psum to 0, a TBX x TBY block
        //         for(tz_i = 0; c1 / TBX; tz_i++)
        //             Load TBX x TBY block from m1 and TBY x TBX block from m2
        //             Perform multiply-accumulate into psum
        //         Store psum into output

        // This is the end of the high-level algorithm. It is critical
        // to understand the lines above before proceeding.

        // (Note it is possible to do a stride of 1, in which case,
        // the iteration limits are r1/(GDY * TBY) and c2/(GDX * TBX).
        // We believe that striding by the grid dimension produces
        // more cache locality)

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

        // OTAX = GDX * TBX * tx_i
        // OTAY = GBY * TBY * ty_i

        // The group block anchor coordinates for M1 and M2 are:

        // TM1AX = TBX * tz_i
        // TM1AY = GDY * TBY * ty_i

        // TM2AX = GDX * TBY * tx_i
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
        //          psum[y_i][x_i] += tM1[y_i][z_i] + tM2[z_i][x_i];

        // However, the computation and data is split across tiles
        // into TDX x TDY pieces, where each tile computes a BX x BY
        // output block. We can rewrite the for loops like this: 

        // for(ty_i = 0 ; ty_i < TDY; ty_i ++){
        //    for(tx_i = 0 ; tx_i < TDX; tx_i ++){
        //       for(y_i = 0; y_i < BY; y_i ++){
        //          for(x_i = 0; x_i < BX; x_i ++){
        //             for(z_i = 0; z_i < TBX; z_i ++){
        //                psum[ty_i * BY + y_i][tx_i * BX + x_i] += tM1[ty_i * BY + y_i][z_i] + tM2[z_i][tx_i * BX + x_i];

        // If we assign each iteration of the outer two loops to tiles
        // we can replace ty_i and tx_i with the tile group variables
        // __bsg_x and __bsg_y and start to simplify this code:

        // for(y_i = 0; y_i < BY; y_i ++){
        //    for(x_i = 0; x_i < BX; x_i ++){
        //       for(z_i = 0; z_i < TBX; z_i ++){
        //          psum[__bsg_y * BY + y_i][__bsg_x * BX + x_i] += tM1[__bsg_y * BY + y_i][z_i] + tM2[z_i][__bsg_y * BX + x_i];

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
        //             psum[__bsg_y * BY + y_i][__bsg_x * BX + x_i] += tM1[__bsg_y * BY + y_i][b_i * BX + z_i] + tM2[b_i * BX + z_i][__bsg_y * BX + x_i];

        // Let's reorder the loops so that the block access is the
        // outer most loop, and the inner loops do not cross between
        // blocks in tile scratchpads:

        // for(b_i = 0; b_i < TBX/BX; b_i ++){
        //    for(y_i = 0; y_i < BY; y_i ++){
        //       for(x_i = 0; x_i < BX; x_i ++){
        //          for(z_i = 0; z_i < BX; z_i ++){
        //             psum[__bsg_y * BY + y_i][__bsg_x * BX + x_i] += tM1[__bsg_y * BY + y_i][b_i * BX + z_i] + tM2[b_i * BX + z_i][__bsg_y * BX + x_i];
        
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
        //             psum[__bsg_y * BY + y_i][__bsg_x * BX + x_i] += pM1[__bsg_y * BY + y_i][z_i] + pM2[z_i][__bsg_y * BX + x_i];
        //    pM1 = bsg_remote_ptr(lM1, __bsg_x + b_i, 0) -- row major
        //    pM2 = bsg_remote_ptr(lM1, 0, __bsg_y + b_i) -- column major

        // This is the computation that each tile in the group
        // performs. It can be re-written with the existing
        // accum_block function:

        // pM1 = bsg_remote_ptr(lM1, __bsg_x, 0)
        // pM2 = bsg_remote_ptr(lM1, 0, __bsg_y)
        // for(b_i = 0; b_i < TBX/BX; b_i ++){
        //    accum_block(psum, pM1, pM2)
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
        //    accum_block(psum, pM1, pM2)
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
        //    accum_block(psum, pM1, pM2)
        //    pM1 = ((char *) pM1 + 0x40000) & (MASK(TDX + 18));
        //    pM2 = ((char *) pM2 + 0x800000) & (MASK(TDY + 23));

        // There is one further optmization, but it requires slight
        // restructuring of the code above. accum_block loads psum
        // into registers, and stores it back to scratchpad every time
        // it is called. This can be optimized out by refactoring the
        // code so that b loop is back inside, next to the z loop:
        
        // for(y_i = 0; y_i < BY; y_i ++){
        //    for(x_i = 0; x_i < BX; x_i ++){
        //       for(b_i = 0; b_i < TBX/BX; b_i ++){
        //          for(z_i = 0; z_i < BX; z_i ++){
        //             psum[__bsg_y * BY + y_i][__bsg_x * BX + x_i] += tM1[__bsg_y * BY + y_i][b_i * BX + z_i] + tM2[b_i * BX + z_i][__bsg_y * BX + x_i];

        // Applying the pointer transformation from above:

        // for(y_i = 0; y_i < BY; y_i ++){
        //    for(x_i = 0; x_i < BX; x_i ++){
        //       pM1 = bsg_remote_ptr(lM1, __bsg_x, __bsg_y)
        //       pM2 = bsg_remote_ptr(lM2, __bsg_x, __bsg_x)
        //       for(b_i = 0; b_i < TBX/BX; b_i ++){
        //          for(z_i = 0; z_i < BX; z_i ++){
        //             psum[__bsg_y * BY + y_i][__bsg_x * BX + x_i] += pM1[__bsg_y * BY + y_i][z_i] + pM2[z_i][__bsg_y * BX + x_i];
        //          pM1 = ((char *) pM1 + 0x40000) & (MASK(TDX + 18));
        //          pM2 = ((char *) pM2 + 0x800000) & (MASK(TDY + 23));

        // This code cannot reuse accum block, but it does remove an
        // unnecessary load and store that is inside of accum_block.


        // Theoretical FLOP rates:
        //
        // Assuming TDX = TDY, and BX = BY = 4 (to maximize register use)
        //
        // For the first tile-group based approach without the psum
        // optimization, the instructions for the inner loop are:
        //     instr_fma: TDX * (16^3)
        //     instr_load (remote): 2 * (16^2)
        //     instr_load (local/group): G*(8 * (16^2) + (16^2))
        //     instr_store (local): TDX * (16^2)
        //
        // Thus, the flop/instr rate is: TDX * 2 * (16^3) / (TDX * 9 * (16^2) + 2 * (16^2) + TDX * (16^3)) --> TDX * 32 / (TDX * 25  + 2)
        //
        // As TDX approaches infinity the ratio becomes 32/25, or 1.28 flops/instr

        // For the second approach with the psum optimization the
        // instructions for the inner loop are:
        //     instr_fma: TDX * (16^3)
        //     instr_load (remote): 2 * (16^2)
        //     instr_load (local/group): G*(8 * (16^2))
        //     instr_store (local): (16^2)
        //
        // Thus, the flop/instr rate is: TDX * 2 * (16^3) / (TDX * 8 * (16^2) + (16^2) + TDX * (16^3)) --> TDX * 32 / (TDX * 24  + 1)
        //
        // As TDX approaches infinity the ratio becomes 32/25, or 1.33 flops/instr

        for (int by_i = __bsg_y; by_i < r1/BY; by_i += BSG_TILE_GROUP_Y_DIM) {
                for (int bx_i = __bsg_x; bx_i < c2/BX; bx_i += BSG_TILE_GROUP_X_DIM) {

                        // Multiply the blocks, and accumulate into the result
                        if(PROFILE)
                                bsg_cuda_print_stat_start(0);
                        // TODO: divide bz by X_GROUP
                        for (int bz_i = 0; bz_i < blocks; bz_i++) {
                                if(PROFILE)
                                        bsg_cuda_print_stat_start(1);
                                load_block<BY, BX, LOAD_M1_TRANSPOSED>(block_row, mat1, mat1_strides, by_i, bz_i);
                                if(PROFILE){
                                        bsg_cuda_print_stat_end(1);
                                        bsg_cuda_print_stat_start(2);
                                }

                                //TODO: mat2 depends on Y_GROUP, must load in m2 bz/Y_GROUP times
                                load_block<BY, BX, false>(block_col, mat2, mat2_strides, bz_i, bx_i);
                                // TODO: Run accum_block X_GROUP * Y_GROUP times, changing the
                                if(PROFILE){
                                        bsg_cuda_print_stat_end(2);
                                        bsg_cuda_print_stat_start(3);
                                }
                                // TODO: always do an accumulate
                                accum_block<BY, 4, BX, 4, LOAD_M1_TRANSPOSED>(psum, block_row, block_col);
                                if(PROFILE){
                                        bsg_cuda_print_stat_end(3);
                                }
                        }

                        if(PROFILE)
                                bsg_cuda_print_stat_start(4);

                        // Store the result, AND zero the psum array
                        // to leverage parallel remote and local
                        // stores.
                        prefetch<BY, BX>(result, result_strides, by_i, bx_i);
                        store_block_and_reset<BY, BX>(psum, result, result_strides, by_i, bx_i);
                        if(PROFILE){
                                bsg_cuda_print_stat_end(4);
                                bsg_cuda_print_stat_end(0);
                        }
                }
        }

        // End profiling
        bsg_cuda_print_stat_kernel_end();

        return 0;
}
/*
extern "C"
int kernel_mm_opt_8x8(
                  hb_tensor_t* _result,
                  hb_tensor_t* _mat1,
                  hb_tensor_t* _mat2) {

        auto mat1 = HBTensor<float, 2>(_mat1);
        auto mat2 = HBTensor<float, 2>(_mat2);
        auto result = HBTensor<float, 2>(_result);
        
        kernel_mm_opt<8,8,false, false>((float* bsg_attr_noalias) result.data_ptr(),
                                        result.get_strides(),
                                        (float* bsg_attr_noalias) mat1.data_ptr(),
                                        mat1.get_strides(),
                                        mat1.dim(0), mat1.dim(1),
                                        (float* bsg_attr_noalias) mat2.data_ptr(),
                                        mat2.get_strides(),
                                        mat2.dim(0), mat2.dim(1));
        g_barrier.sync();
        return 0;
}
*/
extern "C"
int kernel_mm_opt_16x16(
                  hb_tensor_t* _result,
                  hb_tensor_t* _mat1,
                  hb_tensor_t* _mat2) {

        auto mat1 = HBTensor<float, 2>(_mat1);
        auto mat2 = HBTensor<float, 2>(_mat2);
        auto result = HBTensor<float, 2>(_result);
        
        kernel_mm_opt<16,16,false, false>((float bsg_attr_remote * bsg_attr_noalias) result.data_ptr(),
                                        result.get_strides(),
                                        (float bsg_attr_remote * bsg_attr_noalias) mat1.data_ptr(),
                                        mat1.get_strides(),
                                        mat1.dim(0), mat1.dim(1),
                                        (float bsg_attr_remote * bsg_attr_noalias) mat2.data_ptr(),
                                        mat2.get_strides(),
                                        mat2.dim(0), mat2.dim(1));
        g_barrier.sync();
        return 0;
}

