#include "ipnsw.hpp"
#include "HammerBlade.hpp"
#include "Graph500Data.hpp"
#include "Graph.hpp"
#include "IO.hpp"
#include "IPNSWGraph.hpp"
#include "IPNSWRunner.hpp"
#include "IProductUBmkKernelRunner.hpp"
#include "IProductUBmkResultReader.hpp"
#include "IProductUBmkFactory.hpp"
#include "BeamSearchKernelRunner.hpp"
#include "BeamSearchResultReader.hpp"
#include "BeamSearchFactory.hpp"
#include "GreedyWalkKernelRunner.hpp"
#include "GreedyWalkResultReader.hpp"
#include "GreedyWalkFactory.hpp"
#include "GreedyWalkResults.hpp"
#include "StringHelpers.hpp"
#include <iostream>
#include <memory>

#include "GreedyWalkResults.cpp"

using namespace ipnsw;

int Main(int argc, char *argv[])
{
    Parser args;
    args.parse(argc, argv);

    std::unique_ptr<IPNSWRunner> runner;
    std::unique_ptr<IPNSWFactory> factory;

    if (ipnsw::startswith(args.version(), "greedy_walk")) {
        factory = std::unique_ptr<IPNSWFactory>(new GreedyWalkFactory);
    } else if (ipnsw::startswith(args.version(), "beam_search")) {
        factory = std::unique_ptr<IPNSWFactory>(new BeamSearchFactory);
    } else if (ipnsw::startswith(args.version(), "iproduct_ubmk")) {
        /* parse the number of inner products */
        std::cout << "num inner products " << args.num_iproducts() << std::endl;
        int n_iproducts = args.num_iproducts();
        factory = std::unique_ptr<IPNSWFactory>(new IProductUBmkFactory(n_iproducts));
    } else if (args._version == "debug") {
        /* just for debugging */
        std::cout << "--num-iproducts=" << args.num_iproducts() << std::endl;
        std::cout << "--queries=";
        auto do_queries = args.do_queries();
        for (auto q : do_queries) {
            std::cout << q << " ";
        }
        std::cout << std::endl;
        return 0;
    } else {
        return 0;
    }

    runner = std::unique_ptr<IPNSWRunner>(new IPNSWRunner(args, factory));
    runner->run();

    return 0;
}

#ifdef COSIM
void cosim_main(uint32_t *exit_code, char * args) {
    // We aren't passed command line arguments directly so we parse them
    // from *args. args is a string from VCS - to pass a string of arguments
    // to args, pass c_args to VCS as follows: +c_args="<space separated
    // list of args>"
    int argc = get_argc(args);
    char *argv[argc];
    get_argv(args, argc, argv);

#ifdef VCS
    svScope scope;
    scope = svGetScopeFromName("tb");
    svSetScope(scope);
#endif
    int rc = Main(argc, argv);
    *exit_code = rc;
    return;
}
#else
int main(int argc, char ** argv) {
    return Main(argc, argv);
}
#endif
