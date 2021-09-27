#define EIGEN_USE_MKL_ALL
#include "EigenSparseMatrix.hpp"
#include <string>
#include <chrono>
#include <iostream>
#include "mkl.h"

using namespace dwarfs;

std::string sparse_status_to_string(int s)
{
    switch (s) {
    case SPARSE_STATUS_SUCCESS: return "success";
    case SPARSE_STATUS_NOT_INITIALIZED: return "not initialized";
    case SPARSE_STATUS_ALLOC_FAILED: return "no memory";
    case SPARSE_STATUS_INVALID_VALUE: return "invalid value";
    case SPARSE_STATUS_EXECUTION_FAILED: return "execution failed";
    case SPARSE_STATUS_INTERNAL_ERROR: return "internal error";
    case SPARSE_STATUS_NOT_SUPPORTED: return "not suppored";
    }
}

template <typename EigenSparseMatrix>
void create_mkl_sparse(const EigenSparseMatrix &in, sparse_matrix_t *out)
{
    static constexpr size_t ALIGN = 64;
    MKL_INT *rows_start = (MKL_INT*)mkl_malloc(sizeof(MKL_INT) * in.rows(), ALIGN);
    MKL_INT *rows_end   = (MKL_INT*)mkl_malloc(sizeof(MKL_INT) * in.rows(), ALIGN);
    MKL_INT *col_idx    = (MKL_INT*)mkl_malloc(sizeof(MKL_INT) * in.nonZeros(), ALIGN);
    float   *values     = (float*)  mkl_malloc(sizeof(float)   * in.nonZeros(), ALIGN);

    using idx_t = typename EigenSparseMatrix::StorageIndex;
    using real_t = typename EigenSparseMatrix::Scalar;
    for (int i = 0; i < in.rows(); i++) {
        rows_start[i] = static_cast<MKL_INT>(in.outerIndexPtr()[i]);
        rows_end[i] = static_cast<MKL_INT>(rows_start[i] + in.innerNonZeroPtr()[i]);
    }

    for (int nz = 0; nz < in.nonZeros(); nz++) {
        col_idx[nz] = static_cast<MKL_INT>(in.innerIndexPtr()[nz]);
        values[nz]  = static_cast<float>(in.innerIndexPtr()[nz]);
    }
    
    int stat = mkl_sparse_s_create_csr(out, SPARSE_INDEX_BASE_ZERO
                                       , static_cast<MKL_INT>(in.rows())
                                       , static_cast<MKL_INT>(in.cols())
                                       , rows_start
                                       , rows_end
                                       , col_idx
                                       , values);
    if (stat != SPARSE_STATUS_SUCCESS) {
        std::cout << "failed to initialize" << std::endl;
    }
}

int main(int argc, char *argv[])
{    
    std::string input = argv[1];
    int directed     = *(argv[2])=='y';    
    int weighted     = *(argv[3])=='y';
    int zero_indexed = *(argv[4])=='y';
    int iterations = atoi(argv[5]);
    
    using CSR = Eigen::SparseMatrix<float, Eigen::RowMajor>;
    std::cout << "reading input " << input << "..." << std::flush;
    auto esm = eigen_sparse_matrix::FromAsciiNonZerosList<CSR>(
        input
        , weighted
        , directed
        , zero_indexed
        );
    std::cout << "done"<< std::endl;
    
    sparse_operation_t operation = SPARSE_OPERATION_NON_TRANSPOSE;
    sparse_matrix_t A, B, C[iterations];
    std::cout << "initializing matrix A..." << std::flush;
    create_mkl_sparse(esm, &A);
    std::cout << "done" << std::endl;
    std::cout << "initializing matrix B..." << std::flush;;
    create_mkl_sparse(esm, &B);
    std::cout << "done" << std::endl;
    std::cout << "running sparse matrix multiply" << std::endl;
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; i++) {
        int stat = mkl_sparse_spmm(operation, A, B, &C[i]);
        if (stat != SPARSE_STATUS_SUCCESS) {
            std::cout << "failed: " << sparse_status_to_string(stat) << std::endl;
        }
    }
    auto end = std::chrono::steady_clock::now();

    std::chrono::duration<double> seconds = end-start;
    std::cout << "elapsed time for " << iterations << " iterations: " << seconds.count() << std::endl;
    std::cout << "average elapsed time: " << seconds.count()/iterations << std::endl;
    return 0;
}
