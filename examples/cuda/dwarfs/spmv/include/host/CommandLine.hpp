#pragma once
#include <string>
#include <cstdlib>

namespace spmv {
    class CommandLine {
    public:
        CommandLine() {}

        std::string riscv_path() const { return _riscv_path; }
        std::string kernel_name() const { return _kernel_name; }
        int rows() const { return _rows; }
        int cols() const { return _cols; }
        int nnz_per_row() const { return _nnz_per_row; }
        int groups() const { return _groups; }
        int tgx() const { return _tgx; }
        int tgy() const { return _tgy; }

        static CommandLine Parse(int argc, char *argv[]) {
            CommandLine cl;
            cl._riscv_path = argv[1];
            cl._kernel_name = argv[2];
            cl._rows = atoi(argv[3]);
            cl._cols = atoi(argv[4]);
            cl._nnz_per_row = atoi(argv[5]);
            cl._groups = atoi(argv[6]);
            cl._tgx = atoi(argv[7]);
            cl._tgy = atoi(argv[8]);
            return cl;
        }
    private:
        std::string _riscv_path;
        std::string _kernel_name;
        int _rows;
        int _cols;
        int _nnz_per_row;
        int _groups;
        int _tgx;
        int _tgy;
    };
}
