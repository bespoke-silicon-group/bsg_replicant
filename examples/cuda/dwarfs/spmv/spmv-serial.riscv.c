#include "bsg_manycore.h"
#include "sparse_matrix.h"
#ifndef ROWS
#error please define ROWS
#endif

#ifndef NNZ_PER_ROW
#error "please define NNZ_PER_ROW"
#endif

int spmv(sparse_matrix_t *spmatptr
         , float *vector_i
         , float *vector_o)
{
    sparse_matrix_t mat = *spmatptr;
    // for each row...
    for (int i = 0; i < ROWS; i++) {
        int row_off = mat.mnr_off_ptr[i];
        float row_sum = 0.0;
        // for each non-zero in row
        for (int j = 0; j < NNZ_PER_ROW; j++) {
            // fetch non-zero's column
            int k    = mat.mnr_idx_ptr[row_off+j];
            // fetch non-zero's data
            float d0 = mat.val_ptr[row_off+j];
            // fetch input vector's data
            float d1 = vector_i[k];
            // fma            
            row_sum += d0*d1;
        }
        // update output
        vector_o[i] = row_sum;
    }
    return 0;
}
