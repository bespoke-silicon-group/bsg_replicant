#include <Eigen/Sparse>
#include <sstream>
#include <fstream>
#include <vector>
#include <random>
#include <cstdlib>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include "Random.hpp"
#pragma once

namespace dwarfs {

    namespace eigen_sparse_matrix {
        /* dump non-zeros data to a file */
        template <class SparseMatrixType>
        void write_nnz(SparseMatrixType &mat, const std::string &fname) {
            if (mat.isCompressed())
                mat.uncompress();
            std::ofstream ofs(fname);
            using Index = typename SparseMatrixType::StorageIndex;
            const Index *nnzs = mat.innerNonZeroPtr();
            std::cout << "# nnz    = " << mat.nonZeros() << std::endl;
            std::cout << "# majors = " << mat.outerSize() << std::endl;
            std::cout << "# minors = " << mat.innerSize() << std::endl;
            ofs << "major,nnz" << std::endl;
            for (Index i = 0; i < mat.outerSize(); i++) {
                Index nnz = nnzs[i];
                ofs << i << "," << nnz << std::endl;
            }
        }
        /* dump non-zeros data to a file */
        template <class SparseMatrixType>
        void write_matrix(SparseMatrixType &mat, const std::string &fname) {
            if (mat.isCompressed())
                mat.uncompress();
            std::ofstream ofs(fname);
            using Index = typename SparseMatrixType::StorageIndex;
            using Scalar = typename SparseMatrixType::Scalar;
            const Index *nnz_ptr = mat.innerNonZeroPtr();
            const Index *off_ptr = mat.outerIndexPtr();
            const Index *idx_ptr = mat.innerIndexPtr();
            const Scalar *val_ptr = mat.valuePtr();
            
            std::cout << "# nnz    = " << mat.nonZeros() << std::endl;
            std::cout << "# majors = " << mat.outerSize() << std::endl;
            std::cout << "# minors = " << mat.innerSize() << std::endl;

            for (Index i = 0; i < mat.outerSize(); i++) {
                Index nnz = nnz_ptr[i];
                Index off = off_ptr[i];
                const Index  *idxp = &idx_ptr[off];
                const Scalar *valp = &val_ptr[off];
                ofs << i << ":";
                for (Index nonzero = 0; nonzero < nnz; nonzero++) {
                    ofs << " (" << idxp[nonzero] << "," << valp[nonzero] << ")";
                }
                ofs << std::endl;
            }
        }
    
        /**
         * Generates a sparse matrix with uniform number of non-zeros per row.
         */
        template <class SparseMatrixType>
        SparseMatrixType GenerateUniform(typename SparseMatrixType::StorageIndex rows
                                         , typename SparseMatrixType::StorageIndex columns
                                         , typename SparseMatrixType::StorageIndex nnz_per_row) {

            using real_t = typename SparseMatrixType::Scalar;
            using idx_t  = typename SparseMatrixType::StorageIndex;
            using Triplet = Eigen::Triplet<real_t>;
            std::vector<Triplet> ijk;

            // init random engine and dist
            std::uniform_real_distribution<real_t> dist(-1.0, 1.0);
            std::default_random_engine &gen = GLOBAL_RANDOM_ENGINE;

            // make a list of column indices
            std::vector<idx_t> cols;
            for (int j = 0; j < columns; j++) {
                cols.push_back(j);
            }
            std::random_shuffle(cols.begin(), cols.end());

            // shuffle for each row
            idx_t nnz = 0;
            for (idx_t i = 0; i < rows; i++) {
                // select the first nnz
                for (idx_t nnz_i = 0; nnz_i < nnz_per_row; nnz_i++) {
                    // reshuffle and reset nnz
                    if (nnz == cols.size()) {
                        std::random_shuffle(cols.begin(), cols.end());
                        nnz = 0;
                    }
                    idx_t j = cols[nnz++];
                    real_t k = dist(gen);
                    ijk.push_back(Triplet(i,j,k));
                }
            }
        
            SparseMatrixType mat(rows, columns);
            mat.setFromTriplets(ijk.begin(), ijk.end());
            mat.uncompress();
            return mat;
        }

        /**
         * Builds a sparse matrix using data from a MatrixMarket formatted file
         */
        template <class SparseMatrixType>
        SparseMatrixType FromAsciiNonZerosList(
            const std::string &filename
            , bool weighted
            , bool directed
            , bool zero_indexed = false) {
            using real_t = typename SparseMatrixType::Scalar;
            using idx_t  = typename SparseMatrixType::StorageIndex;
            using Triplet = Eigen::Triplet<real_t>;
            std::vector<Triplet> ijk;
            std::ifstream file(filename);
            char line[256];

            // skip comment lines starting with '%'
            do {
                file.getline(line, sizeof(line));
            } while (line[0] == '%');

            // first line is: ROWS COLS NNZ
            std::stringstream ss(line);
            int rows, cols, nnz;
            ss >> rows;
            ss >> cols;
            ss >> nnz;

            // init matrix and reserve triplets
            SparseMatrixType mat(rows, cols);
            ijk.reserve(nnz);

            // continue until no more nnz
            while (!file.eof()) {
                idx_t i, j;
                real_t k;
                file >> i;
                file >> j;
                // some of the inputs are not zero indexed
                if (!zero_indexed) {
                    --i;
                    --j;
                }
                // read weight if applicable
                if (weighted) {
                    file >> k;
                } else {
                    k = 1.0;
                }

                //std::cout << "(" << i << "," << j << "," << k << ")" << std::endl;
                ijk.push_back(Triplet(i,j,k));
                // symmetric matrix?
                if (!directed) {
                    ijk.push_back(Triplet(j,i,k));
                }
            }
            // use triplets to set nnz
            mat.setFromTriplets(ijk.begin(), ijk.end());
            mat.uncompress();
            return mat;
        }
    }

}
