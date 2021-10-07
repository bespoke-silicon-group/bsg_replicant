#include "bsg_manycore.h"
#include "bsg_manycore.hpp"
#include "bsg_set_tile_x_y.h"
#include "sparse_matrix.h"
#include "bsg_tile_group_barrier.hpp"
#include "bsg_manycore_atomic.h"

#ifndef GROUPS
#error please define GROUPS
#endif

#ifndef ROWS
#error please define ROWS
#endif

#ifndef NNZ_PER_ROW
#error please define NNZ_PER_ROW
#endif

#define GROUP_SIZE                              \
    (TILE_GROUP_DIM_X*TILE_GROUP_DIM_Y)

#define GROUPER_X 0
#define GROUPER_Y 0

extern "C" int spmv(sparse_matrix_t *spmatptr
                    , float *vector_i
                    , float *vector_o
                    , int   *vector_o_lock)
{
    sparse_matrix_t mat = *spmatptr;    

    // for each row...
    for (int i = __bsg_tile_group_id; i <  ROWS; i += GROUPS) {
        // row offsets
        int row_off = mat.mnr_off_ptr[i];
        float row_partial = 0.0;
        // for each non-zero in row
        for (int j = __bsg_id; j < NNZ_PER_ROW; j += GROUP_SIZE) {
            // fetch non-zero's column
            int k    = mat.mnr_idx_ptr[row_off+j];
            // fetch non-zero's data
            float d0 = mat.val_ptr[row_off+j];
            // fetch input vector's data
            float d1 = vector_i[k];
            // fma            
            row_partial += d0*d1;
        }        

        // acquire the lock
        while (bsg_amoswap_aq(&vector_o_lock[i], 1) != 0);        
        // add partial sum
        vector_o[i] += row_partial;
        // release the lock
        bsg_amoswap_rl(&vector_o_lock[i], 0);
    }
    return 0;
}
