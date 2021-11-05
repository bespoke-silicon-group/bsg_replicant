#pragma once
#include <type_traits>
#include <memory>
#include "HammerBlade.hpp"
#include "EigenSparseMatrix.hpp"
#include "sparse_matrix.h"

namespace dwarfs {
    template <typename EigenSparseMatrix>
    class SparseMatrix {
        using HammerBlade = hammerblade::host::HammerBlade;
        using Dim = hammerblade::host::HammerBlade;
        using idx_t = typename EigenSparseMatrix::StorageIndex;
        using real_t = typename EigenSparseMatrix::Scalar;
    public:
        SparseMatrix()
            : _hdr_dev(0)
            , _hb(HammerBlade::Get())
            , _espm(nullptr) {
        }

        void initializeFromEigenSparseMatrix(const std::shared_ptr<EigenSparseMatrix> &espm) {
            _espm = espm;

            if (_espm->isCompressed()) {
                _espm->uncompress();
            }

            _hdr_h.is_row_major = EigenSparseMatrix::Options == Eigen::RowMajor;
            _hdr_h.n_major = _espm->outerSize();
            _hdr_h.n_minor = _espm->innerSize();
            _hdr_h.n_non_zeros = espm->nonZeros();

            // allocate row nnz vector
            alloc_aligned(_hdr_h.n_major * sizeof(idx_t), &_hdr_h.mjr_nnz_ptr, &_mjr_nnz_allocd_ptr);
            _hb->push_write(_hdr_h.mjr_nnz_ptr, _espm->innerNonZeroPtr(), _hdr_h.n_major * sizeof(idx_t));
            
            // allocate row offset vector
            alloc_aligned((1+_hdr_h.n_major) * sizeof(idx_t), &_hdr_h.mnr_off_ptr, &_mnr_off_allocd_ptr);
            _hb->push_write(_hdr_h.mnr_off_ptr, _espm->outerIndexPtr(), (1+_hdr_h.n_major) * sizeof(idx_t));

            // allocate col idx vector
            _hdr_h.mnr_idx_ptr = _hb->alloc(_hdr_h.n_non_zeros * sizeof(idx_t));
            _hb->push_write(_hdr_h.mnr_idx_ptr, _espm->innerIndexPtr(), _hdr_h.n_non_zeros * sizeof(idx_t));

            // allocate values vector
            _hdr_h.val_ptr = _hb->alloc(_hdr_h.n_non_zeros * sizeof(real_t));
            _hb->push_write(_hdr_h.val_ptr, _espm->valuePtr(), _hdr_h.n_non_zeros * sizeof(real_t));

            // allocate the header
            _hdr_dev = _hb->alloc(sizeof(sparse_matrix_t));
            _hb->push_write(_hdr_dev, &_hdr_h, sizeof(sparse_matrix_t));
            
        }        

        void initializeEmptyProduct(const std::shared_ptr<EigenSparseMatrix> &A_espm,
                                    const std::shared_ptr<EigenSparseMatrix> &B_espm) {

            _hdr_h.is_row_major = EigenSparseMatrix::Options == Eigen::RowMajor;
            _hdr_h.n_major = A_espm->outerSize();
            _hdr_h.n_minor = B_espm->innerSize();
            _hdr_h.n_non_zeros = 0;

            // allocate row nnz vector
            alloc_aligned(_hdr_h.n_major * sizeof(idx_t), &_hdr_h.mjr_nnz_ptr, &_mjr_nnz_allocd_ptr);
            
            // allocate row offset vector
            alloc_aligned((1+_hdr_h.n_major) * sizeof(idx_t), &_hdr_h.mnr_off_ptr, &_mnr_off_allocd_ptr);

            // allocate private data
            alloc_aligned(_hdr_h.n_major * sizeof(idx_t), &_hdr_h.alg_priv_ptr, &_alg_priv_data_allocd_ptr);
            
            // set col idx vector to nullptr
            _hdr_h.mnr_idx_ptr = 0;
            
            // set values vector to nullptr
            _hdr_h.val_ptr = 0;

            // allocate the header
            _hdr_dev = _hb->alloc(sizeof(sparse_matrix_t));
            _hb->push_write(_hdr_dev, &_hdr_h, sizeof(sparse_matrix_t));
        }

        std::vector<std::vector<std::pair<idx_t, real_t>>> updatePartialEmptyProduct() {
            // re-read the header
            _hb->read(_hdr_dev, &_hdr_h, sizeof(sparse_matrix_t));

            std::cout << "updateEmptyProduct(): output has " << _hdr_h.n_non_zeros << " nonzeros" << std::endl;
            
            std::vector<hb_mc_eva_t> alg_priv_data (_hdr_h.n_major);
            _hb->push_read(_hdr_h.alg_priv_ptr, &alg_priv_data[0], _hdr_h.n_major * sizeof(hb_mc_eva_t));

            std::vector<idx_t> mjr_nnz(_hdr_h.n_major);
            _hb->push_read(_hdr_h.mjr_nnz_ptr, &mjr_nnz[0], _hdr_h.n_major * sizeof(idx_t));

            // sync
            _hb->sync_read();

            std::vector<std::vector<std::pair<idx_t,real_t>>> results;
            for (idx_t mjr = 0; mjr < _hdr_h.n_major; mjr++) {
                std::vector<std::pair<idx_t, real_t>> partials(mjr_nnz[mjr]);
                results.push_back(partials);
            }
            for (idx_t mjr = 0; mjr < _hdr_h.n_major; mjr++) {
                std::cout << "row " << std::dec << mjr << ": " << std::hex << alg_priv_data[mjr] << std::endl;
                _hb->push_read(alg_priv_data[mjr], results[mjr].data(), mjr_nnz[mjr] * sizeof(std::pair<idx_t,real_t>));                
            }
            std::cout << std::dec;
            _hb->sync_read();
            return results;
        }

        std::shared_ptr<EigenSparseMatrix> updateEmptyProduct() {
            // re-read the header
            _hb->read(_hdr_dev, &_hdr_h, sizeof(sparse_matrix_t));

            // make new matrix
            int rows = _hdr_h.is_row_major ? _hdr_h.n_major : _hdr_h.n_minor;
            int cols = _hdr_h.is_row_major ? _hdr_h.n_minor : _hdr_h.n_major;
            _espm = std::shared_ptr<EigenSparseMatrix>(new EigenSparseMatrix(rows, cols));

            // read directly into sparse matrix object
            _espm->reserve(_hdr_h.n_non_zeros);
            _espm->uncompress();
            _hb->push_read(_hdr_h.mnr_off_ptr, _espm->outerIndexPtr(),   _hdr_h.n_major * sizeof(idx_t));
            _hb->push_read(_hdr_h.mjr_nnz_ptr, _espm->innerNonZeroPtr(), _hdr_h.n_major * sizeof(idx_t));
            _hb->push_read(_hdr_h.mnr_idx_ptr, _espm->innerIndexPtr(),   _hdr_h.n_non_zeros * sizeof(idx_t));
            _hb->push_read(_hdr_h.val_ptr,     _espm->valuePtr(),        _hdr_h.n_non_zeros * sizeof(real_t));

            // sync
            _hb->sync_read();
            return _espm;
        }

        hb_mc_eva_t hdr_dev() const { return _hdr_dev; }
        sparse_matrix_t hdr() const { return _hdr_h; }

    private:
        void alloc_aligned(hb_mc_eva_t size, hb_mc_eva_t *data_ptr, hb_mc_eva_t *allocd_ptr) {
            hb_mc_eva_t aligned_to
                = _hb->physical_dimension().x() // vcache columns
                * 2 // north + south
                * _hb->config()->vcache_stripe_words // words/line
                * sizeof(int); // sizeof(word);

            hb_mc_eva_t ptr = _hb->alloc(size + aligned_to);
            hb_mc_eva_t rem = ptr % aligned_to;
            *allocd_ptr = ptr;
            *data_ptr = ptr - rem + aligned_to;
            return;
        }

    private:
        sparse_matrix_t  _hdr_h;
        hb_mc_eva_t      _hdr_dev;
        // we allocate a buffer and align it
        // it requires us to save the allocated pointer
        hb_mc_eva_t      _mjr_nnz_allocd_ptr;
        hb_mc_eva_t      _mnr_off_allocd_ptr;
        hb_mc_eva_t      _alg_priv_data_allocd_ptr;
        HammerBlade::Ptr _hb;
        std::shared_ptr<EigenSparseMatrix> _espm;
    };
}
