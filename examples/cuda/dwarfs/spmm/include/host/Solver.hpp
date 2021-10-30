#pragma once
#include "EigenSparseMatrix.hpp"
#include <unordered_map>
#include <vector>
#include <array>
#include <sstream>
namespace spmm {
    template <class SparseMatrixType>
    class Solver {
    public:
        typedef enum {
            ROW_C = 0,
            NNZ_A,
            NNZ_B,
            NNZ_C,
            FLOPS,
            FMUL,
            FADD,
            LOOKUPS,
            INSERTIONS,
            UPDATES,
            N_FIELDS,
        } SolverSolveRowStatsID;

        typedef enum {
            B_IDX,
            B_VISITED,
            B_NNZ,
            B_NFIELDS,
        } BStatsID;

        typedef enum {
            A_IDX,
            A_VISITED,
            A_NNZ,
            A_NFIELDS,
        } AStatsID;
        
        using real_t = typename SparseMatrixType::Scalar;
        using idx_t  = typename SparseMatrixType::StorageIndex;
        Solver(const SparseMatrixType &A,
               const SparseMatrixType &B):
            _A(A)
            ,_B(B) {
            // solve row stats
            _solve_row_stats_header = {
                "C_row"
                , "A_nonzeros"
                , "B_nonzeros"
                , "C_nonzeros"
                , "ops_fp"
                , "ops_fmul"
                , "ops_fadd"
                , "lookups"
                , "insertions"
                , "updates"
            };
            _solve_row_stats.resize(A.rows());

            // A stats
            _A_stats_header = {
                "A"
                , "A_visited"
                , "A_nnz"
            };
            _A_stats.resize(A.rows());            

            // B stats
            _B_stats_header = {
                "B"
                , "B_visited"
                , "B_nnz"
            };
            _B_stats.resize(B.rows());

            // allocate memory for arrays known a-priori
            _C_row_nnz.reserve(_A.rows());
            _C_row_off.reserve(_A.rows());
            _B_idx_trace.resize(_A.rows());
        }

        void solve() {
            solve_range(0, _A.rows());
        }

        void solve_range(idx_t start, idx_t stop) {
            _start = start;
            _stop = stop;
            for (idx_t Ai = start; Ai < stop; Ai++) {
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

            // zero out stats
            _solve_row_stats[Ai].fill(0);
            _solve_row_stats[Ai][SolverSolveRowStatsID::ROW_C] = Ai;
            _solve_row_stats[Ai][SolverSolveRowStatsID::NNZ_A] = nnz;

            // A stats
            _A_stats[Ai][AStatsID::A_IDX] = Ai;
            _A_stats[Ai][AStatsID::A_VISITED]++;
            _A_stats[Ai][AStatsID::A_NNZ] = nnz;

            // for each nonzero entry in row A[i:]
            for (idx_t nonzero = 0; nonzero < nnz; ++nonzero) {
                idx_t  Bi  = cols[nonzero];
                real_t Aij = vals[nonzero];
                // calculate Aij * Bi
                add_scalar_row_product(Ai, Bi, Aij, partials);
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

            _solve_row_stats[Ai][SolverSolveRowStatsID::NNZ_C] = Ci_nnz;
        }

    private:
        template <class map_type>
        void add_scalar_row_product(idx_t Ai, idx_t Bi, real_t Aij, map_type &partials) {
            // compute A[i,j] * B[j:]
            idx_t off = _B.outerIndexPtr()[Bi];
            idx_t nnz = _B.innerNonZeroPtr()[Bi];
            // B stats
            _B_stats[Bi][BStatsID::B_IDX] = Bi;
            _B_stats[Bi][BStatsID::B_VISITED]++;
            _B_stats[Bi][BStatsID::B_NNZ] = nnz;            
            // wait on off
            const idx_t *cols = &_B.innerIndexPtr()[off];
            const real_t *vals = &_B.valuePtr()[off];
            _solve_row_stats[Ai][SolverSolveRowStatsID::NNZ_B] += nnz;
            // for each non-zero entry in row B[i:]
            for (idx_t nonzero = 0; nonzero < nnz; ++nonzero) {
                idx_t  Bj  = cols[nonzero];
                // add trace
                _B_idx_trace[Ai].push_back(Bj);
                real_t Bij = vals[nonzero];
                // compute partial
                real_t Cij_partial = Aij*Bij;
                _solve_row_stats[Ai][SolverSolveRowStatsID::FLOPS]++;
                _solve_row_stats[Ai][SolverSolveRowStatsID::FMUL]++;
                _solve_row_stats[Ai][SolverSolveRowStatsID::LOOKUPS]++;
                // search if entry is in partials
                auto it = partials.find(Bj);
                if (it == partials.end()) {
                    // insert new partial
                    partials[Bj]  = Cij_partial;
                    _solve_row_stats[Ai][SolverSolveRowStatsID::INSERTIONS]++;
                } else {
                    // add partial to existing entry
                    partials[Bj] += Cij_partial;
                    _solve_row_stats[Ai][SolverSolveRowStatsID::FLOPS]++;
                    _solve_row_stats[Ai][SolverSolveRowStatsID::FADD]++;
                    _solve_row_stats[Ai][SolverSolveRowStatsID::UPDATES]++;
                }
            }
        }

    private:
        template <typename Chdr, typename Cstats>
        std::string stats_str(const Chdr & hdr, const Cstats &stats) const {
            std::stringstream ss;
            // header line
            for (const std::string &h : hdr) {
                ss << h << ",";
            }
            ss << "\n";
            // stats for each line
            for (idx_t i = _start; i < _stop; i++) {
                const auto &sline = stats[i];
                for (auto st : sline) {
                    ss << st << ",";
                }
                ss << "\n";                
            }
            return ss.str();
        }

    public:
        std::string solve_row_stats() const {
            return stats_str(_solve_row_stats_header, _solve_row_stats);
        }

        std::string A_stats() const {
            return stats_str(_A_stats_header, _A_stats);
        }

        std::string B_stats() const {
            return stats_str(_B_stats_header, _B_stats);
        }

        const std::vector<idx_t>& row_idx_trace(idx_t row) const {
            return _B_idx_trace[row];
        }

    private:
        const SparseMatrixType & _A;
        const SparseMatrixType & _B;
        idx_t _start;
        idx_t _stop;
        // output data
        std::vector<idx_t>  _C_row_nnz;
        std::vector<idx_t>  _C_row_off;
        std::vector<idx_t>  _C_cols;
        std::vector<real_t> _C_vals;

        // statistics
        std::array<std::string, SolverSolveRowStatsID::N_FIELDS>
        _solve_row_stats_header;

        std::vector<std::array<int, SolverSolveRowStatsID::N_FIELDS>>
        _solve_row_stats;

        // A stats
        std::array<std::string, AStatsID::A_NFIELDS>
        _A_stats_header;
        std::vector<std::array<int, AStatsID::A_NFIELDS>>
        _A_stats;

        // B stats
        std::array<std::string, BStatsID::B_NFIELDS>
        _B_stats_header;
        std::vector<std::array<int, BStatsID::B_NFIELDS>>
        _B_stats;

        // trace
        std::vector<std::vector<idx_t>> _B_idx_trace;
    };
}
