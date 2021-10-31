#pragma once
#include <memory>
#include "EigenSparseMatrix.hpp"
#include "SparseMatrixPartition.hpp"
namespace dwarfs {
    template <typename EigenSparseMatrixPtrType>
    class SparseMatrixProductPartitioner {
    public:
        using _EigenSparseMatrixType = decltype(*std::declval<EigenSparseMatrixPtrType>());
        using EigenSparseMatrixType = typename std::remove_reference<_EigenSparseMatrixType>::type;
        using idx_t  = typename EigenSparseMatrixType::StorageIndex;
        using real_t = typename EigenSparseMatrixType::Scalar;
        using SparseMatrixPartitionType = SparseMatrixPartition<EigenSparseMatrixPtrType>;
        
        SparseMatrixProductPartitioner(
            const EigenSparseMatrixPtrType   & A
            , const EigenSparseMatrixPtrType & B
            , int partfactor
            )
            : _partfactor(partfactor)
            , _A(A)
            , _B(B)
            , _C(eigen_sparse_matrix::__new_helper<EigenSparseMatrixType,EigenSparseMatrixPtrType>::make_ptr(A->rows(), B->cols())) {
            _A_partitions = eigen_sparse_matrix::PartitionMjrPtr(_A, _partfactor);
            _B_partitions = eigen_sparse_matrix::PartitionMnrPtr(_B, _partfactor);
            _C_partition_mjr_start = eigen_sparse_matrix::PartitionStartPtr(_A, _partfactor);
        }

        EigenSparseMatrixPtrType EigenA(idx_t i, idx_t j) {
            return _A_partitions[i];
        }

        std::unique_ptr<SparseMatrixPartitionType> A(idx_t i, idx_t j) {
            auto ptr = std::unique_ptr<SparseMatrixPartitionType>(
                new SparseMatrixPartitionType(
                    _A_partitions[i]
                    , _C_partition_mjr_start[i]
                    , _C_partition_mjr_start[i+1]
                    , j
                    )
                );
            ptr->initialize();
            return ptr;
        }

        EigenSparseMatrixPtrType EigenB(idx_t i, idx_t j) {
            return _B_partitions[j];
        }

        std::unique_ptr<SparseMatrixPartitionType> B(idx_t i, idx_t j) {
            auto ptr = std::unique_ptr<SparseMatrixPartitionType>(
                new SparseMatrixPartitionType(
                    _B_partitions[j]
                    , _C_partition_mjr_start[0]
                    , _C_partition_mjr_start[_partfactor]
                    , j
                    )
                );
            ptr->initialize();
            return ptr;
        }

        std::unique_ptr<SparseMatrixPartitionType> C(idx_t i, idx_t j) {
            auto ptr = std::unique_ptr<SparseMatrixPartitionType>(
                new SparseMatrixPartitionType (
                    _C
                    , _C_partition_mjr_start[i]
                    , _C_partition_mjr_start[i+1]
                    , j
                    )
                );
            ptr->initializeEmptyProduct();
            return ptr;
        }

        idx_t C_major_start(idx_t i, idx_t j) { return _C_partition_mjr_start[i]; }
        idx_t C_major_stop (idx_t i, idx_t j) { return _C_partition_mjr_start[i+1]; }

    protected:
        int _partfactor;
        EigenSparseMatrixPtrType _A;
        EigenSparseMatrixPtrType _B;
        EigenSparseMatrixPtrType _C;
        std::vector<EigenSparseMatrixPtrType> _A_partitions;
        std::vector<EigenSparseMatrixPtrType> _B_partitions;
        std::vector<idx_t> _C_partition_mjr_start;
    };
}
