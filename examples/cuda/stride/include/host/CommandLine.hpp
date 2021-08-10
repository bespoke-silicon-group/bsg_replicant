#pragma once
#include <string>
#include <stdexcept>

namespace stride {
    class CommandLine {
    public:
        CommandLine() {}

        static CommandLine parse(int argc, char *argv[]) {
            CommandLine cl;
            int expect = 7;            
            if (argc < expect) {
                throw std::runtime_error("Requires " + std::to_string(expect) + " arguments");
            }

            int idx = 1;
            cl._riscv_path     = argv[idx++];
            cl._kernel_name    = argv[idx++];
            cl._table_words    = atoi(argv[idx++]);
            cl._loads_per_core = atoi(argv[idx++]);
            cl._ilp            = atoi(argv[idx++]);
            cl._stride_words   = atoi(argv[idx++]);
            cl._x              = atoi(argv[idx++]);
            cl._y              = atoi(argv[idx++]);
            return cl;
        }

        int table_words()    const { return _table_words; }
        int loads_per_core() const { return _loads_per_core; }
        int ilp()            const { return _ilp; }
        int stride_words()   const { return _stride_words; }
        int tile_x()              const { return _x; }
        int tile_y()              const { return _y; }
        std::string riscv_path() const { return _riscv_path; }
        std::string kernel_name() const { return _kernel_name; }

    private:
        int _table_words;
        int _loads_per_core;
        int _ilp;
        int _stride_words;
        int _x;
        int _y;

        std::string _riscv_path;
        std::string _kernel_name;
    };
}
