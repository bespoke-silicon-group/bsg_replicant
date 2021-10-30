#include "EigenSparseMatrix.hpp"
#include "Solver.hpp"
#include <string>
using namespace dwarfs;
using namespace spmm;
int main(int argc, char *argv[])
{
    using CSR = Eigen::SparseMatrix<float, Eigen::RowMajor>;
    std::string fname = argv[1];
    int directed = atoi(argv[2]);
    int weighted = atoi(argv[3]);
    int zero_indexed = atoi(argv[4]);
    std::string ofname = argv[5];
    CSR mat = eigen_sparse_matrix::FromAsciiNonZerosList<CSR>(fname, weighted, directed, zero_indexed);
    eigen_sparse_matrix::write_matrix(mat, ofname + ".el");
    //using CSRPtr = CSR*;
    //CSR *matp = &mat;
    int partition_factor = 8;
    using CSRPtr = std::shared_ptr<CSR>;
    std::shared_ptr<CSR> matp = std::shared_ptr<CSR>(new CSR(mat));
    std::vector<CSRPtr> mjr_partitions = eigen_sparse_matrix::PartitionMjrPtr(matp, partition_factor);
    std::vector<CSRPtr> mnr_partitions = eigen_sparse_matrix::PartitionMnrPtr(matp, partition_factor);    
    for (int i = 0; i < mjr_partitions.size(); i++) {
        for (int j = 0; j < mnr_partitions.size(); j++) {
            CSRPtr A = mjr_partitions[i];
            CSRPtr B = mnr_partitions[j];
            std::string prefix
                = ofname
                + "." + std::to_string(i) + "_"  + std::to_string(j);
            
            Solver<CSR> slvr(*A, *B);            
            slvr.solve();
            std::ofstream of (prefix + ".solve_row_stats.csv");
            of << slvr.solve_row_stats();
        }
    }
    return 0;
}
