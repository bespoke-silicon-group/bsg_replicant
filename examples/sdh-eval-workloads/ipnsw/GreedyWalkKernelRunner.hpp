#pragma once
#include "IPNSWKernelRunner.hpp"
#include "IPNSWRunner.hpp"

namespace ipnsw {
    class GreedyWalkKernelRunner : public IPNSWKernelRunner {
        std::string kernelName(const IPNSWRunner & runner) const {
            return "ipnsw_greedy_search";
        }

        std::vector<hb_mc_eva_t> argv(const IPNSWRunner & runner) const {
            std::vector<hb_mc_eva_t> argv = {
                runner.graph_metadata_dev(),
                runner.db_dev(),
                runner.query_dev(),
                runner.seen_dev(),
                runner.v_curr_dev(),
                runner.d_curr_dev(),
            };
            return argv;
        };
        Dim gd(const IPNSWRunner &runner) const {return Dim(1,1);}
        Dim tgd(const IPNSWRunner &runner) const {return Dim(1,1);}
    };
}
