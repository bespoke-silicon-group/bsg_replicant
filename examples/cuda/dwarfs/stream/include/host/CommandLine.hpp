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
            GRPS,
            TGX,
            TGY
        };        
        CommandLine() {}
        static CommandLine Parse(int argc, char *argv[]) {
            CommandLine cl;
            cl._riscv_path  = argv[RVPATH];
            cl._kernel_name = argv[KNAME];
            cl._table_words = atoi(argv[TBLWORDS]);
            cl._block_words = atoi(argv[BLKWORDS]);
            cl._groups      = atoi(argv[GRPS]);
            cl._tgx         = atoi(argv[TGX]);
            cl._tgy         = atoi(argv[TGY]);
            return cl;
        }

        const std::string& riscv_path() const { return _riscv_path; }
        const std::string& kernel_name() const { return _kernel_name; }
        int table_words() const { return _table_words; }
        int block_words() const { return _block_words; }
        int tgx() const { return _tgx; }
        int tgy() const { return _tgy; }
        int groups() const { return _groups; }
    private:
        std::string  _riscv_path;
        std::string  _kernel_name;
        int          _table_words;
        int          _block_words;
        int          _groups;
        int          _tgx;
        int          _tgy;
    };
}
