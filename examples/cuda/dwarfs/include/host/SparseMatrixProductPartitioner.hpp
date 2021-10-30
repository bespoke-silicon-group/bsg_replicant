#pragma once
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
        
        SparseMatrixProductPartitioner(
            const EigenSparseMatrixPtrType   & A
            , const EigenSparseMatrixPtrType & B
            , int partfactor
            )
            : _A(A)
            , _B(B)
            , _partfactor(partfactor) {
            _A_partitions = eigen_sparse_matrix::PartitionMjrPtr(_A, _partfactor);
            _B_partitions = eigen_sparse_matrix::PartitionMnrPtr(_B, _partfactor);
            _C_partition_mjr_start = eigen_sparse_matrix::PartitionStartPtr(_A, _partfactor);
        }

        int _partfactor;
        EigenSparseMatrixPtrType _A;
        EigenSparseMatrixPtrType _B;
        std::vector<EigenSparseMatrixPtrType> _A_partitions;
        std::vector<EigenSparseMatrixPtrType> _B_partitions;
        std::vector<idx_t> _C_partition_mjr_start;
    };
}
