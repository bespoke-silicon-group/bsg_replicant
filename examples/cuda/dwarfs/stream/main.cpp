#include "bsg_manycore_regression.h"
#include "CommandLine.hpp"
#include "HammerBlade.hpp"

using namespace stream;
using namespace hammerblade::host;

typedef struct done {
    int words[32];
} done_t ;

static hb_mc_eva_t alloc_align(hb_mc_eva_t size)
{
    HammerBlade::Ptr hb = HammerBlade::Get();
    hb_mc_eva_t align
        = hb->config()->vcache_stripe_words
        * sizeof(int)
        * hb->config()->pod_shape.x
        * 2;
    
    hb_mc_eva_t ptr = hb->alloc(size + align);
    hb_mc_eva_t rem = ptr % align;
    ptr += align;
    ptr -= rem;
    return ptr;
}

int Stream(int argc, char *argv[])
{
    CommandLine cl = CommandLine::Parse(argc, argv);

    HammerBlade::Ptr hb = HammerBlade::Get();
    bsg_pr_info("RISCV path = %s\n", cl.riscv_path().c_str());
    bsg_pr_info("Kernel name = %s\n", cl.kernel_name().c_str());
    bsg_pr_info("Table words = %d\n", cl.table_words());
    bsg_pr_info("Threads per group = %d\n", cl.threads_per_group());
    bsg_pr_info("Block words per thread = %d\n", cl.block_words_per_thread());
    bsg_pr_info("Tile-group Dim = (%2d,%2d)\n"
                , hb->physical_dimension().x()
                , hb->physical_dimension().y());
    
    hb->load_application(cl.riscv_path());

    // prep vectors
    bsg_pr_info("Initializing input\n");
    hb_mc_eva_t A_dev = alloc_align(sizeof(int) * cl.table_words());
    hb_mc_eva_t B_dev = alloc_align(sizeof(int) * cl.table_words());
    hb_mc_eva_t C_dev = alloc_align(sizeof(int) * cl.table_words());

    // launch kernels
    bsg_pr_info("Launching kernel\n");
    hb->push_job(Dim(1,1), hb->physical_dimension()
                 , "read"
                 , A_dev);

    hb->trace(true);    
    hb->exec();
    hb->trace(false);

    // close
    bsg_pr_info("Done, cleaning up\n");
    hb->close();
    return HB_MC_SUCCESS;
}

declare_program_main("Stream", Stream);
