#include <bsg_manycore_regression.h>
#include <bsg_manycore.h>
#include <bsg_manycore_cuda.h>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
using std::string;

template <typename T>
T from_string(const std::string &s) {
    std::stringstream ss(s);
    T v;
    ss >> v;
    return v;    
}

typedef enum {
    EXE,
    POD_X,
    POD_Y,
    NORTH_NOT_SOUTH,
    L2_X,
    SET,
    NARGS
} arg_t;

static hb_mc_manycore_t mc = {};
static hb_mc_config_t *cfg = &mc.config;

static int pod_x;
static int pod_y;
static int north_not_south;
static int l2_x;
static int set;

static hb_mc_npa_t make_npa(int pod_x, int pod_y, int north_not_south, int l2_x, int set, unsigned way)
{
    hb_mc_coordinate_t pod;
    pod.x = pod_x;
    pod.y = pod_y;

    hb_mc_coordinate_t l2 = hb_mc_config_pod_dram(cfg, pod, 0);
    l2.x += l2_x;
    l2.y  = north_not_south ? hb_mc_config_pod_dram_north_y(cfg, pod) : hb_mc_config_pod_dram_south_y(cfg, pod);

    hb_mc_npa_t addr;
    addr.x = l2.x;
    addr.y = l2.y;
    addr.epa = hb_mc_config_get_vcache_block_size(cfg) * (set  + cfg->vcache_sets * way);

    return addr;
}

static int TestOneWay(unsigned way, const std::vector<unsigned> &w_data)
{
    hb_mc_npa_t addr = make_npa(pod_x, pod_y, north_not_south, l2_x, set, way);
    char addr_str[256];

    hb_mc_npa_to_string(&addr, addr_str, sizeof(addr_str));
    bsg_pr_info("TESTING %s\n", addr_str);

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
        bsg_pr_err("DATA MISMATCH: NPA %s (way = %u)\n", addr_str, way);
        bsg_pr_err("%-8s %-8s\n", "w", "r");
        for (unsigned i = 0; i < w_data.size(); i++)
            bsg_pr_err("%08x %08x\n", r_data[i], w_data[i]);

        return HB_MC_FAIL;
    }

    return HB_MC_SUCCESS;
}

static int TestOne(const std::vector<unsigned> &w_data)
{
    for (unsigned w = 0; w < cfg->vcache_ways; w++) {
        BSG_CUDA_CALL(TestOneWay(w, w_data));
    }
    return HB_MC_SUCCESS;
}

static int Test(int argc, char *argv[])
{
    // parse command line, like a normal person you know... like not one that uses argp
    if (argc != NARGS) {
        bsg_pr_err("Hey silly, %d is the wrong number of args (it should be %d). Use the Makefile darnit! That's what it's for...",
                   argc, NARGS);
        return HB_MC_FAIL;
    }

    pod_x           = from_string<int>(argv[POD_X]);
    pod_y           = from_string<int>(argv[POD_Y]);
    north_not_south = from_string<int>(argv[NORTH_NOT_SOUTH]);
    l2_x            = from_string<int>(argv[L2_X]);
    set             = from_string<int>(argv[SET]);

    bsg_pr_test_info("Pod-X:           %2d\n", pod_x);
    bsg_pr_test_info("Pod-Y:           %2d\n", pod_y);
    bsg_pr_test_info("North-not-South: %2d\n", north_not_south);
    bsg_pr_test_info("VCache-X:        %2d\n", l2_x);
    bsg_pr_test_info("Set:             %08x\n", set);    
    
    // init
    BSG_CUDA_CALL(hb_mc_manycore_init(&mc, "Got 99 Problems", 0));    

    std::vector<unsigned> data(cfg->vcache_block_words, 0x55555555);
    
    // test once
    BSG_CUDA_CALL(TestOne(data));
    for (unsigned i = 0; i < data.size(); i++)
        data[i] = ~data[i];
    // flip all the bits and test again
    BSG_CUDA_CALL(TestOne(data));
    
    // done
    BSG_CUDA_CALL(hb_mc_manycore_exit(&mc));
    return HB_MC_SUCCESS;
}

declare_program_main("L2 Read Set", Test);
