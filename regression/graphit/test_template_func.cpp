#include "builtins_hammerblade.h"
#include <string.h>
#include "../cl_manycore_regression.h"
#define TEST_NAME "test_bfs_templated"
#define ALLOC_NAME "default_allocator"

const std::string ucode_path =
        "//nbu/spin/eafurst/bsg_bladerunner/bsg_manycore"
        "/software/spmd/bsg_cuda_lite_runtime/template_func/main.riscv";

using hammerblade::Device;
using hammerblade::Vector;
using hammerblade::GraphHB;
using hammerblade::GlobalScalar;

GraphHB edges;

using GlblHBInt = GlobalScalar<int>;

GlblHBInt test_val_dev;

int launch(int argc, char * argv[]){
    hammerblade::builtin_loadMicroCodeFromFile(ucode_path);

    test_val_dev = GlblHBInt("test_val");
    edges = hammerblade::builtin_loadEdgesFromFileToHB ( argv[(1) ]) ;
    Device::Ptr device = Device::GetInstance();

    std::cerr << "setting global scalar values" << std::endl;
    test_val_dev.set(1);
    std::cerr << "launching kernel using templated funcs" << std::endl;
    device->enqueueJob("kernel_template_func",
                          {edges.num_nodes(),
                           edges.num_nodes(),
                           edges.num_edges(),
                           edges.num_nodes()});
    device->runJobs();
    std::cerr << "kernel finished running" << std::endl;
    int temp = test_val_dev.get();

    std::cerr << "new test value: " << temp << std::endl;
    return 0;
}


#ifdef COSIM
void cosim_main(uint32_t *exit_code, char * args) {
    int argc = get_argc(args);
    char *argv[argc];
    get_argv(args, argc, argv);

#ifdef VCS
    svScope scope;
    scope = svGetScopeFromName("tb");
    svSetScope(scope);
#endif
    std::cerr << "launching the test" << std::endl;
    bsg_pr_test_info("Unified Main Regression Test (COSIMULATION)\n");
    int rc = launch(argc,argv);
    *exit_code = rc;
    bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
    return;
}
#else
int main(int argc, char ** argv) {
    bsg_pr_test_info("Unified Main CUDA Regression Test (F1)\n");
    int rc = launch(argc,argv);
    bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
    return rc;
}
#endif
