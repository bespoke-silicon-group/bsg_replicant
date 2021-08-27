#include "bsg_manycore_regression.h"
#include "CommandLine.hpp"
#include "EigenSparseMatrix.hpp"
#include <iostream>
using namespace spmm;
using namespace dwarfs;

int SpGEMM(int argc, char *argv[])
{
    auto cl = CommandLine::Parse(argc, argv);
    printf("%-14s = %s\n", "riscv_path",  cl.riscv_path().c_str());
    printf("%-14s = %s\n", "kernel_name", cl.kernel_name().c_str());
    printf("%-14s = %s\n", "input_path", cl.input_path().c_str());
    printf("%-14s = %d\n", "is_directed", cl.input_is_directed());
    printf("%-14s = %d\n", "is_weighted", cl.input_is_weighted());
    printf("%-14s = %d\n", "is_zero_indexed", cl.input_is_zero_indexed());    

    using CSR = Eigen::SparseMatrix<float, Eigen::RowMajor>;

    auto A = eigen_sparse_matrix::FromAsciiNonZerosList<CSR>(
        cl.input_path()
        , cl.input_is_weighted()
        , cl.input_is_directed()
        , cl.input_is_zero_indexed()
        );
    
    auto B = A;
    auto AxA = CSR((A*B).pruned());
    eigen_sparse_matrix::write_nnz(A, "A.nnz.csv");
    eigen_sparse_matrix::write_nnz(AxA, "AxA.nnz.csv");
    return HB_MC_SUCCESS;
}

declare_program_main("SpGEMM", SpGEMM);
