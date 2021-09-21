#pragma once
#include <string>

namespace spmm {
    class CommandLineBase {
    public:
        enum {
            EXE,
            RVPATH,
            KNAME,
            INPUT,
            DIRECTED,
            WEIGHTED,
            ZERO_INDEXED,
            NARGS,
        };
        CommandLineBase() {}
        static CommandLineBase Parse(int argc, char *argv[]) {
            CommandLineBase cl;
            cl._riscv_path = argv[RVPATH];
            cl._kernel_name = argv[KNAME];
            cl._input_path = argv[INPUT];
            cl._input_is_directed = *(argv[DIRECTED]) == 'y';
            cl._input_is_weighted = *(argv[WEIGHTED]) == 'y';
            cl._input_is_zero_indexed = *(argv[ZERO_INDEXED]) == 'y';
            return cl;
        }

        bool input_is_directed() const { return _input_is_directed; }
        bool input_is_weighted() const { return _input_is_weighted; }
        bool input_is_zero_indexed() const { return _input_is_zero_indexed; }
        std::string kernel_name() const { return _kernel_name; }
        std::string riscv_path() const { return _riscv_path; }
        std::string input_path() const { return _input_path; }

    protected:
        std::string _kernel_name;
        std::string _riscv_path;
        std::string _input_path;
        bool _input_is_directed;
        bool _input_is_weighted;
        bool _input_is_zero_indexed;
    };

    class SpMMCommandLine : public CommandLineBase {
    public:
        enum {
            TGX = CommandLineBase::NARGS,
            TGY,
        };
        // constructor
        SpMMCommandLine(const CommandLineBase &base) : CommandLineBase(base) {}        
        static SpMMCommandLine Parse(int argc, char *argv[]) {
            SpMMCommandLine cl = CommandLineBase::Parse(argc, argv);
            cl._tgx = atoi(argv[TGX]);
            cl._tgy = atoi(argv[TGY]);
            return cl;
        }
        // getters
        int tgx() const { return _tgx; }
        int tgy() const { return _tgy; }

    private:
        int _tgx;
        int _tgy;
    };

    class SolveRowCommandLine : public CommandLineBase {
    public:
        enum {
            ROW = CommandLineBase::NARGS,
        };
        // constructor
        SolveRowCommandLine(const CommandLineBase &base) : CommandLineBase(base) {}        
        static SolveRowCommandLine Parse(int argc, char *argv[]) {
            SolveRowCommandLine cl = CommandLineBase::Parse(argc, argv);
            cl._row = atoi(argv[ROW]);
            return cl;
        }
        // getters
        int row() const { return _row; }

    private:
        int _row;
    };
        
}
