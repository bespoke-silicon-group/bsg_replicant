#pragma once
#include <type_traits>
#include <memory>
#include <vector>
#include "HammerBlade.hpp"
#include "EigenSparseMatrix.hpp"
#include "SparseMatrix.hpp"
#include "sparse_matrix_partition.h"

namespace dwarfs {
    template <typename EigenSparseMatrix>
    class PartitionedSparseMatrix {
    public:
        using idx_t = typename EigenSparseMatrix::StorageIndex;
        using real_t = typename EigenSparseMatrix::Scalar;

        struct Partition {
            struct {
                sparse_matrix_partition_t      header;
                sparse_matrix_partition_info_t partinfo_h [64];
            } header_h;
            hb_mc_eva_t header_d;
            int &is_row_major() { return header_h.header.matrix.is_row_major; }
            int &n_major() { return header_h.header.matrix.n_major; }
            int &n_minor() { return header_h.header.matrix.n_minor; }
            int &n_non_zeros() { return header_h.header.matrix.n_non_zeros; }
            kernel_int_ptr_t   &mjr_nnz_ptr  () { return header_h.header.matrix.mjr_nnz_ptr; }
            kernel_int_ptr_t   &mnr_off_ptr  () { return header_h.header.matrix.mnr_off_ptr; }
            kernel_int_ptr_t   &mnr_idx_ptr  () { return header_h.header.matrix.mnr_idx_ptr; }
            kernel_float_ptr_t &val_ptr      () { return header_h.header.matrix.val_ptr; }
            kernel_int_ptr_t   &alg_priv_ptr () { return header_h.header.matrix.alg_priv_ptr; }
            int &partitions() { return header_h.partitions; }
            int &partid()     { return header_h.partid; }
            sparse_matrix_partition_info_t & partinfo(idx_t idx) { return header_h.partinfo[idx]; }
        };
        
        PartitionedSparseMatrix()
            : _hb(HammerBlade::Get())
            , _espm(nullptr) {
        }

        void initializeFromEigenSparseMatrix(
            const std::shared_ptr<EigenSparseMatrix> &espm
            , int partitions) {
            _espm = espm;
            _n_partitions = partitions;
            initOnHostPartitions();            
        }

        void initializePartitionFromEigenSparseMatrix(
            const std::shared_ptr<EigenSparseMatrix> &espm
            , int partitions
            , int part) {
            _espm = esp;
            _n_partitions = partitions;
            initOnHostPartitions();
            initOnePart(part);
        }

        idx_t partition_size(idx_t part) {
            return _partitions_start[part+1]-_partitions_start[part];
        }
        idx_t partition_row_start(idx_t part) {
            return _partitions_start[part];
        }
        idx_t partition_row_stop(idx_t part) {
            return _partitions_start[part+1];
        }
        idx_t partitions() const { return _n_partitions; }

    protected:
        void initOnePart(idx_t part) {
            // select partition to allocate
            Partition *partition = &_partitions[part];
            std::shared_ptr<EigenSparseMatrix> parth = _espm_partitions[part];
            partition->is_row_major() = EigenSparseMatrix::Options == Eigen::RowMajor;
            partition->n_major() = parth->outerSize();
            partition->n_minor() = parth->innerSize();
            partition->n_non_zeros() = parth->nonZeros();
            partition->partitions() = _n_partitions;
            partition->partid() = part;

            // setup partinfo
            for (idx_t p = 0; p < _n_partitions; p++) {
                partition->partinfo(p).major_start = _partitions_start[p];
                partition->partinfo(p).partid_hash = p;
            }
            partition->partinfo(_n_partitions).major_start = _espm->outerSize();
            partition->partinfo(_n_partitions).partid_hash = _n_partitions-1;

            // allocate memory
            idx_t n_majors = _partitions_start[p+1]-_partitions_start[p];
            partition->mjr_nnz_ptr() = alloc_aligned(n_majors * sizeof(idx_t));
            partition->mnr_off_ptr() = alloc_aligned(n_majors * sizeof(idx_t));
            partition->alg_priv_ptr() = alloc_aligned(n_majors * sizeof(idx_t));
            partition->mnr_idx_ptr() = alloc_aligned(partition->n_non_zeros() * sizeof(idx_t));
            partition->val_ptr()     = alloc_aligned(partition->n_non_zeros() * sizeof(real_t));

            // setup writes
            _hb->push_write(partition->mjr_nnz_ptr(), parth->innerNonZeroPtr(), n_majors * sizeof(idx_t));
            _hb->push_write(partition->mnr_off_ptr(), parth->outerIndexPtr(),   n_majors * sizeof(idx_t));
            _hb->push_write(partition->mnr_idx_ptr(), parth->innerIndexPtr(),   partition->n_non_zeros() * sizeof(idx_t));
            _hb->push_write(partition->val_ptr(),     parth->valuePtr(),        partition->n_non_zeros() * sizeof(real_t));

            
            // adjust offsets so that the partitions row data can be indexed by the row id
            partition->mjr_nnz_ptr()  -= partitions->partinfo(p).major_start * sizeof(idx_t);
            partition->mnr_off_ptr()  -= partitions->partinfo(p).major_start * sizeof(idx_t);
            partition->alg_priv_ptr() -= partitions->partinfo(p).major_start * sizeof(idx_t);
        }

        hb_mc_eva_t alloc_aligned(hb_mc_eva_t size) {
            hb_mc_eva_t data_ptr, allocd_ptr;
            _hb->alloc_aligned_pod_cache(size, &data_ptr, &allocd_ptr);
            _allocated.push_back(allocd_ptr);
            return data_ptr;
        }
        
        void initOnHostPartitions() {
            _espm_partitions = eigen_sparse_matrix::PartitionPtr(_espm);
            _partitions_start = eigen_sparse_matrix::PartitionMjrStartPtr(_espm);
            _partitions.resize(partitions());
        }

    protected:
        idx_t                                                 _n_partitions;
        std::vector<Partition>                                _partitions;
        std::vector<idx_t>                                    _partitions_start;
        std::vector<std::shared_ptr<EigenSparseMatrix>>       _espm_partitions;
        std::shared_ptr<EigenSparseMatrix>                    _espm;
        HammerBlade::Ptr                                      _hb;
        std::vector<hb_mc_eva_t>                              _allocated;
    };
}
