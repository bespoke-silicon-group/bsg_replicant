#pragma once
#include <type_traits>
#include <memory>
#include <vector>
#include "HammerBlade.hpp"
#include "EigenSparseMatrix.hpp"
#include "SparseMatrix.hpp"
#include "sparse_matrix_partition.h"

namespace dwarfs {

    template <typename EigenSparseMatrixPtrType>
    class SparseMatrixPartition {
    public:
        using _EigenSparseMatrixType = decltype(*std::declval<EigenSparseMatrixPtrType>());
        using EigenSparseMatrixType = typename std::remove_reference<_EigenSparseMatrixType>::type;
        using idx_t  = typename EigenSparseMatrixType::StorageIndex;
        using real_t = typename EigenSparseMatrixType::Scalar;
        using HammerBlade = hammerblade::host::HammerBlade;
        SparseMatrixPartition(const EigenSparseMatrixPtrType & eigen_spmm
                              , idx_t mjr_start
                              , idx_t mjr_stop
                              , idx_t mnr_id)
            : _hammerblade(HammerBlade::Get())
            , _eigen_spmm(eigen_spmm) {
            is_row_major() = EigenSparseMatrixType::Options == Eigen::RowMajor;
            n_major() = _eigen_spmm->outerSize();
            n_minor() = _eigen_spmm->innerSize();
            n_non_zeros() = _eigen_spmm->nonZeros();
            major_start() = mjr_start;
            major_stop()  = mjr_stop;
            minor_id()    = mnr_id;
        }

        // getters/setters
        EigenSparseMatrixType & eigen_spmm() { return _eigen_spmm; }
        int &is_row_major() { return _header_h.matrix.is_row_major; }
        int &n_major() { return _header_h.matrix.n_major; }
        int rows() { return EigenSparseMatrixType::Options == Eigen::RowMajor ? n_major() : n_minor(); }
        int cols() { return EigenSparseMatrixType::Options == Eigen::RowMajor ? n_minor() : n_major(); }
        int &n_minor() { return _header_h.matrix.n_minor; }
        int &n_non_zeros() { return _header_h.matrix.n_non_zeros; }
        kernel_int_ptr_t   &mjr_nnz_ptr  () { return _header_h.matrix.mjr_nnz_ptr; }
        kernel_int_ptr_t   &mnr_off_ptr  () { return _header_h.matrix.mnr_off_ptr; }
        kernel_int_ptr_t   &mnr_idx_ptr  () { return _header_h.matrix.mnr_idx_ptr; }
        kernel_float_ptr_t &val_ptr      () { return _header_h.matrix.val_ptr; }
        kernel_int_ptr_t   &alg_priv_ptr () { return _header_h.matrix.alg_priv_ptr; }
        sparse_matrix_partition_info_t & partinfo() { return _header_h.partinfo; }
        int & major_start () { return _header_h.partinfo.major_start; }
        int & major_stop  () { return _header_h.partinfo.major_stop; }
        int & minor_id    () { return _header_h.partinfo.minor_id; }
        int   part_n_major     () { return major_stop()-major_start(); }
        hb_mc_eva_t & hdr_dev() { return _header_d; }

        /* initialize an empty product matrix */
        void initializeMjr() {
            // allocate memory
            mjr_nnz_ptr()  = alloc(part_n_major() * sizeof(idx_t)) - major_start() * sizeof(idx_t);
            mnr_off_ptr()  = alloc((1+part_n_major()) * sizeof(idx_t)) - major_start() * sizeof(idx_t);
            // write nnz
            push_write(
                mjr_nnz_ptr() + major_start() * sizeof(idx_t)
                , _eigen_spmm->innerNonZeroPtr() + major_start()
                , part_n_major() * sizeof(idx_t)
                );
            // write offsets
            push_write(
                mnr_off_ptr() + major_start() * sizeof(idx_t)
                , _eigen_spmm->outerIndexPtr() + major_start()
                , part_n_major() * sizeof(idx_t)
                );
            write (mnr_off_ptr() + (major_start() + part_n_major()) * sizeof(idx_t)
                   ,&n_non_zeros()
                   ,sizeof(idx_t)
                );
        }

        /* allocate buffer for algorithm private data */
        void initializeAlgPriv() {
            alg_priv_ptr() = alloc(part_n_major() * sizeof(idx_t)) - major_start() * sizeof(idx_t);
        }

        /* initialize the sparse matrix header */
        void initializeHdr() {
            // allocate header
            hdr_dev()   = alloc(sizeof(_header_h));
            // write header
            push_write(
                hdr_dev()
                , &_header_h
                , sizeof(_header_h)
                );
        }

        /* partition on device */
        void initializeMnr() {
            // allocate memory
            mnr_idx_ptr()  = alloc(_eigen_spmm->nonZeros() * sizeof(idx_t));
            val_ptr()      = alloc(_eigen_spmm->nonZeros() * sizeof(real_t));

            // write nonzero indices
            push_write(
                mnr_idx_ptr()
                , _eigen_spmm->innerIndexPtr()
                , _eigen_spmm->nonZeros() * sizeof(idx_t)
                );
            push_write(
                val_ptr()
                , _eigen_spmm->valuePtr()
                , _eigen_spmm->nonZeros() * sizeof(real_t)
                );
        }

        /* initialize partition on device*/
        void initialize() {
            initializeMjr();
            initializeMnr();
            initializeHdr();
        }

        /* initialize empty product partition on device */
        void initializeEmptyProduct() {
            if (_eigen_spmm->isCompressed()) {
                _eigen_spmm->uncompress();
            }
            initializeMjr();
            initializeAlgPriv();
            initializeHdr();
        }

        /* update empty product partition */
        EigenSparseMatrixPtrType updateEmptyProduct() {
            // update the header
            read(&_header_h, hdr_dev(), sizeof(_header_h));
            // make eigen sparse matrix
            if (!_eigen_spmm->isCompressed()) {
                _eigen_spmm->makeCompressed();
            }
            std::cout << "found " << n_non_zeros() << " non zeros" << std::endl;
            _eigen_spmm->reserve(n_non_zeros());
            _eigen_spmm->uncompress();

            std::cout << "copying "
                      << "to "   << std::hex << (_eigen_spmm->outerIndexPtr() + major_start()) << " "
                      << "from " << std::hex << mnr_off_ptr() + major_start() * sizeof(idx_t)  << " "
                      << std::endl;
            push_read(
                _eigen_spmm->outerIndexPtr() + major_start()
                , mnr_off_ptr() + major_start() * sizeof(idx_t)
                , part_n_major() * sizeof(idx_t)
                );
            std::cout << "copying "
                      << "to "   << std::hex << (_eigen_spmm->innerNonZeroPtr() + major_start()) << " "
                      << "from " << std::hex << mjr_nnz_ptr() + major_start() * sizeof(idx_t)  << " "
                      << std::endl;
            push_read(
                _eigen_spmm->innerNonZeroPtr() + major_start()
                ,mjr_nnz_ptr() + major_start() * sizeof(idx_t)
                , part_n_major() * sizeof(idx_t)
                );
            std::cout << "copying "
                      << "to "   << std::hex << (_eigen_spmm->innerIndexPtr())<< " "
                      << "from " << std::hex << mnr_idx_ptr()  << " "
                      << std::endl;
            push_read(
                _eigen_spmm->innerIndexPtr()
                , mnr_idx_ptr()
                , n_non_zeros() * sizeof(idx_t)
                );
            std::cout << "copying "
                      << "to "   << std::hex << (_eigen_spmm->valuePtr())<< " "
                      << "from " << std::hex << val_ptr()  << " "
                      << std::endl;
            push_read(
                _eigen_spmm->valuePtr()
                , val_ptr()
                , n_non_zeros() * sizeof(real_t)
                );
            _hammerblade->sync_read();
            for (idx_t row = 0; row < major_start(); row++) {
                _eigen_spmm->outerIndexPtr() [row] = 0;
                _eigen_spmm->innerNonZeroPtr() [row] = 0;
            }
            for (idx_t row = major_stop(); row < _eigen_spmm->outerSize(); row++) {
                _eigen_spmm->outerIndexPtr() [row] = _eigen_spmm->nonZeros();
                _eigen_spmm->innerNonZeroPtr() [row] = 0;
            }
            std::cout << std::dec;
            eigen_sparse_matrix::write_matrix(*_eigen_spmm, "dummy.txt");
            return _eigen_spmm;
        }

        /* allocate memory */
        hb_mc_eva_t alloc(hb_mc_eva_t size) {
            hb_mc_eva_t aligned, allocated;
            _hammerblade->alloc_aligned_pod_cache(size, &aligned, &allocated);
            _allocated.push_back(allocated);
            return aligned;
        }

        // push write
        void push_write(hb_mc_eva_t dst, const void *src, size_t size) {
            _hammerblade->push_write(dst, src, size);
        }
        // write
        void write(hb_mc_eva_t dst, const void *src, size_t size) {
            _hammerblade->write(dst, src, size);
        }
        // push read
        void push_read(void *dst, hb_mc_eva_t src, size_t size) {
            _hammerblade->push_read(src, dst, size);
        }
        // read
        void read(void *dst, hb_mc_eva_t src, size_t size) {
            _hammerblade->read(src, dst, size);
        }

    private:
        HammerBlade::Ptr          _hammerblade;
        EigenSparseMatrixPtrType  _eigen_spmm;
        sparse_matrix_partition_t _header_h;
        hb_mc_eva_t               _header_d;
        std::vector<hb_mc_eva_t>  _allocated;
    };
}
