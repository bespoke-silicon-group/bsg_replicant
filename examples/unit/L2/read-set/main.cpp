#include <bsg_manycore_regression.h>
#include <bsg_manycore.h>
#include <bsg_manycore_cuda.h>
#include <string>
#include <sstream>

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
    SOUTH_NOT_NORTH,
    L2_X,
    SET,
    NARGS
} arg_t;

int Test(int argc, char *argv[])
{
    // parse command line, like a normal person you know... like not one that uses argp
    if (argc != NARGS) {
        bsg_pr_err("Hey silly, %d is the wrong number of args (it should be %d). Use the Makefile darnit! That's what it's for...",
                   argc, NARGS);
        return HB_MC_FAIL;
    }

    int pod_x           = from_string<int>(argv[EXE]);
    int pod_y           = from_string<int>(argv[POD_X]);
    int south_not_north = from_string<int>(argv[POD_Y]);
    int l2_x            = from_string<int>(argv[SOUTH_NOT_NORTH]);
    int set             = from_string<int>(argv[SET]);

    #if 0
    // init
    bsg_manycore_t mc = {};
    BSG_CUDA_CALL(hb_mc_manycore_init(&mc, "Got 99 Problems", 0));

    
    // done
    BSG_CUDA_CALL(hb_mc_manycore_exit(&mc));
    #endif
    return HB_MC_SUCCESS;
}

declare_program_main("L2 Read Set", Test);
