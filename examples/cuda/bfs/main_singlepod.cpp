// Copyright (c) 2019, University of Washington All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this list
// of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
//
// Neither the name of the copyright holder nor the names of its contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#include <bsg_manycore_tile.h>
#include <bsg_manycore_errno.h>
#include <bsg_manycore_tile.h>
#include <bsg_manycore_loader.h>
#include <bsg_manycore_cuda.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <cmath>
#include <bsg_manycore_regression.h>
#include "HammerBlade.hpp"
#include "bfs/CL.hpp"
#include "bfs/BFSGraph.hpp"
#include "bfs/BFSSparseSet.hpp"
#include "bfs/BFSDenseSet.hpp"
#include "WGraph.hpp"
#include "SparsePushBFS.hpp"
#include "SparsePullBFS.hpp"

using namespace hammerblade::host;
using namespace BFS;
using namespace graph_tools;

HammerBlade::Ptr HB;

int Main(int argc, char *argv[])
{
    CL cl;
    cl.parse(argc, argv);

    WGraph g_csr = WGraph::FromCSR("soc-Pokec","/work/global/zy383/Bladerunner6.4.0/bsg_replicant/examples/cuda/bfs/inputs/CSRfile/pokec/");
    WGraph g_csc = WGraph::FromCSR("soc-Pokec","/work/global/zy383/Bladerunner6.4.0/bsg_replicant/examples/cuda/bfs/inputs/CSCfile/pokec/");
    //WGraph g_csr = WGraph::FromCSR("lgc_csr_float32","/work/global/zy383/Bladerunner6.4.0/bsg_replicant/examples/cuda/bfs/inputs/CSRfile/lgc_ista/");
    //WGraph g_csc = WGraph::FromCSR("lgc_csr_float32","/work/global/zy383/Bladerunner6.4.0/bsg_replicant/examples/cuda/bfs/inputs/CSCfile/lgc_ista/");
    //WGraph g_csr = WGraph::FromGraph500Data(Graph500Data::FromFile(cl.input_graph_path()));
    //WGraph g_csc = WGraph::FromGraph500Data(Graph500Data::FromFile(cl.input_graph_path()),true);
    //WGraph g_csr = WGraph::FromCSR("wiki-Vote","/work/shared/users/phd/zy383/HB_Cosim/Bladerunner_6.2.0/bsg_bladerunner/bsg_replicant/examples/cuda/bfs/inputs/CSRfile/wiki-Vote/");
    //WGraph g_csc = WGraph::C2SR("wiki-Vote","/work/shared/users/phd/zy383/HB_Cosim/Bladerunner_6.2.0/bsg_bladerunner/bsg_replicant/examples/cuda/bfs/inputs/CSCfile/wiki-Vote/");
    
    int iter = cl.bfs_iteration();

    std::vector<SparsePushBFS> stats = SparsePushBFS::RunBFS(g_csr, cl.bfs_root(), cl.bfs_iteration(), false);

    // load application
    HB = HammerBlade::Get();
    HB->load_application(cl.binary_path());

    //decide the edge traversal direction
    //float frontier_density = stats[iter].frontier_in().size()/g_csc.num_nodes();
    //bsg_pr_info("Frontier density is %d, frontier size is %d, num of node is %d \n",frontier_density,(stats[iter].frontier_in()).size(), g_csc.num_nodes());
    BFSGraph bfsg_csc(g_csc);
    BFSGraph bfsg_csr(g_csr);
    float frontier_size = stats[iter].frontier_in().size();
    float num_nodes = bfsg_csr.graph().num_nodes();
    float frontier_density = frontier_size/num_nodes;
    std::cout<<"density: "<<frontier_density<<"frontier size: "<<frontier_size<<"num_nodes:"<<num_nodes<<std::endl;
    //bsg_pr_info("Frontier density is %s, frontier size is %s, num of node is %s \n",std::to_string(frontier_density),std::to_string(frontier_size), std::to_string(num_nodes));
    BFSSparseSet frontier_in_sparse(stats[iter].frontier_in(), bfsg_csr.graph().num_nodes());
    BFSDenseSet  frontier_in_dense(stats[iter].frontier_in(), bfsg_csr.graph().num_nodes());
    BFSDenseSet  frontier_out(std::set<int>(),    bfsg_csr.graph().num_nodes());
    BFSDenseSet  visited_io(stats[iter].visited_in(),   bfsg_csr.graph().num_nodes());
    int direction;//edge traversal direction, 0 for pull and 1 for push
    hammerblade::host::HammerBlade::Ptr _hb(hammerblade::host::HammerBlade::Get());
    kernel_int_ptr_t dircetion_hb = _hb->alloc(sizeof(int));
    
    if (frontier_density>0.1){
        direction = 0;
    }
    else{
        direction = 1;
    }
    _hb->push_write(dircetion_hb, &direction,sizeof(int));

    
    
    

    bfsg_csr.formatOnDevice(); 
    bfsg_csc.formatOnDevice();
    frontier_in_dense.formatOnDevice();
    frontier_in_sparse.formatOnDevice();
    frontier_out.formatOnDevice();
    visited_io.formatOnDevice();

    // sync writes
    HB->sync_write();
    bsg_pr_info("BFS iteration %d on %s graph with %d nodes and %d edges starting from root %d\n",
                cl.bfs_iteration(),
                cl.graph_type().c_str(),
                bfsg_csr.num_nodes(),
                bfsg_csr.num_edges(),
                cl.bfs_root());

    bsg_pr_info("Launching BFS with %d groups of shape (x=%d,y=%d)\n", cl.groups(), cl.tgx(), cl.tgy());
    HB->push_job(Dim(cl.groups(),1), Dim(cl.tgx(),cl.tgy()),
                 "bfs", bfsg_csr.kgraph_dev(),bfsg_csc.kgraph_dev(), frontier_in_sparse.dev(), frontier_in_dense.dev(), frontier_out.dev(), visited_io.dev(),dircetion_hb);
    HB->exec();

    // read output
    frontier_out.updateFromDevice();
    visited_io.updateFromDevice();
    HB->sync_read();

    std::set<int> frontier_out_kernel = frontier_out.setAfterUpdate();
    const std::set<int>& frontier_out_host = stats[iter].frontier_out();

    // check sets are equal
    // check that host contains kernel
    bool equals = true;
    for (int m : frontier_out_host) {
        auto it = frontier_out_kernel.find(m);
        if (it == frontier_out_kernel.end()) {
            bsg_pr_err("Found %d in host result but not kernel\n", m);
            equals = false;
        }
    }
    // check that kernel contains host
    for (int m : frontier_out_kernel) {
        auto it = frontier_out_host.find(m);
        if (it == frontier_out_host.end()) {
            bsg_pr_err("Found %d in kernel result but not host\n", m);
            equals = false;
        }
    }

    stats[iter].dump("bfs_stats.txt");
    HB->close();
    return equals ? HB_MC_SUCCESS : HB_MC_FAIL;
}

declare_program_main("BFS", Main);
