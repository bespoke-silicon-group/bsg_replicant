#include <Eigen/Sparse>
#include <sstream>
#include <fstream>
#pragma once

namespace dwarfs {    
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
}
