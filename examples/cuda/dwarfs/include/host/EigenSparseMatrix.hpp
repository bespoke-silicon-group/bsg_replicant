#include <Eigen/Sparse>
#include <sstream>
#include <fstream>
#include <utility>
#include <vector>
#include <random>
#include <cstdlib>
#include <algorithm>
#include <limits>
#include <fstream>
#include <sstream>
#include <iostream>
#include "Random.hpp"
#pragma once

namespace dwarfs {

    namespace eigen_sparse_matrix {
        /* Compare non-zeros along major axis and return true if they are approximately equal */
        template <class SparseMatrixType>
        bool mjr_equal(SparseMatrixType &A, SparseMatrixType &B, typename SparseMatrixType::StorageIndex mjr) {
            using Index = typename SparseMatrixType::StorageIndex;
            using Scalar = typename SparseMatrixType::Scalar;
            Index nnz_A = A.innerNonZeroPtr()[mjr];
            Index nnz_B = B.innerNonZeroPtr()[mjr];
#ifdef DEBUG_EIGEN_SPARSE_MATRIX
            std::cout << "nnz(A," << mjr << ")=" << nnz_A << std::endl;
            std::cout << "nnz(B," << mjr << ")=" << nnz_B << std::endl;
#endif
            if (nnz_A != nnz_B) {
                return false;
            }

            Index nnz = nnz_A;
            // offsets
            Index off_A = A.outerIndexPtr()[mjr];
            Index off_B = B.outerIndexPtr()[mjr];
            // indices
            Index *idx_A = &A.innerIndexPtr()[off_A];
            Index *idx_B = &B.innerIndexPtr()[off_B];
            // values
            Scalar *val_A = &A.valuePtr()[off_A];
            Scalar *val_B = &B.valuePtr()[off_B];
            // for each non-zero
            for (Index nz = 0; nz < nnz; nz++) {
                // fetch indices
                Index iA = idx_A[nz];
                Index iB = idx_B[nz];
#ifdef DEBUG_EIGEN_SPARSE_MATRIX
                std::cout << "idx(A, " << nz << ")=" << iA << std::endl;
                std::cout << "idx(B, " << nz << ")=" << iB << std::endl;
#endif
                // fetch scalar
                Scalar vA = val_A[nz];
                Scalar vB = val_B[nz];
#ifdef DEBUG_EIGEN_SPARSE_MATRIX
                std::cout << "val(A, " << nz << ")=" << vA << std::endl;
                std::cout << "val(B, " << nz << ")=" << vB << std::endl;
#endif
                if (iA != iB)
                    return false;

                if ((vA-vB) > std::numeric_limits<float>::epsilon())
                    return false;
            }
            return true;
        }

        /* Compare non-zeros along major axis for range of major indices */
        template <class SparseMatrixType>
        bool mjr_range_equal(SparseMatrixType &A
                             , SparseMatrixType &B
                             , typename SparseMatrixType::StorageIndex mjr_lo
                             , typename SparseMatrixType::StorageIndex mjr_hi)
        {
            using Index = typename SparseMatrixType::StorageIndex;
            //using Scalar = typename SparseMatrixType::Scalar;
            if (A.isCompressed())
                A.uncompress();
            if (B.isCompressed())
                B.uncompress();
            for (Index mjr = mjr_lo; mjr < mjr_hi; mjr++) {
#ifdef DEBUG_EIGEN_SPARSE_MATRIX
                std::cout << "comparing row " << mjr << std::endl;
#endif
                if (!mjr_equal(A, B, mjr))
                    return false;
            }
            return true;
        }

        template <class SparseMatrixType>
        void show_metadata(SparseMatrixType &mat) {
            if (mat.isCompressed())
                mat.uncompress();
            std::cout << "# nnz    = " << mat.nonZeros() << std::endl;
            std::cout << "# majors = " << mat.outerSize() << std::endl;
            std::cout << "# minors = " << mat.innerSize() << std::endl;
        }

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

        /* dump offsets data to a file */
        template <class SparseMatrixType>
        void write_offset(SparseMatrixType &mat, const std::string &fname) {
            if (mat.isCompressed())
                mat.uncompress();
            std::ofstream ofs(fname);
            using Index = typename SparseMatrixType::StorageIndex;
            const Index *offs = mat.outerIndexPtr();
            std::cout << "# nnz    = " << mat.nonZeros() << std::endl;
            std::cout << "# majors = " << mat.outerSize() << std::endl;
            std::cout << "# minors = " << mat.innerSize() << std::endl;
            ofs << "major,offset" << std::endl;
            for (Index i = 0; i < mat.outerSize(); i++) {
                Index off = offs[i];
                ofs << i << "," << off << std::endl;
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

        /**
         * Partition a SparseMatrixType into n partitions.
         * Return a vector SparseMatrixType pointers.
         */
        template <typename SparseMatrixType, typename SparseMatrixPtrType>
        struct __new_helper {
            static SparseMatrixPtrType make_ptr(
                typename SparseMatrixType::StorageIndex rows
                , typename SparseMatrixType::StorageIndex cols
                ) {
                return SparseMatrixPtrType(new SparseMatrixType(rows, cols));
            }
        };
        /**
         * Partially specialize for raw pointers.
         */
        template <typename SparseMatrixType>
        struct __new_helper <SparseMatrixType, SparseMatrixType*> {
            static SparseMatrixType* make_ptr(
                typename SparseMatrixType::StorageIndex rows
                , typename SparseMatrixType::StorageIndex cols
                ) {
                return new SparseMatrixType(rows, cols);
            }
        };

        template <typename SparseMatrixPtrType>
        auto PartitionStartPtr(SparseMatrixPtrType mat, int partitions, bool by_mnr = false) {
            // define types
            using SparseMatrixType = typename std::remove_reference<decltype(*mat)>::type;
            using idx_t  = typename SparseMatrixType::StorageIndex;

            // determine size of each partition
            int per_part, rem;
            if (by_mnr) {
                per_part = mat->innerSize()/partitions;
                rem = mat->outerSize()%partitions;
            } else {
                per_part = mat->outerSize()/partitions;
                rem = mat->outerSize()%partitions;
            }

            // determine starting row of each partition
            std::vector<idx_t> partition_start(partitions+1);
            partition_start[0] = 0;
            for (int i = 0; i < partitions; i++) {
                partition_start[i+1] = partition_start[i] + per_part + (i < rem);
            }

            return partition_start;
        }

        template <typename SparseMatrixPtrType>
        std::vector<SparseMatrixPtrType> PartitionMjrPtr(SparseMatrixPtrType mat, int partitions) {
            // define types
            using SparseMatrixType = typename std::remove_reference<decltype(*mat)>::type;
            using idx_t  = typename SparseMatrixType::StorageIndex;
            using real_t = typename SparseMatrixType::Scalar;

            // always uncompress
            mat->uncompress();

            // determine size of each partition
            std::vector<idx_t> partition_start = PartitionStartPtr<SparseMatrixPtrType>(mat, partitions);

            // create each partition
            int i;
            std::vector<SparseMatrixPtrType> partition_pointers(partitions);
            for (i = 0; i < partitions; i++) {
                // allocate partition
                SparseMatrixPtrType part = __new_helper<SparseMatrixType,SparseMatrixPtrType>::make_ptr(mat->rows(), mat->cols());
                // calculate row range
                idx_t row_start = partition_start[i];
                idx_t row_stop  = partition_start[i+1];
                idx_t rows = row_stop - row_start;
                // calculate nonzero range
                idx_t nz_start = mat->outerIndexPtr()[row_start];
                idx_t nz_stop  = mat->outerIndexPtr()[row_stop];
                idx_t nnz = nz_stop - nz_start;
                part->reserve(nnz);
                part->uncompress();
                // set offsets and copy nonzeros
                for (idx_t row = 0; row < row_start; row++) {
                    part->outerIndexPtr()  [row] = 0;
                    part->innerNonZeroPtr()[row] = 0;
                }
                for (idx_t row = row_start; row < row_stop; row++) {
                    part->outerIndexPtr()  [row]  = mat->outerIndexPtr()[row]-nz_start;
                    part->innerNonZeroPtr()[row]  = mat->innerNonZeroPtr()[row];
                }
                for (idx_t row = row_stop; row < mat->outerSize(); row++) {
                    part->outerIndexPtr()  [row] = nnz;
                    part->innerNonZeroPtr()[row] = 0;
                }
                memcpy(part->innerIndexPtr(), mat->innerIndexPtr() + nz_start, nnz * sizeof(idx_t));
                memcpy(part->valuePtr(), mat->valuePtr() + nz_start, nnz * sizeof(real_t));
                // add to output
                partition_pointers[i] = part;
            }

            return partition_pointers;
        }

        template <typename SparseMatrixPtrType>
        std::vector<SparseMatrixPtrType> PartitionMnrPtr(SparseMatrixPtrType mat, int partitions) {
            // define types
            using SparseMatrixType = typename std::remove_reference<decltype(*mat)>::type;
            using idx_t  = typename SparseMatrixType::StorageIndex;
            using real_t = typename SparseMatrixType::Scalar;

            // always uncompress
            mat->uncompress();

            // determine size of each partition
            std::vector<idx_t> partition_start = PartitionStartPtr<SparseMatrixPtrType>(mat, partitions, true);

            // create each partition
            int i;
            std::vector<SparseMatrixPtrType> partition_pointers(partitions);
            for (idx_t p = 0; p < partitions; p++)
                partition_pointers[p] = __new_helper<SparseMatrixType, SparseMatrixPtrType>::make_ptr(mat->rows(), mat->cols());
            
            std::vector<std::vector<idx_t*>>  part_cols(
                partitions
                , std::vector<idx_t*>(mat->outerSize())
                );
            std::vector<std::vector<idx_t>>   part_nnz (                
                partitions
                , std::vector<idx_t>(mat->outerSize(), 0)
                );                
            std::vector<std::vector<real_t*>> part_vals(
                partitions
                , std::vector<real_t*>(mat->outerSize())
                );
            std::vector<std::vector<idx_t>>   part_off (
                partitions+1
                , std::vector<idx_t>(mat->outerSize())
                );

            for (idx_t row = 0; row < mat->outerSize(); row++) {
                part_cols[0][row] = mat->innerIndexPtr() + mat->outerIndexPtr()[row];
                part_vals[0][row] = mat->valuePtr() + mat->outerIndexPtr()[row];                    
            }

            // sort out pointers/nnz
            for (idx_t row = 0; row < mat->outerSize(); row++) {
                idx_t nnz = mat->innerNonZeroPtr()[row];
                idx_t off = mat->outerIndexPtr()[row];
                idx_t col_i = 0;
                idx_t p = 0;
                for (idx_t col_i = 0; col_i < nnz;) {
                    // save the partition base
                    idx_t p_base = col_i;
                    part_cols[p][row] = mat->innerIndexPtr() + off + col_i;
                    part_vals[p][row] = mat->valuePtr() + off + col_i;
                    // scan column indicies they fall into next partition
                    idx_t col = mat->innerIndexPtr()[off + col_i];
                    while (col_i < nnz && col < partition_start[p+1]) {
                        col_i++;
                        col = mat->innerIndexPtr()[off + col_i];
                    }
                    part_nnz[p][row] = col_i-p_base;
                    p++;
                }
            }
            // calculate row offsets in each partition
            for (idx_t p = 0; p < partitions; p++) {
                for (idx_t row = 0; row < mat->outerSize(); row++) {
                    part_off[p][row+1] = part_off[p][row] + part_nnz[p][row];
                }
            }
            // setup each partitions sparse matrix
            for (idx_t p = 0; p < partitions; p++) {
                SparseMatrixPtrType partp = partition_pointers[p];
                idx_t nnz = part_off[p][mat->outerSize()];                
                if (partp->isCompressed()) {
                    partp->reserve(nnz);                   
                    partp->uncompress();
                }

                for (idx_t row = 0; row < mat->outerSize(); row++) {
                    partp->innerNonZeroPtr()[row] = part_nnz[p][row];
                    partp->outerIndexPtr()[row] = part_off[p][row];

                    memcpy(partp->innerIndexPtr() + part_off[p][row]
                           , part_cols[p][row]
                           , part_nnz[p][row] * sizeof(idx_t));
                    
                    memcpy(partp->valuePtr() + part_off[p][row]
                           , part_vals[p][row]
                           , part_nnz[p][row] * sizeof(real_t));
                }
                partp->innerNonZeroPtr()[mat->outerSize()] = nnz;
            }
            return partition_pointers;
        }        
    }
}
