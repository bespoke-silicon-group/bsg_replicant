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
#include <fstream>
#include <iostream>
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
    //std::cout<<"===================================enter host=============================="<<std::endl;
    CL cl;
    cl.parse(argc, argv);
    HB = HammerBlade::Get();
    HB->load_application(cl.binary_path());
    int iter = cl.bfs_iteration();
    //int pod_ite = cl.bfs_podnum();
    //std::cout<<"====================ite ID is "<<iter<<"======================="<<std::endl;
    int pod_ite = cl.pod_id();
    int num_pods = 64;
    WGraph g = WGraph::FromCSR("wiki-Vote","/work/shared/users/phd/zy383/HB_Cosim/Bladerunner_6.2.0/bsg_bladerunner/bsg_replicant/examples/cuda/bfs/inputs/CSRfile/wiki-Vote/");
    //WGraph g = WGraph::FromCSR("ljournal-2008","/work/shared/users/phd/zy383/HB_Cosim/Bladerunner_6.2.0/bsg_bladerunner/bsg_replicant/examples/cuda/bfs/inputs/CSRfile/livejournal/");
    //SparsePushBFS stats = SparsePushBFS::RunBFS_single(g, cl.bfs_root(), cl.bfs_iteration(), false);
    // load application

    std::shared_ptr<WGraph> wgptr = std::shared_ptr<WGraph>(new WGraph(g));
    std::set<int> frontier = {cl.bfs_root()};
    std::set<int> visited = {cl.bfs_root()};
    SparsePushBFS bfs = SparsePushBFS(wgptr, frontier, visited); 
    bfs.run(iter+1);
    
    float frontier_size = bfs.frontier_in().size();
    //std::cout<<"==================frontier_in size is "<<frontier_size<<"========================"<<std::endl;
    int num_nodes = g.num_nodes();
    float frontier_density = frontier_size/(float)num_nodes;
    int direction;//edge traversal direction, 0 for pull and 1 for push
    direction = (frontier_density>0.1) ? 0:1;

    const std::set<int>& frontier_out_host = bfs.frontier_out();
    std::cout<<"=========================host out frontier size "<<frontier_out_host.size()<<"======================="<<std::endl;
    //std::cout<<"=========================host in frontier size "<<frontier_size<<"======================="<<std::endl;
    //std::cout<<"=========================host visited io size "<<bfs.visited_in().size()<<"======================="<<std::endl;
    
    WGraph g_csr = WGraph::FromCSR("wiki-Vote","/work/shared/users/phd/zy383/HB_Cosim/Bladerunner_6.2.0/bsg_bladerunner/bsg_replicant/examples/cuda/bfs/inputs/CSCfile/wiki-Vote/",pod_ite,num_pods);
    WGraph g_csc = WGraph::FromCSC("wiki-Vote","/work/shared/users/phd/zy383/HB_Cosim/Bladerunner_6.2.0/bsg_bladerunner/bsg_replicant/examples/cuda/bfs/inputs/CSRfile/wiki-Vote/",pod_ite,num_pods);
    //WGraph g_csr = WGraph::FromCSR("soc-Pokec","/work/shared/users/phd/zy383/HB_Cosim/Bladerunner_6.2.0/bsg_bladerunner/bsg_replicant/examples/cuda/bfs/inputs/CSRfile/pokec/",pod_ite,num_pods);
    //WGraph g_csc = WGraph::FromCSC("soc-Pokec","/work/shared/users/phd/zy383/HB_Cosim/Bladerunner_6.2.0/bsg_bladerunner/bsg_replicant/examples/cuda/bfs/inputs/CSCfile/pokec/",pod_ite,num_pods);
    //WGraph g_csr = WGraph::FromCSR("ljournal-2008","/work/shared/users/phd/zy383/HB_Cosim/Bladerunner_6.2.0/bsg_bladerunner/bsg_replicant/examples/cuda/bfs/inputs/CSRfile/livejournal/",pod_ite,num_pods);
    //WGraph g_csc = WGraph::FromCSR("ljournal-2008","/work/shared/users/phd/zy383/HB_Cosim/Bladerunner_6.2.0/bsg_bladerunner/bsg_replicant/examples/cuda/bfs/inputs/CSCfile/livejournal/",pod_ite,num_pods);
    //WGraph g_csr = WGraph::FromCSR("lgc_csr_float32","/work/global/zy383/Bladerunner6.4.0/bsg_replicant/examples/cuda/bfs/inputs/CSRfile/lgc_ista/",pod_ite,num_pods);
    //WGraph g_csc = WGraph::FromCSR("lgc_csr_float32","/work/global/zy383/Bladerunner6.4.0/bsg_replicant/examples/cuda/bfs/inputs/CSCfile/lgc_ista/",pod_ite,num_pods);
    //WGraph g_csr = WGraph::FromGraph500Data(Graph500Data::FromFile(cl.input_graph_path()));
    //WGraph g_csc = WGraph::FromGraph500Data(Graph500Data::FromFile(cl.input_graph_path()),true);
    //WGraph g_csr = WGraph::FromCSR("wiki-Vote","/work/shared/users/phd/zy383/HB_Cosim/Bladerunner_6.2.0/bsg_bladerunner/bsg_replicant/examples/cuda/bfs/inputs/CSRfile/wiki-Vote/");
    //WGraph g_csc = WGraph::C2SR("wiki-Vote","/work/shared/users/phd/zy383/HB_Cosim/Bladerunner_6.2.0/bsg_bladerunner/bsg_replicant/examples/cuda/bfs/inputs/CSCfile/wiki-Vote/");
    //std::cout<<"===================================after graph init!=============================="<<std::endl;
    //decide the edge traversal direction
    //float frontier_density = stats[iter].frontier_in().size()/g_csc.num_nodes();
    //bsg_pr_info("Frontier density is %d, frontier size is %d, num of node is %d \n",frontier_density,(stats[iter].frontier_in()).size(), g_csc.num_nodes());
    BFSGraph bfsg_csc(g_csc);
    BFSGraph bfsg_csr(g_csr);
    //std::cout<<"=================================== graph fromated! =============================="<<std::endl;
    BFSSparseSet frontier_in_sparse(bfs.frontier_in(), num_nodes);
    BFSDenseSet  frontier_in_dense(bfs.frontier_in(), num_nodes);
    
    //std::cout<<"===================================init graph complete!=============================="<<std::endl;
    
    //TODO RECONSTRUCT VISITED VECTOR AND OUTPUT FRONTIER
    std::set<int> visited_in = bfs.visited_in();
    std::set<int> visited_ite;
    for(int m : visited_in){
        if(m%num_pods == pod_ite){
            visited_ite.insert(m/num_pods);
        }    
    }
    int visited_size = (pod_ite < num_nodes % num_pods) ? (num_nodes+num_pods-1)/num_pods:num_nodes/num_pods;
    BFSDenseSet   frontier_out_sparse(std::set<int>(),    visited_size);
    BFSDenseSet   frontier_out_dense(std::set<int>(),    visited_size);
    
    BFSDenseSet  visited_io(visited_ite,   visited_size);
    
    hammerblade::host::HammerBlade::Ptr _hb(hammerblade::host::HammerBlade::Get());
    kernel_int_ptr_t dircetion_hb = _hb->alloc(sizeof(int));
    _hb->push_write(dircetion_hb, &direction,sizeof(int));
    
    kernel_int_ptr_t len_hb = _hb->alloc(sizeof(int));
    _hb->push_write(len_hb, &visited_size,sizeof(int));

    kernel_int_ptr_t ite_hb = _hb->alloc(sizeof(int));
    _hb->push_write(ite_hb, &pod_ite,sizeof(int));

    bfsg_csr.formatOnDevice(); 
    bfsg_csc.formatOnDevice();
    frontier_in_dense.formatOnDevice();
    frontier_in_sparse.formatOnDevice();
    frontier_out_dense.formatOnDevice();
    frontier_out_sparse.formatOnDevice();
    visited_io.formatOnDevice();
    //std::cout<<"===================================prepare complete!=============================="<<std::endl;
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
             "bfs", bfsg_csr.kgraph_dev(),bfsg_csc.kgraph_dev(), frontier_in_sparse.dev(), frontier_in_dense.dev(), frontier_out_sparse.dev(), frontier_out_dense.dev(), visited_io.dev(),dircetion_hb,len_hb);
    HB->exec();
    // read output
    //frontier_out_dense.updateFromDevice();
    frontier_out_sparse.updateFromDevice();
    //visited_io.updateFromDevice();
    int visited_dev;
    _hb->push_read(len_hb, &visited_dev, sizeof(int));
    HB->sync_read();
    //TODO RETREIVE PUSH OUTPUT
    //std::set<int> frontier_out_kernel_de = frontier_out_dense.setAfterUpdate();
    std::set<int> frontier_out_kernel_sp = frontier_out_sparse.setAfterUpdate(visited_dev);
    
    //check results for this pod iteration
    std::set<int> host_out_ite;
    
    for (int m: frontier_out_host){
        //std::cout<<"================frontier out: "<< m <<" !!!!!!!!!!!!!!" << std::endl;
        if(m%num_pods==pod_ite){
            host_out_ite.insert(m/num_pods);
        }
    }
    std::cout<<"===================================host out length is "<<host_out_ite.size()<<"kernel out length is "<<visited_dev<<"=============================="<<std::endl;
    bool equals = true;
    for (int m : host_out_ite) {
        auto it = frontier_out_kernel_sp.find(m);
        if (it == frontier_out_kernel_sp.end()) {
            bsg_pr_err("Found %d in host result but not kernel in iteration %d\n", m,iter);
            equals = false;
        }
        //else{
        //    bsg_pr_err("++++Found %d in both host and kernel in iteration %d\n", m,iter);
        //}
    }
    for (int m : frontier_out_kernel_sp) {
        auto it = host_out_ite.find(m);
        if (it == host_out_ite.end()) {
            bsg_pr_err("Found %d in kernel result but not host in iteration %d\n", m,iter);
            equals = false;
        }
        //else{
        //    bsg_pr_err("++++Found %d in both host and kernel in iteration %d\n", m,iter);
        //}
    }

    //stats.dump("bfs_stats.txt");
    std::ofstream outputfile;
    outputfile.open("out_put_lenth.txt");
    outputfile<<frontier_out_kernel_sp.size()<<std::endl;
    outputfile.close();
    HB->close();
    return equals ? HB_MC_SUCCESS : HB_MC_FAIL;
}

declare_program_main("BFS", Main);
