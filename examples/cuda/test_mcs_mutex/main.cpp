#include "bsg_manycore_regression.h"
#include "CommandLine.hpp"
#include "HammerBlade.hpp"

using namespace test_mcs_mutex;
using namespace hammerblade::host;

int MutexMain(int argc, char *argv[])
{
    CommandLine cl = CommandLine::Parse(argc, argv);
    HammerBlade::Ptr hb = HammerBlade::Get();
    bsg_pr_info("riscv-path: %s\n", cl.riscv_path().c_str());
    bsg_pr_info("lock-type: %s\n", cl.lock_type().c_str());
    bsg_pr_info("threads: (%d,%d)\n", cl.tx(), cl.ty());
    bsg_pr_info("critical region length: %d\n", cl.crl());
    bsg_pr_info("non-critical region length: %d\n", cl.ncrl());
    bsg_pr_info("iters: %d\n", cl.iters());
    hb->load_application(cl.riscv_path());
    bsg_pr_info("launching kernel\n");

    std::string kname = cl.lock_type()
        == "simple"
        ? "test_simple_mutex"
        : "test_mcs_mutex";
    
    hb->push_job(Dim(1,1), Dim(cl.tx(),cl.ty()), kname);
    hb->exec();
    hb->close();
    return HB_MC_SUCCESS;
}

declare_program_main("Mutex", MutexMain);
