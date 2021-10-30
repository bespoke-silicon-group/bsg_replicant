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
            SPMM_NARGS,
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

    class SpMMAbrevCommandLine : public SpMMCommandLine {
    public:
        enum {
            ROW_BASE = SpMMCommandLine::SPMM_NARGS,
            ROWS,            
        };
        // constructor
        SpMMAbrevCommandLine(const SpMMCommandLine &base) : SpMMCommandLine(base) {}        
        static SpMMAbrevCommandLine Parse(int argc, char *argv[]) {
            SpMMAbrevCommandLine cl = SpMMCommandLine::Parse(argc, argv);
            cl._row_base = atoi(argv[ROW_BASE]);
            cl._rows     = atoi(argv[ROWS]);
            return cl;
        }
        // getters
        int row_base() const { return _row_base; }
        int rows() const { return _rows; }

    private:
        int _row_base;
        int _rows;
    };

    class SpMMPartitionCommandLine : public SpMMCommandLine {
    public:
        enum {
            PARTFACTOR = SpMMCommandLine::SPMM_NARGS,
            PARTITION,            
        };
        // constructor
        SpMMPartitionCommandLine(const SpMMCommandLine &base) : SpMMCommandLine(base) {}        
        static SpMMPartitionCommandLine Parse(int argc, char *argv[]) {
            SpMMPartitionCommandLine cl = SpMMCommandLine::Parse(argc, argv);
            cl._partfactor = atoi(argv[PARTFACTOR]);

            std::string partition = argv[PARTITION];
            sscanf(partition.c_str()
                   , "%dx%d"
                   , &cl._partition_i
                   , &cl._partition_j);
            
            return cl;
        }
        // getters
        int partfactor() const  { return _partfactor; }
        int partition_i() const { return _partition_i; }
        int partition_j() const { return _partition_j; }

    private:
        int _partfactor;
        int _partition_i;
        int _partition_j;
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
