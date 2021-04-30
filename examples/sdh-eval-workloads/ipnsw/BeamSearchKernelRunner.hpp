#pragma once
#include "IPNSWKernelRunner.hpp"
#include "IPNSWRunner.hpp"
#include "GreedyWalkResults.hpp"

namespace ipnsw {
    class BeamSearchKernelRunner : public IPNSWKernelRunner {
        std::string kernelName(const IPNSWRunner & runner) const {
            return "ipnsw_beam_search";
        }

        std::vector<hb_mc_eva_t> argv(const IPNSWRunner & runner) const {
            int v_curr;
            float d_curr;
            v_curr = std::get<GWR_VERT>(GREEDY_WALK_RESULTS[IPNSWRunner::QUERY]);
            d_curr = std::get<GWR_DIST>(GREEDY_WALK_RESULTS[IPNSWRunner::QUERY]);

            HammerBlade::Ptr hb = HammerBlade::Get();
            hb->write(runner.v_curr_dev(), &v_curr, sizeof(v_curr));
            hb->write(runner.d_curr_dev(), &d_curr, sizeof(d_curr));

            std::vector<hb_mc_eva_t> argv = {
                runner.graph_metadata_dev(),
                runner.db_dev(),
                runner.query_dev(),
                runner.seen_dev(),
                runner.v_curr_dev(),
                runner.d_curr_dev(),
                runner.candidates_dev(),
                runner.results_dev(),
                runner.n_results_dev(),
            };
            return argv;
        };
        Dim gd(const IPNSWRunner &runner) const {return Dim(1,1);}
        Dim tgd(const IPNSWRunner &runner) const {return Dim(1,1);}
    };
}
