#include "bsg_manycore_regression.h"
#include "CommandLine.hpp"
#include "EigenSparseMatrix.hpp"
#include "SparseMatrix.hpp"
#include "SparseMatrixProductPartitioner.hpp"
#include "Solver.hpp"
#include "HammerBlade.hpp"
#include <iostream>
#include <memory>

using namespace spmm;
using namespace dwarfs;
using namespace hammerblade::host;

int SpGEMM(int argc, char *argv[])
{
    auto cl = SpMMPartitionCommandLine::Parse(argc, argv);
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
    
    // auto AxA = CSR((A*B).pruned());

    // eigen_sparse_matrix::write_nnz(A, "A.nnz.csv");
    // eigen_sparse_matrix::write_nnz(AxA, "AxA.nnz.csv");
    // eigen_sparse_matrix::write_offset(AxA, "AxA.offset.csv");
    // eigen_sparse_matrix::write_matrix(AxA, "AxA.txt");

    // init inputs
    std::shared_ptr<CSR> A_ptr = std::shared_ptr<CSR>(new CSR(A));

    printf("calculating partition (%3d,%3d) of (%3d,%3d)\n"
           , cl.partition_i()
           , cl.partition_j()
           , cl.partfactor()
           , cl.partfactor()
        );

    SparseMatrixProductPartitioner<std::shared_ptr<CSR>> partitioner(A_ptr, A_ptr, cl.partfactor());
    //// divide input matrix by partition factor
    //std::vector<std::shared_ptr<CSR>> row_partitions = eigen_sparse_matrix::PartitionMjrPtr(A_ptr, cl.partfactor());
    //std::vector<std::shared_ptr<CSR>> col_partitions = eigen_sparse_matrix::PartitionMnrPtr(A_ptr, cl.partfactor());    
    
    // init application
    // HammerBlade::Ptr hb = HammerBlade::Get();
    // hb->load_application(cl.riscv_path());

    // SparseMatrix<CSR> A_dev, B_dev, C_dev;

    // A_dev.initializePartitionFromEigenSparseMatrix(A_ptr, 64, 0);
    // B_dev.initializePartitionFromEigenSparseMatrix(A_ptr, 64, 0);
    // B_dev.initializeFromEigenSparseMatrix(A_ptr);
    // C_dev.initializeEmptyProduct(A_ptr, A_ptr);

    // // allocate dynamic memory pool
    // hb_mc_eva_t mem_pool = hb->alloc(128 * sizeof(int) * 1024 * 1024);
    // hb_mc_eva_t mem_pool_val
    //     = mem_pool // have it start after the mem pool
    //     + hb->config()->vcache_stripe_words // a cache line away, to avoid false sharing
    //     * sizeof(int); // word size

    // // mem_pool = start of memory pool
    // hb->push_write(mem_pool, &mem_pool_val, sizeof(mem_pool_val));

    // // sync data
    // hb->sync_write();

    // std::cout << "Launching kernel on "
    //           << cl.tgx() << " x "
    //           << cl.tgy() << " grid" << std::endl;
    // std::cout << cl.kernel_name() << std::hex
    //           << "(" << A_dev.hdr_dev()
    //           << "," << B_dev.hdr_dev()
    //           << "," << C_dev.hdr_dev()
    //           << "," << mem_pool
    //           << std::dec
    //           << "," << cl.row_base()
    //           << "," << cl.row_base()+cl.rows()
    //           << ")" << std::dec << std::endl;
    // // launch kernel
    // hb->push_job(Dim(1,1), Dim(cl.tgx(),cl.tgy())
    //              , cl.kernel_name()
    //              , A_dev.hdr_dev()
    //              , B_dev.hdr_dev()
    //              , C_dev.hdr_dev()
    //              , mem_pool
    //              , cl.row_base()
    //              , cl.row_base()+cl.rows());
    // hb->exec();

    // std::shared_ptr<CSR> C_ptr = C_dev.updateEmptyProduct();
    // eigen_sparse_matrix::write_nnz(*C_ptr, "C.nnz.csv");
    // eigen_sparse_matrix::write_offset(*C_ptr, "C.offset.csv");
    // eigen_sparse_matrix::write_matrix(*C_ptr, "C.txt");

    // // check equality for computed rows
    // bool eq = eigen_sparse_matrix::mjr_range_equal(AxA, *C_ptr, cl.row_base(), cl.row_base()+cl.rows());
    // std::cout << "mjr_range_equal(AxA, *C_ptr, "
    //           << cl.row_base() << ", "
    //           << cl.row_base() + cl.rows()
    //           << ") = " << eq << std::endl;
    
    // hb->close();
    return HB_MC_SUCCESS;
}

declare_program_main("SpGEMM", SpGEMM);
