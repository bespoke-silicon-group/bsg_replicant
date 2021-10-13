#include <string>
namespace stream {
    class CommandLine {
    public:
        enum {
            PROGNAME = 0,
            RVPATH,
            KNAME,
            TBLWORDS,
            THRDSPGRP,
            BLCKWRDSPTHRD,
        };        
        CommandLine() {}
        static CommandLine Parse(int argc, char *argv[]) {
            CommandLine cl;
            cl._riscv_path  = argv[RVPATH];
            cl._kernel_name = argv[KNAME];
            cl._table_words = atoi(argv[TBLWORDS]);
            cl._threads_per_group = atoi(argv[THRDSPGRP]);
            cl._block_words_per_thread = atoi(argv[BLCKWRDSPTHRD]);
            return cl;
        }

        const std::string& riscv_path() const { return _riscv_path; }
        const std::string& kernel_name() const { return _kernel_name; }
        int table_words() const { return _table_words; }
        int threads_per_group() const { return _threads_per_group; }
        int block_words_per_thread() const { return _block_words_per_thread; }

    private:
        std::string  _riscv_path;
        std::string  _kernel_name;
        int          _table_words;
        int          _threads_per_group;
        int          _block_words_per_thread;
    };
}
