#include "bsg_manycore_regression.h"
#include "bsg_manycore_cuda.h"
#include "CommandLine.hpp"
#include "HammerBlade.hpp"
#include "EigenSparseMatrix.hpp"
#include "sparse_matrix.h"
#include "Eigen/Core"
#include <iostream>
using namespace spmv;
using namespace dwarfs;
using namespace hammerblade::host;

int spmv_main(int argc, char *argv[])
{
    // parse command line
    auto cl = CommandLine::Parse(argc, argv);

    // generate input
    bsg_pr_info("Generating input\n");

    using CSR = Eigen::SparseMatrix<float, Eigen::RowMajor, int>;
    CSR csr = eigen_sparse_matrix::GenerateUniform<CSR>(cl.rows(), cl.cols(), cl.nnz_per_row());

    // init hb
    bsg_pr_info("Initializing manycore\n");
    auto hb = HammerBlade::Get();
    hb->load_application(cl.riscv_path());

    // init input
    sparse_matrix_t spm;
    spm.is_row_major = 1;

    // set scalars
    spm.n_major = csr.rows();
    spm.n_minor = csr.cols();
    spm.n_non_zeros = csr.nonZeros();

    // allocate vectors
    spm.mjr_nnz_ptr = hb->alloc(csr.rows() * sizeof(int));
    spm.mnr_off_ptr = hb->alloc(csr.rows() * sizeof(int));
    spm.mnr_idx_ptr = hb->alloc(csr.cols()  * sizeof(int));
    spm.val_ptr     = hb->alloc(csr.cols()  * sizeof(float));
    hb_mc_eva_t spm_dev = hb->alloc(sizeof(spm));
    
    // init vectors
    hb->push_write(spm.mjr_nnz_ptr
                   , csr.innerNonZeroPtr()
                   , csr.rows() * sizeof(int));

    hb->push_write(spm.mnr_off_ptr
                   , csr.outerIndexPtr()
                   , csr.rows() * sizeof(int));

    hb->push_write(spm.mnr_idx_ptr
                   , csr.innerIndexPtr()
                   , csr.nonZeros() * sizeof(int));

    hb->push_write(spm.val_ptr
                   , csr.valuePtr()
                   , csr.nonZeros() * sizeof(float));

    hb->push_write(spm_dev
                   , &spm
                   , sizeof(spm));

    // allocate input vector
    Eigen::VectorXf v_i = Eigen::VectorXf::Random(csr.cols());
    Eigen::VectorXf v_o = Eigen::VectorXf::Zero(csr.rows());
    Eigen::VectorXf v_lock_o = v_o;

    hb_mc_eva_t v_i_dev = hb->alloc(csr.cols() * sizeof(float));
    hb_mc_eva_t v_o_dev = hb->alloc(csr.rows() * sizeof(float));
    hb_mc_eva_t v_lock_o_dev = hb->alloc(csr.rows() * sizeof(int));

    hb->push_write(v_i_dev
                   , v_i.data()
                   , csr.cols() * sizeof(float));

    hb->push_write(v_lock_o_dev
                   , v_lock_o.data()
                   , csr.rows() * sizeof(int));

    // launch kernel
    bsg_pr_info("Writing input\n");
    hb->sync_write();

    bsg_pr_info("Running kernel with %d groups of size (%d,%d)\n"
                , cl.groups()
                , cl.tgx()
                , cl.tgy());

    Dim grid(cl.groups(), 1);
    Dim grp(cl.tgx(), cl.tgy());
    
    hb->push_job(grid, grp, "spmv"
                 , spm_dev
                 , v_i_dev
                 , v_o_dev
                 , v_lock_o_dev);
    hb->exec();

    // read output
    hb->push_read(v_o_dev
                  , v_o.data()
                  , csr.rows() * sizeof(float));
    hb->sync_read();

    Eigen::VectorXf answer = csr * v_i;

    int correct = v_o.isApprox(answer);

    // calculate the l2 norm
    printf("norm(solution-correct) = %f\n"
           , (v_o-answer).norm());

    //int correct = 0;
    printf("v_o.isApprox(answer) = %d\n", correct);
    if (!correct) {
        printf("%20s %20s\n"
               , "correct"
               , "solution");
        for (int i = 0; i < answer.size(); i++) {
            printf("%20f %20f\n"
                   , answer(i)
                   , v_o(i));
        }
    }

    // validate result
    bsg_pr_info("Cleanup\n");
    hb->close();
    return HB_MC_SUCCESS;
}

declare_program_main("SpMV", spmv_main)
