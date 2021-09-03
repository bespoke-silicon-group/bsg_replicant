#pragma once
#include "EigenSparseMatrix.hpp"
#include <unordered_map>
#include <vector>
namespace spmm {
    template <class SparseMatrixType>
    class Solver {
    public:
        using real_t = typename SparseMatrixType::Scalar;
        using idx_t  = typename SparseMatrixType::StorageIndex;
        Solver(const SparseMatrixType &A,
               const SparseMatrixType &B):
            _A(A)
            ,_B(B) {
        }

        void solve() {
            // allocate memory for arrays known a-priori
            _C_row_nnz.reserve(_A.rows());
            _C_row_off.reserve(_A.rows());

            // for each row in A...
            for (idx_t Ai = 0; Ai < _A.rows(); ++Ai) {
                solve_row(Ai);
            }
        }

        SparseMatrixType solution() const {
            using Triplet = Eigen::Triplet<real_t, idx_t>;
            std::vector<Triplet> ijk;
            ijk.reserve(_C_vals.size());

            idx_t rows = _A.rows();
            idx_t cols = _B.cols();
            for (idx_t row = 0; row < _C_row_off.size(); ++row) {
                idx_t off = _C_row_off[row];
                idx_t nnz = _C_row_nnz[row];
                for (idx_t nonzero = 0; nonzero < nnz; ++nonzero) {
                    idx_t col = _C_cols[off+nonzero];
                    real_t val = _C_vals[off+nonzero];
                    if (row >= _A.rows() ||
                        col >= _B.cols()) {
                        printf("error: %d %d\n", row, col);
                    }
                    ijk.push_back(Triplet(row, col, val));
                }
            }

            SparseMatrixType mat(rows, cols);
            mat.setFromTriplets(ijk.begin(), ijk.end());
            return mat;
        }

    private:
        void solve_row(idx_t Ai) {
            // solve the solution for row C[i:]
            // by summing scalar-vector products from
            // non-zero entries of A[i:] with B
            idx_t off = _A.outerIndexPtr()[Ai];
            idx_t nnz = _A.innerNonZeroPtr()[Ai];
            // wait on off
            const idx_t *cols = &_A.innerIndexPtr()[off];
            const real_t *vals = &_A.valuePtr()[off];
            std::unordered_map<idx_t, real_t> partials;

            // for each nonzero entry in row A[i:]
            for (idx_t nonzero = 0; nonzero < nnz; ++nonzero) {
                idx_t  Bi  = cols[nonzero];
                real_t Aij = vals[nonzero];
                // calculate Aij * Bi
                add_scalar_row_product(Bi, Aij, partials);
            }
            // insert partials into C
            // this will need to be done atomically in a multithreaded environment
            // qustion: how?
            // answer: we can change the format just a little bit to be pointer based
            idx_t Ci_off = _C_cols.size();
            idx_t Ci_nnz = partials.size();
            _C_row_off.push_back(Ci_off);
            _C_row_nnz.push_back(Ci_nnz);

            for (auto it = partials.begin(); it != partials.end(); it++) {
                _C_cols.push_back(it->first);
                _C_vals.push_back(it->second);
            }
        }

        template <class map_type>
        void add_scalar_row_product(idx_t Bi, real_t Aij, map_type &partials) {
            // compute A[i,j] * B[j:]
            idx_t off = _B.outerIndexPtr()[Bi];
            idx_t nnz = _B.innerNonZeroPtr()[Bi];
            // wait on off
            const idx_t *cols = &_B.innerIndexPtr()[off];
            const real_t *vals = &_B.valuePtr()[off];

            // for each non-zero entry in row B[i:]
            for (idx_t nonzero = 0; nonzero < nnz; ++nonzero) {
                idx_t  Bj  = cols[nonzero];
                real_t Bij = vals[nonzero];
                // compute partial
                real_t Cij_partial = Aij*Bij;
                // search if entry is in partials
                auto it = partials.find(Bj);
                if (it == partials.end()) {
                    // insert new partial
                    partials[Bj]  = Cij_partial;
                } else {                    
                    // add partial to existing entry
                    partials[Bj] += Cij_partial;
                }
            }
        }

    private:
        const SparseMatrixType & _A;
        const SparseMatrixType & _B;
        // output data
        std::vector<idx_t>  _C_row_nnz;
        std::vector<idx_t>  _C_row_off;
        std::vector<idx_t>  _C_cols;
        std::vector<real_t> _C_vals;
    };
}
