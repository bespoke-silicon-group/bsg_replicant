#include "EigenSparseMatrix.hpp"
#include "bsg_manycore_regression.h"
#include "CommandLine.hpp"
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
    {
        auto AxA = CSR((A*A));

        eigen_sparse_matrix::show_metadata(A);
        eigen_sparse_matrix::show_metadata(AxA);
    }
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


    // init application
    HammerBlade::Ptr hb = HammerBlade::Get();
    hb->load_application(cl.riscv_path());

    // partition matrix
    SparseMatrixProductPartitioner<std::shared_ptr<CSR>> partitioner(A_ptr, A_ptr, cl.partfactor());
    auto A_dev = partitioner.A(cl.partition_i(), cl.partition_j());
    auto B_dev = partitioner.B(cl.partition_i(), cl.partition_j());
    auto C_dev = partitioner.C(cl.partition_i(), cl.partition_j());

    // allocate dynamic memory pool
    hb_mc_eva_t mem_pool = hb->alloc(128 * sizeof(int) * 1024 * 1024);
    hb_mc_eva_t mem_pool_val
        = mem_pool // have it start after the mem pool
        + hb->config()->vcache_stripe_words // a cache line away, to avoid false sharing
        * sizeof(int); // word size

    // mem_pool = start of memory pool
    hb->push_write(mem_pool, &mem_pool_val, sizeof(mem_pool_val));

    // sync data
    hb->sync_write();

    std::cout << "Launching kernel on "
              << cl.tgx() << " x "
              << cl.tgy() << " grid" << std::endl;
    std::cout << cl.kernel_name() << std::hex
              << "(" << A_dev->hdr_dev()
              << "," << B_dev->hdr_dev()
              << "," << C_dev->hdr_dev()
              << "," << mem_pool
              << ")" << std::dec << std::endl;
    // launch kernel
    hb->push_job(
        Dim(1,1)
        , Dim(cl.tgx(),cl.tgy())
        , cl.kernel_name()
        , A_dev->hdr_dev()
        , B_dev->hdr_dev()
        , C_dev->hdr_dev()
        , mem_pool
        );

    hb->exec();

    std::shared_ptr<CSR> C_ptr = C_dev->updateEmptyProduct();
    auto & Ap = *partitioner.EigenA(cl.partition_i(), cl.partition_j());
    auto & Bp = *partitioner.EigenB(cl.partition_i(), cl.partition_j());
    eigen_sparse_matrix::write_matrix(Ap, "Ap.txt");
    eigen_sparse_matrix::write_matrix(Bp, "Bp.txt");
    auto AxA   = CSR((Ap * Bp).pruned());
    eigen_sparse_matrix::write_matrix(AxA, "AxAp.txt");
    eigen_sparse_matrix::write_matrix(*C_ptr, "Cp.txt");
    std::cout << "checking result" << std::endl;
    // check equality for computed rows
    bool eq = eigen_sparse_matrix::mjr_range_equal(
        AxA
        , *C_ptr
        , partitioner.C_major_start(cl.partition_i(), cl.partition_j())
        , partitioner.C_major_stop (cl.partition_i(), cl.partition_j())
        );

    std::cout << "mjr_range_equal(AxA, *C_ptr, "
              << partitioner.C_major_start(cl.partition_i(),cl.partition_j()) << ","
              << partitioner.C_major_stop(cl.partition_i(),cl.partition_j()) << ","
              << ") = " << eq << std::endl;
    
    hb->close();
    return eq ? HB_MC_SUCCESS : HB_MC_FAIL;
}

declare_program_main("SpGEMM", SpGEMM);
