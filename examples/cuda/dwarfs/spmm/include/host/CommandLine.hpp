#pragma once
#include <string>

namespace spmm {
    class CommandLine {
    public:
        CommandLine() {}
        static CommandLine Parse(int argc, char *argv[]) {
            CommandLine cl;
            cl._riscv_path = argv[1];
            cl._kernel_name = argv[2];
            cl._input_path = argv[3];
            cl._input_is_directed = *(argv[4]) == 'y';
            cl._input_is_weighted = *(argv[5]) == 'y';
            cl._input_is_zero_indexed = *(argv[6]) == 'y';
            cl._tgx = atoi(argv[7]);
            cl._tgy = atoi(argv[8]);
            return cl;
        }

        bool input_is_directed() const { return _input_is_directed; }
        bool input_is_weighted() const { return _input_is_weighted; }
        bool input_is_zero_indexed() const { return _input_is_zero_indexed; }
        std::string kernel_name() const { return _kernel_name; }
        std::string riscv_path() const { return _riscv_path; }
        std::string input_path() const { return _input_path; }
        int tgx() const { return _tgx; }
        int tgy() const { return _tgy; }

    private:
        std::string _kernel_name;
        std::string _riscv_path;
        std::string _input_path;
        bool _input_is_directed;
        bool _input_is_weighted;
        bool _input_is_zero_indexed;
        int  _tgx;
        int  _tgy;
    };
}
