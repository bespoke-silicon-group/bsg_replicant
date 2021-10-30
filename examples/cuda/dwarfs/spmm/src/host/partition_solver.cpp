#include <vector>
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
    int partition_factor = atoi(argv[11]);
    
    auto A = eigen_sparse_matrix::FromAsciiNonZerosList<CSR>(
        input
        , weighted
        , directed
        , zero_indexed
        );


    std::vector<CSR*> mjr_partitions = eigen_sparse_matrix::PartitionMjrPtr<CSR*>(&A, partition_factor);
    std::vector<int>  mjr_part_base  = eigen_sparse_matrix::PartitionStartPtr<CSR*>(&A, partition_factor);
    std::vector<CSR*> mnr_partitions = eigen_sparse_matrix::PartitionMnrPtr<CSR*>(&A, partition_factor);

    for (int i = 0; i < mjr_partitions.size(); i++) {
        for (int j = 0; j < mnr_partitions.size(); j++) {
            CSR *A_i = mjr_partitions[i];
            CSR *A_j = mjr_partitions[j];
            Solver<CSR> slvr(*A_i, *A_j);
            slvr.solve_range(mjr_part_base[i],mjr_part_base[i+1]);
            //auto sol = slvr.solutions();
            //auto ans = CSR(((*A_i)*(*A_j)).pruned());
            std::string prefix = std::to_string(i) + "_" + std::to_string(j);
            std::ofstream of (prefix + "." + solve_row_stats);
            of << slvr.solve_row_stats();

            std::ofstream af(prefix + "." + a_stats);
            af << slvr.A_stats();
            
            std::ofstream bf(prefix + "." + b_stats);
            bf << slvr.B_stats();    
        }
    }
    Solver<CSR>slvr(A, A);
    slvr.solve();

    return 0;
}
