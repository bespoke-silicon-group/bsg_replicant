#include <bsg_manycore_regression.h>
#include <bsg_manycore.h>
#include <bsg_manycore_cuda.h>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <set>

hb_mc_manycore_t mc = {};
hb_mc_config_t *cfg = &mc.config;
const char *binary = nullptr;

typedef struct {
    int pod_x;
    int pod_y;
    int tile_x;
    int tile_y;
    int pc;
} test_t;

#define DECLARE_TEST(pod_x, pod_y, tile_x, tile_y, pc)    \
    {pod_x, pod_y, tile_x, tile_y, pc},

static std::vector<test_t> tests = {
#include "unit.inc"
};

static int RunOne(const test_t &test)
{
    return HB_MC_SUCCESS;
}

static int RunAll()
{
    for (auto & test : tests) {
        int r = RunOne(test);
        if (r != HB_MC_SUCCESS) {
            bsg_pr_err("Test(pod_x = %d, pod_y = %d, tile_x = %d, tile_y = %d, pc = %04x) : FAILED\n",
                       test.pod_x, test.pod_y, test.tile_x, test.tile_y, test.pc);
        }
    }       
    return HB_MC_SUCCESS;
}


static int Test(int argc, char *argv[])
{
    binary = argv[1];
    bsg_pr_info("Opening '%s'\n", binary);
    BSG_CUDA_CALL(hb_mc_manycore_init(&mc, "manycore", DEVICE_ID));
    BSG_CUDA_CALL(RunAll());
    BSG_CUDA_CALL(hb_mc_manycore_exit(&mc));
    return HB_MC_SUCCESS;
}

declare_program_main("VCore DMEM RW", Test);
