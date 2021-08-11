#include <cstdio>
#include "bsg_manycore_regression.h"
#include "HammerBlade.hpp"
#include "CommandLine.hpp"

using namespace stride;
using namespace std;
using namespace hammerblade::host;

std::vector<hb_mc_eva_t> setup_A(const CommandLine &cl, hb_mc_eva_t A_dev)
{
    HammerBlade::Ptr hb = HammerBlade::Get();
    std::vector<hb_mc_eva_t> A(cl.table_words());    
    for (int i = 0; i < cl.table_words(); i++) {
        A[i] = A_dev + 4*((i + cl.stride_words())%cl.table_words());
    }

    return A;
}

int StrideMain(int argc, char *argv[])
{
    CommandLine cl = CommandLine::parse(argc, argv);

    HammerBlade::Ptr hb = HammerBlade::Get();
    hb->load_application(cl.riscv_path());

    printf("Resetting barrier\n");
    hb->push_job(hb->physical_dimension(), Dim(1,1), "reset_barrier");
    hb->exec();

    // align allocation to stripe size * number of stripes
    hb_mc_eva_t align
        = hb->config()->vcache_stripe_words
        * sizeof(int)
        * hb->config()->pod_shape.x
        * 2;

    hb_mc_eva_t A_dev = hb->alloc(cl.table_words()*sizeof(int)
                                  + align);
    printf("Aligning 0x%08x to 0x%08x\n"
           , A_dev
           , align);

    hb_mc_eva_t rem = A_dev % align;
    A_dev += align;
    A_dev -= rem;

    printf("Table of %8d words allocated at 0x%08x\n"
           , cl.table_words()
           , A_dev);

    std::vector<hb_mc_eva_t> A_host = setup_A(cl, A_dev);

    hb->push_write(A_dev, &A_host[0], cl.table_words() * sizeof(int));
    hb->sync_write();

    printf("Launching stride kernel with x = %2d, y = %2d, group = (X=%2d,Y=%2d)\n"
           , cl.tile_x()
           , cl.tile_y()
           , hb->physical_dimension().x()
           , hb->physical_dimension().y());

    hb->push_job(Dim(1,1), hb->physical_dimension(), cl.kernel_name()
                 , A_dev
                 , cl.tile_x()
                 , cl.tile_y()
                 , cl.loads_per_core());
    hb->exec();
    hb->close();

    return HB_MC_SUCCESS;
}

declare_program_main("Stride", StrideMain);
