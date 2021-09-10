#include <string>
namespace stream {
    class CommandLine {
    public:
        enum {
            PROGNAME = 0,
            RVPATH,
            KNAME,
            TBLWORDS,
            BLKWORDS,
        };        
        CommandLine() {}
        static CommandLine Parse(int argc, char *argv[]) {
            CommandLine cl;
            cl._riscv_path  = argv[RVPATH];
            cl._kernel_name = argv[KNAME];
            cl._table_words = atoi(argv[TBLWORDS]);
            cl._block_words = atoi(argv[BLKWORDS]);
            return cl;
        }

        const std::string& riscv_path() const { return _riscv_path; }
        const std::string& kernel_name() const { return _kernel_name; }
        int table_words() const { return _table_words; }
        int block_words() const { return _block_words; }
    private:
        std::string  _riscv_path;
        std::string  _kernel_name;
        int          _table_words;
        int          _block_words;
    };
}
