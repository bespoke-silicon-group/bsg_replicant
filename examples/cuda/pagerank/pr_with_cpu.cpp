#define COSIM
#include "pr.hpp"
#define X 1 
#define Y 1

GraphHB edges;
Vector<float> old_rank_dev;
Vector<float> new_rank_dev;
Vector<int32_t> out_degree_dev;
Vector<float> contrib_dev;
Vector<float> error_dev;

GlobalScalar<float> damp_dev;
GlobalScalar<float> beta_score_dev;

Graph edges_cpu;
float* __restrict old_rank_hb;
float* __restrict old_rank_cpu;
float* __restrict new_rank_cpu;
int  * __restrict out_degree_cpu;
float* __restrict contrib_cpu;
float* __restrict error_cpu;
int  * __restrict generated_tmp_vector_cpu;
float damp_cpu;
float beta_score_cpu;

template <typename APPLY_FUNC > void edgeset_apply_pull_parallel(Graph & g , APPLY_FUNC apply_func)
{
    int64_t numVertices = g.num_nodes(), numEdges = g.num_edges();
  parallel_for ( NodeID d=0; d < g.num_nodes(); d++) {
    for(NodeID s : g.in_neigh(d)){
      apply_func ( s , d );
    } //end of loop on in neighbors
  } //end of outer for loop
} //end of edgeset apply function
struct error_generated_vector_op_apply_func_5
{
  void operator() (NodeID v)
  {
    error_cpu[v] = ((float) 0) ;
  };
};
struct contrib_generated_vector_op_apply_func_4
{
  void operator() (NodeID v)
  {
    contrib_cpu[v] = ((float) 0) ;
  };
};
struct generated_vector_op_apply_func_3
{
  void operator() (NodeID v)
  {
    out_degree_cpu[v] = generated_tmp_vector_cpu[v];
  };
};
struct new_rank_generated_vector_op_apply_func_1
{
  void operator() (NodeID v)
  {
    new_rank_cpu[v] = ((float) 0) ;
  };
};
struct old_rank_generated_vector_op_apply_func_0
{
  void operator() (NodeID v)
  {
    old_rank_cpu[v] = (((float) 1)  / builtin_getVertices(edges_cpu) );
  };
};
struct computeContrib
{
  void operator() (NodeID v)
  {
    contrib_cpu[v] = (old_rank_cpu[v] / out_degree_cpu[v]);
  };
};
struct updateEdge
{
  void operator() (NodeID src, NodeID dst)
  {
    new_rank_cpu[dst] += contrib_cpu[src];
  };
};

struct updateVertex
{
  void operator() (NodeID v)
  {
    double old_score = old_rank_cpu[v];
    new_rank_cpu[v] = (beta_score_cpu + (damp_cpu * new_rank_cpu[v]));
    error_cpu[v] = fabs((new_rank_cpu[v] - old_rank_cpu[v])) ;
    old_rank_cpu[v] = new_rank_cpu[v];
    new_rank_cpu[v] = ((float) 0) ;
  };
};
struct printRank
{
  void operator() (NodeID v)
  {
    std::cout << old_rank_cpu[v]<< std::endl;
  };
};
struct reset
{
  void operator() (NodeID v)
  {
    old_rank_cpu[v] = (((float) 1)  / builtin_getVertices(edges_cpu) );
    new_rank_cpu[v] = ((float) 0) ;
  };
};

int launch(int argc, char * argv[]){

//============================================ Mitigrate the CPU code here =================================================
  edges_cpu = builtin_loadEdgesFromFile ( argv_safe((3) , argv, argc)) ;
  old_rank_hb = new float [ builtin_getVertices(edges_cpu) ];
  old_rank_cpu = new float [ builtin_getVertices(edges_cpu) ];
  new_rank_cpu = new float [ builtin_getVertices(edges_cpu) ];
  out_degree_cpu = new int [ builtin_getVertices(edges_cpu) ];
  contrib_cpu = new float [ builtin_getVertices(edges_cpu) ];
  error_cpu = new float [ builtin_getVertices(edges_cpu) ];
  damp_cpu = ((float) 0.85) ;
  beta_score_cpu = ((((float) 1)  - damp_cpu) / builtin_getVertices(edges_cpu) );
  parallel_for (int vertexsetapply_iter = 0; vertexsetapply_iter < builtin_getVertices(edges_cpu) ; vertexsetapply_iter++) {
    old_rank_generated_vector_op_apply_func_0()(vertexsetapply_iter);
  };
  parallel_for (int vertexsetapply_iter = 0; vertexsetapply_iter < builtin_getVertices(edges_cpu) ; vertexsetapply_iter++) {
    new_rank_generated_vector_op_apply_func_1()(vertexsetapply_iter);
  };
  generated_tmp_vector_cpu = builtin_getOutDegrees(edges_cpu) ;
  parallel_for (int vertexsetapply_iter = 0; vertexsetapply_iter < builtin_getVertices(edges_cpu) ; vertexsetapply_iter++) {
    generated_vector_op_apply_func_3()(vertexsetapply_iter);
  };
  parallel_for (int vertexsetapply_iter = 0; vertexsetapply_iter < builtin_getVertices(edges_cpu) ; vertexsetapply_iter++) {
    contrib_generated_vector_op_apply_func_4()(vertexsetapply_iter);
  };
  parallel_for (int vertexsetapply_iter = 0; vertexsetapply_iter < builtin_getVertices(edges_cpu) ; vertexsetapply_iter++) {
    error_generated_vector_op_apply_func_5()(vertexsetapply_iter);
  };

  startTimer() ;
  parallel_for (int vertexsetapply_iter = 0; vertexsetapply_iter < builtin_getVertices(edges_cpu) ; vertexsetapply_iter++) {
    reset()(vertexsetapply_iter);
  };
  
  for ( int i = 0; i < 1 ; i++ ) {
    parallel_for (int vertexsetapply_iter = 0; vertexsetapply_iter < builtin_getVertices(edges_cpu) ; vertexsetapply_iter++) {
      computeContrib()(vertexsetapply_iter);
    };
    edgeset_apply_pull_parallel(edges_cpu, updateEdge());
    parallel_for (int vertexsetapply_iter = 0; vertexsetapply_iter < builtin_getVertices(edges_cpu) ; vertexsetapply_iter++) {
      updateVertex()(vertexsetapply_iter);
    };
  }

//=========================================The HB code is here ============================================================
  InputParser input(argc, argv);
  if(!input.cmdOptionExists("-g")){ 
  
    std::cerr << "no input args\n";
    return 0;
  }
  std::string ucode_path = input.getRISCVFile();

  int version = 0;
  if(ucode_path.find("push") != std::string::npos) {
    version = 1;
  }
  else if(ucode_path.find("beamer") != std::string::npos) {
    version = 2;
  }
  else if (ucode_path.find("blocked") != std::string::npos) {
    version = 3;
  }
  std::cerr << "load microcode" << std::endl;
  hammerblade::builtin_loadMicroCodeFromFile(ucode_path);
  std::cerr << "load graph" << std::endl;

  std::string graph_f = input.getCmdOption("-g");
  edges = hammerblade::builtin_loadEdgesFromFileToHB (graph_f.c_str()); 
 
  float tmp_rank_val = (((float) 1)  / builtin_getVertices(edges_cpu) );
  std::vector<float> zerosf(edges.num_nodes(), 0.0);
  std::vector<int32_t> zeros(edges.num_nodes(), 0);
  std::vector<float> rank(edges.num_nodes(), tmp_rank_val);
 
//  float* rank_test = rank.data();
//  for(int i = 0; i < edges.num_nodes(); i++) {
//    std::cout << rank_test[i] << std::endl;
//  }
  old_rank_dev = Vector<float>(edges.num_nodes());
  new_rank_dev = Vector<float>(edges.num_nodes());
  out_degree_dev = Vector<int32_t>(edges.num_nodes());
  contrib_dev = Vector<float>(edges.num_nodes());
  error_dev = Vector<float>(edges.num_nodes());

  damp_dev = GlobalScalar<float>("damp");
  beta_score_dev = GlobalScalar<float>("beta_score");

  Device::Ptr device = Device::GetInstance();

  float damp = ((float) 0.85);
  float beta_score = ((((float) 1) - damp) / edges.num_nodes());
  damp_dev.set(damp);
  beta_score_dev.set(beta_score);

  std::vector<int32_t> generated_tmp = edges.get_in_degrees();
  out_degree_dev.copyToDevice(generated_tmp.data(), edges.num_nodes());
  old_rank_dev.copyToDevice(rank.data(), rank.size());
  new_rank_dev.copyToDevice(zerosf.data(), zerosf.size());
  contrib_dev.copyToDevice(zerosf.data(), zerosf.size());
  error_dev.copyToDevice(zerosf.data(), zerosf.size());

  std::cerr << "doing batch dma write" << std::endl;
  //device->freeze_cores();
  device->write_dma();
  //device->unfreeze_cores();
  std::cerr << "starting while loop" << std::endl;
  
//  old_rank_dev.copyToHost(old_rank_hb, edges.num_nodes());
//  device->read_dma();
//  for(int i=0; i < edges.num_nodes(); i++) {
//    std::cout << "old_rank_hb[" << i << "] is " << old_rank_hb[i] << std::endl;
//  }
  switch(version){
    case 0: // do dense pull bfs
      for(int i = 0; i < 1; i++) //just doing one large iteration
      {

        device->enqueueJob("computeContrib_kernel", hb_mc_dimension(X,Y),{contrib_dev.getAddr(), old_rank_dev.getAddr(), out_degree_dev.getAddr(), edges.num_nodes()});
        device->runJobs();
        device->enqueueJob("edgeset_apply_pull_serial_call", hb_mc_dimension(X,Y),{edges.getInIndicesAddr() , edges.getInNeighborsAddr(), new_rank_dev.getAddr(), contrib_dev.getAddr(), edges.num_nodes(), edges.num_edges()});
        device->runJobs();
        device->enqueueJob("updateVertex_kernel", hb_mc_dimension(X,Y),{old_rank_dev.getAddr(), new_rank_dev.getAddr(), error_dev.getAddr(), edges.num_nodes(), edges.num_edges()});
        device->runJobs();
        std::cerr << "finished call" << std::endl;
    }
    break;
  case 1: //do blocked dense pull bfs
      for(int i = 0; i < 1; i++) //just doing one large iteration
      {
        device->enqueueJob("computeContrib_kernel", hb_mc_dimension(X,Y),{contrib_dev.getAddr(), old_rank_dev.getAddr(), out_degree_dev.getAddr(), edges.num_nodes()});
        device->runJobs();
        device->enqueueJob("edgeset_apply_push_serial_call", hb_mc_dimension(X,Y),{edges.getOutIndicesAddr() , edges.getOutNeighborsAddr(), new_rank_dev.getAddr(), contrib_dev.getAddr(), edges.num_nodes(), edges.num_edges()});
        device->runJobs();
        device->enqueueJob("updateVertex_kernel", hb_mc_dimension(X,Y),{old_rank_dev.getAddr(), new_rank_dev.getAddr(), error_dev.getAddr(), edges.num_nodes(), edges.num_edges()});
        device->runJobs();
      }
      break;
  case 3: //do blocked dense pull bfs
      for(int i = 0; i < 1; i++) //just doing one large iteration
      {
        device->enqueueJob("computeContrib_kernel", hb_mc_dimension(X,Y),{contrib_dev.getAddr(), old_rank_dev.getAddr(), out_degree_dev.getAddr(), edges.num_nodes()});
        device->runJobs();
        device->enqueueJob("edgeset_apply_pull_serial_call", hb_mc_dimension(X,Y),{edges.getInVertexlistAddr() , edges.getInNeighborsAddr(), new_rank_dev.getAddr(), contrib_dev.getAddr(), edges.num_nodes(), edges.num_edges()});
        device->runJobs();
        device->enqueueJob("updateVertex_kernel", hb_mc_dimension(X,Y),{old_rank_dev.getAddr(), new_rank_dev.getAddr(), error_dev.getAddr(), edges.num_nodes(), edges.num_edges()});
        device->runJobs();
      }
      break;
  }
 
  old_rank_dev.copyToHost(old_rank_hb, edges.num_nodes());
  device->read_dma();
  for(int i=0; i < edges.num_nodes(); i++) {
    std::cout << "old_rank_hb[" << i << "] is " << old_rank_hb[i] << " and old_rank_cpu[" << i << "] is " << old_rank_cpu[i] << std::endl;
    if (old_rank[i] != old_rank_cpu[i]) {
      std::cerr << "Result dose not equal, do not PASS the test !" << std::endl;
      break;
    }
  }
  std::cerr << "finished while loop" << std::endl;
  bool verify = true;

  return 0;
}

declare_program_main("GraphIt PageRank", launch);