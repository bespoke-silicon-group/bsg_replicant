#include <bsg_manycore_regression.h>
#include <bsg_manycore.h>
#include <bsg_manycore_cuda.h>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>

hb_mc_manycore_t mc = {};
hb_mc_config_t *cfg = &mc.config;

typedef struct {
    int pod_x;
    int pod_y;
    int tile_x;
    int tile_y;
    int dmem_word;
} test_t;

#define DECLARE_TEST(pod_x, pod_y, tile_x, tile_y, dmem_word)    \
    {pod_x, pod_y, tile_x, tile_y, dmem_word},

static std::vector<test_t> tests = {
#include "unit.inc"
};

static hb_mc_npa_t MakeNPA(const test_t &test)
{
    hb_mc_coordinate_t pod;
    pod.x = test.pod_x;
    pod.y = test.pod_y;

    hb_mc_coordinate_t coord = hb_mc_config_pod_vcore_origin(cfg, pod);
    coord.x += test.tile_x;
    coord.y += test.tile_y;

    hb_mc_npa_t addr;
    addr.x = coord.x;
    addr.y = coord.y;
    addr.epa = test.dmem_word * 4;
    
    return addr;
}


static int RunOneWData(const test_t & test,
                  const std::vector<unsigned> &w_data)
{
    hb_mc_npa_t addr = MakeNPA(test);
    char addr_str[256];

    hb_mc_npa_to_string(&addr, addr_str, sizeof(addr_str));
    bsg_pr_dbg("TESTING %s\n", addr_str);

    static int i = 0;
    BSG_CUDA_CALL(hb_mc_manycore_write_mem(&mc, &addr, &w_data[0], w_data.size() * sizeof(unsigned)));

    // read
    // flip each bit in buffer before reading
    std::vector<unsigned> r_data(w_data.size());
    for (unsigned i = 0; i < r_data.size(); i++)
        r_data[i] = ~w_data[i];

    // do the read
    BSG_CUDA_CALL(hb_mc_manycore_read_mem(&mc, &addr, &r_data[0], r_data.size() * sizeof(unsigned)));

    // compare with what we wrote
    if (memcmp(&w_data[0], &r_data[0], w_data.size() * sizeof(unsigned)) != 0) {
        bsg_pr_err("DATA MISMATCH: NPA %s\n",
                   addr_str);

        bsg_pr_err("%-8s %-8s\n", "w", "r");
        for (unsigned i = 0; i < w_data.size(); i++)
            bsg_pr_err("%08x %08x\n", r_data[i], w_data[i]);

        return HB_MC_FAIL;
    }

    return HB_MC_SUCCESS;
}

static int RunOne(const test_t &test)
{
    std::vector<unsigned> data(1, 0x55555555);
    // test once
    BSG_CUDA_CALL(RunOneWData(test, data));
    for (unsigned i = 0; i < data.size(); i++)
        data[i] = ~data[i];
    // flip all the bits and test again
    BSG_CUDA_CALL(RunOneWData(test, data));
    return HB_MC_SUCCESS;
}

static int RunAll()
{
    for (auto & test : tests) {
        int r = RunOne(test);
        if (r != HB_MC_SUCCESS) {
            bsg_pr_err("Test(pod_x = %d, pod_y = %d, tile_x = %d, tile_y = %d, word = %d) : FAILED\n",
                       test.pod_x, test.pod_y, test.tile_x, test.tile_y, test.dmem_word);
        }
    }       
    return HB_MC_SUCCESS;
}


static int Test(int argc, char *argv[])
{
    BSG_CUDA_CALL(hb_mc_manycore_init(&mc, "manycore", HB_MC_DEVICE_ID));
    BSG_CUDA_CALL(RunAll());
    BSG_CUDA_CALL(hb_mc_manycore_exit(&mc));
    return HB_MC_SUCCESS;
}

declare_program_main("VCore DMEM RW", Test);
