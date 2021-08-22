#pragma once
#include "Eigen/Sparse"
#include <vector>
#include <algorithm>
#include <random>
#include <memory>
#include <sstream>
#include <cstdlib>
#include "Random.hpp"

namespace spmv {

    template<Eigen::StorageOptions FMT>
    class SparseMatrix {
    private:
        using FLOAT = float;
        using EigenSparseMatrix = Eigen::SparseMatrix<FLOAT, FMT, int>;
        using SparseMatrixPtr = std::shared_ptr<EigenSparseMatrix>;

    public:
        SparseMatrix() : _spm_ptr(nullptr) {}

        static SparseMatrix Uniform(int rows
                                    , int columns
                                    , int nnz_per_row) {
            // we use triplets to init sparse matrix
            using Triplet = Eigen::Triplet<FLOAT>;
            SparseMatrix mat;
            std::vector<Triplet> ijk;

            // init random engine and dist
            std::uniform_real_distribution<FLOAT> dist(-1.0, 1.0);
            std::default_random_engine &gen = GLOBAL_RANDOM_ENGINE;

            // make a list of column indices
            std::vector<int> cols;
            for (int j = 0; j < columns; j++) {
                cols.push_back(j);
            }
            std::random_shuffle(cols.begin(), cols.end());

            // shuffle for each row
            int nnz = 0;
            for (int i = 0; i < rows; i++) {
                // select the first nnz
                for (int nnz_i = 0; nnz_i < nnz_per_row; nnz_i++) {
                    // reshuffle and reset nnz
                    if (nnz == cols.size()) {
                        std::random_shuffle(cols.begin(), cols.end());
                        nnz = 0;
                    }
                    int j = cols[nnz++];
                    float k = dist(gen);
                    ijk.push_back(Triplet(i,j,k));
                }
            }

            // allocate sparse matrix and initialize from triplets
            SparseMatrixPtr ptr = SparseMatrixPtr(new EigenSparseMatrix(rows, columns));
            ptr->setFromTriplets(ijk.begin(), ijk.end());
            mat._spm_ptr = ptr;
            mat._spm_ptr->uncompress();
            return mat;
        }

        std::string to_string() const {
            std::stringstream ss;
            ss << "inner size : " << _spm_ptr->innerSize() << "\n";
            ss << "outer size : " << _spm_ptr->outerSize() << "\n";
            ss << "rows : " << _spm_ptr->rows() << "\n";
            ss << "columns : " << _spm_ptr->cols() << "\n";
            for (int major = 0; major < _spm_ptr->outerSize(); major++) {
                int nnz = _spm_ptr->innerNonZeroPtr()[major];
                int off = _spm_ptr->outerIndexPtr()[major];
                int   *minor_ptr = _spm_ptr->innerIndexPtr()+off;
                FLOAT *value_ptr = _spm_ptr->valuePtr()+off;
                ss << major << " : ";
                for (int j = 0; j < nnz; j++)
                    ss << "(" << minor_ptr[j] << "," << value_ptr[j] << "), ";

                ss << "\n";
            }
            return ss.str();
        }

        int rows() const { return _spm_ptr->rows(); }
        int cols() const { return _spm_ptr->cols(); }

        int numMajors() const { return _spm_ptr->outerSize(); }
        int numMinors()  const { return _spm_ptr->innerSize(); }
        int nonZeros() const  { return _spm_ptr->nonZeros();  }

        const int* majorNNZPtr() const { return _spm_ptr->innerNonZeroPtr(); }
        const int* minorOffPtr() const { return _spm_ptr->outerIndexPtr(); }
        const int* minorIdxPtr() const { return _spm_ptr->innerIndexPtr(); }
        const FLOAT*  valuePtr() const { return _spm_ptr->valuePtr(); }
        const int   isRowMajor() const { return FMT == Eigen::RowMajor ? 1 : 0; }
        EigenSparseMatrix eigenSparseMatrix() const { return *_spm_ptr; }

    private:
        SparseMatrixPtr _spm_ptr;
    };

    using CSR = SparseMatrix<Eigen::RowMajor>;
    using CSC = SparseMatrix<Eigen::ColMajor>;
}
