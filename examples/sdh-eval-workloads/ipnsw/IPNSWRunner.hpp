#pragma once
#include "IO.hpp"
#include "HammerBlade.hpp"
#include "IPNSWGraph.hpp"
#include "IPNSWFactory.hpp"
#include "IPNSWKernelRunner.hpp"
#include "IPNSWResultReader.hpp"
#include "GreedyWalkResults.hpp"
#include <memory>

namespace ipnsw {

    class IPNSWRunner {
    public:
        //static constexpr int QUERY = 276; // fewest dot products for greedy walk
        //static constexpr int QUERY = 472; // fewest dot products for beam search
        //static constexpr int QUERY = 427;
        //static constexpr int QUERY = 355;
        //static constexpr int QUERY = 2;
        static constexpr int QUERY = 188;
        //static constexpr int QUERY = 229;
        //static constexpr int QUERY = 490;
        //static constexpr int QUERY = 16;
        //static constexpr int QUERY = 461;
        //static constexpr int QUERY = 470;

        using HammerBlade = hammerblade::host::HammerBlade;
        using Dim = hammerblade::host::Dim;

        IPNSWRunner(const Parser &p,
                    std::unique_ptr<IPNSWFactory> & fact):
            _factory(std::move(fact)) {
            _io = std::unique_ptr<IO>(new IO(p));
            _hb = HammerBlade::Get();
            _kernel_runner = _factory->KernelRunner();
            _result_reader = _factory->ResultReader();
        }

        virtual ~IPNSWRunner() { delete _hb; }

        void readInput() {
            auto graphs   = _io->graphs();
            _graphs = {
                Graph(std::move(graphs[3])),
                Graph(std::move(graphs[2])),
                Graph(std::move(graphs[1])),
                Graph(std::move(graphs[0]))
            };

            _db       = _io->database<float,100>();
            _queries  = _io->queries<float,100>();
        }

        void loadProgram() {
            _hb->load_application(ucodePath());
        }

        void initializeDeviceMemoryDB() {
            std::cout << "Initializing database " << std::endl;
            _db_dev = _hb->alloc(_db.size() * sizeof(_db[0]));
            _hb->push_write(_db_dev, &_db[0], _db.size() * sizeof(_db[0]));
        }

        void initializeDeviceMemoryQuery() {
            std::cout << "Initializing query "  << std::endl;
            int query = QUERY;

            auto do_queries = _io->do_queries();
            if (!do_queries.empty())
                query = do_queries[0];

            _query_dev = _hb->alloc(sizeof(_queries[query]));
            _hb->push_write(_query_dev, &_queries[query], sizeof(_queries[query]));
        }

        void initializeDeviceMemorySeen() {
            std::cout << "Initializing seen set " << std::endl;
            _seen_dev = _hb->alloc(_db.size() * sizeof(int));
        }

        void initializeDeviceMemoryGraphs() {
            for (auto & graph : _graphs)
                graph.initialize_on_device();

            _graph_metadata_dev = Graph::InitializeMetadataOnDevice(_graphs);
        }

        void initializeDeviceVCurr() {
            _v_curr_dev = _hb->alloc(sizeof(int));
        }
        void initializeDeviceDCurr() {
            _d_curr_dev = _hb->alloc(sizeof(float));
        }

        void initializeDeviceCandidateDev() {
            _candidates_dev = _hb->alloc(sizeof(GreedyWalkResult)*513);
        }

        void initializeDeviceResultsDev() {
            _results_dev = _hb->alloc(sizeof(GreedyWalkResult) * 129);
        }

        void initializeDeviceNResultsDev() {
            _n_results_dev = _hb->alloc(sizeof(int));
        }

        void initializeDeviceMemory() {
            initializeDeviceMemoryDB();
            initializeDeviceMemoryQuery();
            initializeDeviceMemorySeen();
            initializeDeviceMemoryGraphs();
            initializeDeviceVCurr();
            initializeDeviceDCurr();
            initializeDeviceCandidateDev();
            initializeDeviceResultsDev();
            initializeDeviceNResultsDev();
            // sync
            std::cout << "Starting DMA" << std::endl;
            _hb->sync_rw();
        }

        void runKernel() {
            std::cout << "Launching kernel" << std::endl;
            _kernel_runner->runKernel(*this);
        }

        void readResults() {
            _result_reader->readResults(*this);

        }

        void run() {
            readInput();
            loadProgram();
            initializeDeviceMemory();
            runKernel();
            readResults();
        }

        /////////////
        // Getters //
        /////////////
        std::string ucodePath() const {
            return _io->ucode();
        }

        hb_mc_eva_t db_dev() const { return _db_dev; }
        hb_mc_eva_t query_dev() const { return _query_dev; }
        hb_mc_eva_t seen_dev() const { return _seen_dev; }
        hb_mc_eva_t v_curr_dev() const { return _v_curr_dev; }
        hb_mc_eva_t d_curr_dev() const { return _d_curr_dev; }
        hb_mc_eva_t graph_metadata_dev() const { return _graph_metadata_dev; }
        hb_mc_eva_t candidates_dev() const { return _candidates_dev; }
        hb_mc_eva_t results_dev() const { return _results_dev; }
        hb_mc_eva_t n_results_dev() const { return _n_results_dev; }

        /////////////
        // Setters //
        /////////////

    private:
        std::unique_ptr<IO>                  _io;
        std::vector<Graph>                   _graphs;
        std::vector<std::array<float, 100>>  _db;
        std::vector<std::array<float, 100>>  _queries;
        HammerBlade::Ptr                     _hb;

        // device pointers
        hb_mc_eva_t _db_dev;
        hb_mc_eva_t _query_dev;
        hb_mc_eva_t _seen_dev;
        hb_mc_eva_t _v_curr_dev;
        hb_mc_eva_t _d_curr_dev;
        hb_mc_eva_t _graph_metadata_dev;
        hb_mc_eva_t _candidates_dev;
        hb_mc_eva_t _results_dev;
        hb_mc_eva_t _n_results_dev;

        // composites
        std::unique_ptr<IPNSWKernelRunner> _kernel_runner;
        std::unique_ptr<IPNSWResultReader> _result_reader;
        std::unique_ptr<IPNSWFactory>      _factory;
    };
}
