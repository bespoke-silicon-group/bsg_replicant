#include "sssp.hpp"
#define X 16 
#define Y 2
#define NUM_LOCKS 1024
#define VERIFY false 
#define ROOT 6
#define DELTA 32

WGraphHB edges;
GlobalScalar<hb_mc_eva_t> dist_dev; 
//BucketPriorityQueue<int> pq;

bool apply(int s, int d, int w, std::vector<int> &dist) {
  int new_dist = (dist[s] + w);
  if(dist[d] > new_dist) {
    dist[d] = new_dist;
    return true;
  }
  return false;
}

void sssp_pull_call(std::vector<int> &front, std::vector<int> &next, std::vector<int> &dist) {
  auto g = edges.getHostGraph();
  auto * in_neigh = g.in_neighbors_shared_.get();
  auto ** in_index = g.in_index_shared_.get();
  for(int d = 0; d < edges.num_nodes(); d++) {
    int ind = in_index[d] - in_neigh;
    int degree = g.in_degree(d);
    auto * neighbors = &in_neigh[ind];
    for(int s = 0; s < degree; s++){
      if(front[neighbors[s].v]){
        if(apply(neighbors[s].v, d, neighbors[s].w, dist)) {
          next[d] = 1;
        }
      } 
    }
  }  

}

void host_sssp_pull(BucketPriorityQueue<int>& pq, std::vector<int> &dist, int iter) {
  dist[ROOT] = 0;
  Device::Ptr device = Device::GetInstance(); 
  Vector<int> next_frontier_dev = Vector<int>(edges.num_nodes());
  std::vector<int> h_next(edges.num_nodes(), 0);
  std::vector<int> h_front(edges.num_nodes(), 0);  

  for(int i = 0; i < iter; i++) {
    if(!(pq.finished() == 0)) { std::cout << "no more items on iter: " << i << "\n"; break; }
    Vector<int32_t> front = pq.popDenseReadyVertexSet(); 
    front.copyToHost(h_front.data(), edges.num_nodes()); 
    device->freeze_cores();
    device->read_dma();
    device->unfreeze_cores();
    int num_elems = std::count(h_front.begin(), h_front.end(), 1);
    std::cout << "num elems in front: " << num_elems << " val of 0: " << h_front[0] << std::endl; 
    sssp_pull_call(h_front, h_next, dist);
    num_elems = std::count(h_next.begin(), h_next.end(), 1);
    std::cout << "num elems in next front: " << num_elems << std::endl; 
    std::cout << "dist of 1: " << dist[1] << std::endl;
    next_frontier_dev.copyToDevice(h_next.data(), edges.num_nodes());  
    hammerblade::write_global_buffer_dma<int>(dist.data(), dist_dev, edges.num_nodes());
    device->freeze_cores();
    device->write_dma();
    device->unfreeze_cores();
    hammerblade::updateBucketWithGraphItVertexSubset<int>(next_frontier_dev, pq);
    std::fill(h_next.begin(), h_next.end(), 0);
  }
}
void host_sssp_push(BucketPriorityQueue<int> &pq, std::vector<int> &dist, int iter) {
  host_sssp_pull(pq, dist, iter);
}

int launch(int argc, char * argv[]){
  InputParser input(argc, argv);
  if(!input.cmdOptionExists("-g")) { 
  
    std::cerr << "no input args\n";
    for(auto i = 0; i < argc; i++) {
      std::cerr << argv[i] << " ";
    }
    std::cerr << std::endl;
    return 0;
  }
  std::string ucode_path = input.getRISCVFile();

  int iter = 0;
  //std::string iterstrbase = "iteration-";
  //auto pos = ucode_path.find(iterstrbase);
  //auto iterstr = ucode_path.substr(pos + iterstrbase.size(), std::string::npos);
  //std::stringstream ss(iterstr);
  //ss >> iter;
  std::cerr << "iteration: " << iter << std::endl;

  int version = 0; //pull-vertex
  if(ucode_path.find("push-vertex") != std::string::npos) {
    version = 1;
  }
  std::cerr << "load microcode" << std::endl;
  hammerblade::builtin_loadMicroCodeFromFile(ucode_path);
  std::cerr << "load graph" << std::endl;

  std::string graph_f = input.getCmdOption("-g");
  //std::string frontier_f = input.getCmdOption("-f");
  edges = hammerblade::builtin_loadWeightedEdgesFromFileToHB (graph_f.c_str()); 
  std::cerr << "out deg of 0: " << edges.out_degree(5) << "num edges: " << edges.num_edges() << std::endl;


  Device::Ptr device = Device::GetInstance(); 
  dist_dev = GlobalScalar<hb_mc_eva_t>("dist");
  hammerblade::init_global_array<int>(edges.num_nodes(), dist_dev);
  hammerblade::assign_val_dma<int>(0, edges.num_nodes(), (2147483647), dist_dev);
  int start_vertex = 0;
  //hammerblade::insert_val<int>(start_vertex, 0, dist_dev); 
 
  std::cerr << "init locks\n";
  GlobalScalar<hb_mc_eva_t> glbl_locks = GlobalScalar<hb_mc_eva_t>("locks");
  hammerblade::init_global_array<std::atomic<int>>(NUM_LOCKS, glbl_locks);
  std::atomic<int> tmp_array[NUM_LOCKS] = {};
  hammerblade::write_global_buffer_dma<std::atomic<int>>(tmp_array, glbl_locks, NUM_LOCKS);

  std::cerr << "doing batch dma write" << std::endl;
  device->freeze_cores();
  device->write_dma();
  device->unfreeze_cores();
  hammerblade::insert_val<int>(start_vertex, 0, dist_dev); 
  std::cerr << "init pq" << std::endl;
  BucketPriorityQueue<int> pq = BucketPriorityQueue<int>(edges.num_nodes(), &dist_dev, (hammerblade::BucketOrder)1, (hammerblade::PriorityOrder)0, (int) 128, (int) 32);

  std::cerr << "host side compute up to current iter: \n";
  std::vector<int> h_dist(edges.num_nodes(), 2147483647);
  if(version == 0) {
    host_sssp_pull(pq, h_dist, iter);
  } else {
    host_sssp_push(pq, h_dist, iter);
  } 
  hammerblade::write_global_buffer_dma<int>(h_dist.data(), dist_dev, edges.num_nodes());
  device->freeze_cores();
  device->write_dma();
  device->unfreeze_cores();

  std::cerr << "starting while loop" << std::endl;
  Vector<int32_t> next_frontier_dev;
  switch(version){
    case 0: { // do dense pull bfs
      //device->enqueueJob("init_kernel", hb_mc_dimension(X,Y), {edges.num_nodes()});
      //device->runJobs();
      for(int i = 0; i < 1; i++) //just doing one large iteration
      {
     
	std::cerr << "doing SSSP Delta Stepping kernel" << std::endl;
        //Vector<int32_t> frontier = hammerblade::getBucketWithGraphItVertexSubset<int>(pq);
        Vector<int32_t> frontier = pq.popDenseReadyVertexSet(); 
        std::cerr << "got frontier from pq\n";
        next_frontier_dev = Vector<int32_t>(edges.num_nodes());
        //next_frontier_dev.assign(0, edges.num_nodes(), 0);
        //device->freeze_cores();
        //device->write_dma();
        //device->unfreeze_cores();
        printf("0x%08x\n", frontier.getAddr());
        printf("next: 0x%08x\n", next_frontier_dev.getAddr());
        std::cerr << "initialized next front\n";
        device->enqueueJob("edgeset_apply_pull_parallel_weighted_deduplicated_from_vertexset_with_frontier_call",
                         hb_mc_dimension(X,Y),
                        {edges.getInIndicesAddr(),
                         edges.getInNeighborsAddr(),
                         frontier.getAddr(),
                         next_frontier_dev.getAddr(),  
                         edges.num_nodes(),
                         edges.num_edges(),
                         edges.num_nodes()});
        device->runJobs();
        std::cerr << "updating buckets:\n";
        hammerblade::updateBucketWithGraphItVertexSubset<int>(next_frontier_dev, pq);
        hammerblade::deleteObject(frontier);
    }
    break;
    }
    case 1: { //do sparse push blocked bfs
    break;
    } 
  }
        
  std::cerr << "finished while loop" << std::endl;

  if(VERIFY) {
    int * host_next = new int[edges.num_nodes()];
    next_frontier_dev.copyToHost(host_next, edges.num_nodes());

    device->freeze_cores();
    device->read_dma();
    device->unfreeze_cores();

    ofstream file("./frontier_verify.txt");
    if(!file.is_open()) std::cerr <<"couldn't open file\n";
    for(int i = 0; i < edges.num_nodes(); i++) {
      if(host_next[i] == 1 && i % 50 == 0) std::cerr << i << std::endl;
      file << host_next[i] << std::endl;
    }
    file.close();
  }
	device->finish();
  return 0;
}
#ifdef VCS 
int vcs_main(int argc, char ** argv) {
    bsg_pr_test_info("Unified Main Regression Test (COSIMULATION)\n");
    int rc = launch(argc,argv);
    bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
    return rc;
}
#else
int main(int argc, char ** argv) {
    bsg_pr_test_info("Unified Main CUDA Regression Test (F1)\n");
    int rc = launch(argc,argv);
    bsg_pr_test_pass_fail(rc == HB_MC_SUCCESS);
    return rc;
}
#endif
