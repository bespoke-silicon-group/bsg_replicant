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
    int directed     = *(argv[2])=='y';    
    int weighted     = *(argv[3])=='y';
    int zero_indexed = *(argv[4])=='y';
    std::string solve_row_stats = argv[5];
    std::string A_name = argv[6];
    std::string sol_name = argv[7];
    std::string ans_name = argv[8];
    std::string a_stats  = argv[9];
    std::string b_stats  = argv[10];
    
    auto A = eigen_sparse_matrix::FromAsciiNonZerosList<CSR>(
        input
        , weighted
        , directed
        , zero_indexed
        );


    Solver<CSR>slvr(A, A);
    slvr.solve();

    auto sol = slvr.solution();
    auto ans = CSR((A * A).pruned());
    eigen_sparse_matrix::write_matrix(A,   A_name);
    eigen_sparse_matrix::write_matrix(sol, sol_name);
    eigen_sparse_matrix::write_matrix(ans, ans_name);

    std::ofstream of(solve_row_stats);
    of << slvr.solve_row_stats();

    std::ofstream af(a_stats);
    af << slvr.A_stats();

    std::ofstream bf(b_stats);
    bf << slvr.B_stats();
    
    return 0;
}
