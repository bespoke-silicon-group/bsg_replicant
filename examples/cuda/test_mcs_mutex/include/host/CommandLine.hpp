#pragma once
#include <string>
namespace test_mcs_mutex {
    class CommandLine {
    public:
        enum {
            RVPATH = 1,
            KNAME,
            LOCK_TYPE,
            THREADS,
            CRL,
            NCRL,
            ITERS,
        };
        CommandLine() {}
        static CommandLine Parse(int argc, char *argv[]) {
            CommandLine cl;
            cl._rvpath = argv[RVPATH];
            cl._kname = argv[KNAME];
            cl._threads = atoi(argv[THREADS]);
            cl._crl = atoi(argv[CRL]);
            cl._ncrl = atoi(argv[NCRL]);
            cl._iters = atoi(argv[ITERS]);
            cl._lock_type = argv[LOCK_TYPE];
            return cl;
        }
        int threads() const { return _threads; }
        int crl() const { return _crl; }
        int ncrl() const { return _ncrl; }
        int iters() const { return _iters; }
        const std::string & riscv_path() const { return _rvpath; }
        const std::string & kernel_name() const { return _kname; }
        const std::string & lock_type() const { return _lock_type; }
    private:
        std::string _rvpath;
        std::string _kname;
        std::string _lock_type;
        int _threads;
        int _crl; // critical region length
        int _ncrl; // non-critical region length
        int _iters; // times to iterate
    };
}
