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
    int south_not_north;
    int l2_x;
    int set;
} test_t;

#define DECLARE_TEST(pod_x, pod_y, south_not_north, vcache_x, set)      \
    {pod_x, pod_y, south_not_north, vcache_x, set},

static std::vector<test_t> tests = {
#include "unit.inc"
};



static hb_mc_npa_t MakeNPA(int pod_x, int pod_y,
                            int south_not_north, int l2_x,
                            int set, unsigned way)
{
    hb_mc_coordinate_t pod;
    pod.x = pod_x;
    pod.y = pod_y;

    hb_mc_coordinate_t l2 = hb_mc_config_pod_dram(cfg, pod, 0);
    l2.x += l2_x;
    l2.y  = south_not_north ?
        hb_mc_config_pod_dram_south_y(cfg, pod) :
        hb_mc_config_pod_dram_north_y(cfg, pod) ;

    hb_mc_npa_t addr;
    addr.x = l2.x;
    addr.y = l2.y;
    addr.epa = hb_mc_config_get_vcache_block_size(cfg) * (set  + cfg->vcache_sets * way);

    return addr;
}


static int RunOneWay(const test_t & test,
                     unsigned way,
                     const std::vector<unsigned> &w_data)
{
    hb_mc_npa_t addr = MakeNPA(test.pod_x,
                                test.pod_y,
                                test.south_not_north,
                                test.l2_x,
                                test.set,
                                way);
    char addr_str[256];

    hb_mc_npa_to_string(&addr, addr_str, sizeof(addr_str));
    bsg_pr_dbg("TESTING %s\n", addr_str);

    static int i = 0;
    BSG_CUDA_CALL(hb_mc_manycore_dma_write(&mc, &addr, &w_data[0], w_data.size() * sizeof(unsigned)));

    // read
    // flip each bit in buffer before reading
    std::vector<unsigned> r_data(w_data.size());
    for (unsigned i = 0; i < r_data.size(); i++)
        r_data[i] = ~w_data[i];

    // do the read
    BSG_CUDA_CALL(hb_mc_manycore_read_mem(&mc, &addr, &r_data[0], r_data.size() * sizeof(unsigned)));

    // compare with what we wrote
    if (memcmp(&w_data[0], &r_data[0], w_data.size() * sizeof(unsigned)) != 0) {
        bsg_pr_err("DATA MISMATCH: NPA %s (way = %u)\n",
                   addr_str, way);

        bsg_pr_err("%-8s %-8s\n", "w", "r");
        for (unsigned i = 0; i < w_data.size(); i++)
            bsg_pr_err("%08x %08x\n", r_data[i], w_data[i]);

        return HB_MC_FAIL;
    }

    return HB_MC_SUCCESS;
}

static int RunOneWData(const test_t &test, const std::vector<unsigned> &w_data)
{
    for (unsigned w = 0; w < cfg->vcache_ways; w++) {
        BSG_CUDA_CALL(RunOneWay(test, w, w_data));
    }
    return HB_MC_SUCCESS;
}

static int RunOne(const test_t &test)
{
    std::vector<unsigned> data(cfg->vcache_block_words, 0x55555555);
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
            bsg_pr_err("Test(pod_x = %d, pod_y = %d, south_not_north = %d, vcache_x = %d, set = %d) : FAILED\n",
                       test.pod_x, test.pod_y, test.south_not_north, test.l2_x, test.set);
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

declare_program_main("L2 Read Set", Test);
