#include <iostream>
#include <fstream>
#include "EigenSparseMatrix.hpp"
#include "Solver.hpp"


using namespace spmm;
using namespace dwarfs;

int main(int argc, char *argv[])
{
    using CSR = Eigen::SparseMatrix<float, Eigen::RowMajor>;
    std::string input = argv[1];
    int weighted     = *(argv[2])=='y';
    int directed     = *(argv[3])=='y';
    int zero_indexed = *(argv[4])=='y';
    std::string solve_row_stats = argv[5];
    
    auto A = eigen_sparse_matrix::FromAsciiNonZerosList<CSR>(
        input
        , weighted
        , directed
        , zero_indexed
        );


    Solver<CSR>slvr(A, A);
    slvr.solve();

    std::ofstream of(solve_row_stats);
    of << slvr.solve_row_stats();
    
    return 0;
}
