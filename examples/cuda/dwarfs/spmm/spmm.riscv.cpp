#include "bsg_manycore.h"
#include "sparse_matrix.h"

extern "C" int spmm(sparse_matrix_t *A_ptr, // csr
                    sparse_matrix_t *B_ptr, // csr
                    sparse_matrix_t *C_ptr) // csr
{
    return 0;
}
