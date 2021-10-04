#include "pr.hpp"

#define X 16 
#define Y 8

GraphHB edges;
Vector<float> old_rank_dev;
Vector<float> new_rank_dev;
Vector<int32_t> out_degree_dev;
Vector<float> contrib_dev;
Vector<float> error_dev;

GlobalScalar<float> damp_dev;
GlobalScalar<float> beta_score_dev;

int launch(int argc, char * argv[]){
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
 
  float tmp_rank_val = (float) 1 / edges.num_edges();
  std::vector<float> zerosf(edges.num_nodes(), 0.0);
  std::vector<int32_t> zeros(edges.num_nodes(), 0);
  std::vector<float> rank(edges.num_nodes(), tmp_rank_val);
 
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
        
  std::cerr << "finished while loop" << std::endl;
  bool verify = true;

  return 0;
}

declare_program_main("GraphIt PageRank", launch);
