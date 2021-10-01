#include "hb_intrinsics.h"
#include "bsg_manycore_regression.h"
#include <string.h> 
const std::string ucode_path = 
		 "/work/global/lc873/work/sdh/playground/bsg_bladerunner-v6.2/bsg_replicant/examples/cuda/test_pagerank/pagrank_hb_device.riscv"; 
using hammerblade::Device;
using hammerblade::Vector;
using hammerblade::GraphHB;
using hammerblade::WGraphHB;
using hammerblade::GlobalScalar;
GraphHB edges; 
GlobalScalar<hb_mc_eva_t> old_rank_dev;
GlobalScalar<hb_mc_eva_t> new_rank_dev;
GlobalScalar<hb_mc_eva_t> out_degree_dev;
GlobalScalar<hb_mc_eva_t> contrib_dev;
GlobalScalar<hb_mc_eva_t> error_dev;
GlobalScalar<hb_mc_eva_t> generated_tmp_vector_2_dev;
GlobalScalar<double > damp_dev;
GlobalScalar<double > beta_score_dev;
int pagerank_main(int argc, char * argv[]){
  hammerblade::builtin_loadMicroCodeFromFile(ucode_path);
  old_rank_dev = GlobalScalar<hb_mc_eva_t>("old_rank");
  hammerblade::init_global_array(hammerblade::builtin_getVerticesHB(edges), old_rank_dev);
  hammerblade::assign_val(0, hammerblade::builtin_getVerticesHB(edges), (((float) 1)  / hammerblade::builtin_getVerticesHB(edges)), old_rank_dev);
  new_rank_dev = GlobalScalar<hb_mc_eva_t>("new_rank");
  hammerblade::init_global_array(hammerblade::builtin_getVerticesHB(edges), new_rank_dev);
  hammerblade::assign_val(0, hammerblade::builtin_getVerticesHB(edges), ((float) 0) , new_rank_dev);
  out_degree_dev = GlobalScalar<hb_mc_eva_t>("out_degree");
  hammerblade::init_global_array(hammerblade::builtin_getVerticesHB(edges), out_degree_dev);
  hammerblade::assign_val(0, hammerblade::builtin_getVerticesHB(edges), (0) , out_degree_dev);
  contrib_dev = GlobalScalar<hb_mc_eva_t>("contrib");
  hammerblade::init_global_array(hammerblade::builtin_getVerticesHB(edges), contrib_dev);
  hammerblade::assign_val(0, hammerblade::builtin_getVerticesHB(edges), ((float) 0) , contrib_dev);
  error_dev = GlobalScalar<hb_mc_eva_t>("error");
  hammerblade::init_global_array(hammerblade::builtin_getVerticesHB(edges), error_dev);
  hammerblade::assign_val(0, hammerblade::builtin_getVerticesHB(edges), ((float) 0) , error_dev);
  generated_tmp_vector_2_dev = GlobalScalar<hb_mc_eva_t>("generated_tmp_vector_2");
  hammerblade::init_global_array(hammerblade::builtin_getVerticesHB(edges), generated_tmp_vector_2_dev);
  hammerblade::assign_val(0, hammerblade::builtin_getVerticesHB(edges), builtin_getOutDegrees(edges) , generated_tmp_vector_2_dev);
  damp_dev = GlobalScalar<double >("damp");
  beta_score_dev = GlobalScalar<double >("beta_score");
  edges = hammerblade::builtin_loadEdgesFromFileToHB ( argv[(1) ]) ;
  Device::Ptr device = Device::GetInstance();
  damp = ((float) 0.85) ;
  beta_score = ((((float) 1)  - damp) / hammerblade::builtin_getVerticesHB(edges));
  device->enqueueJob("old_rank_generated_vector_op_apply_func_0_kernel",{hammerblade::builtin_getVerticesHB(edges)});
  device->runJobs();
  device->enqueueJob("new_rank_generated_vector_op_apply_func_1_kernel",{hammerblade::builtin_getVerticesHB(edges)});
  device->runJobs();
  generated_tmp_vector_2 = builtin_getOutDegrees(edges) ;
  device->enqueueJob("generated_vector_op_apply_func_3_kernel",{hammerblade::builtin_getVerticesHB(edges)});
  device->runJobs();
  device->enqueueJob("contrib_generated_vector_op_apply_func_4_kernel",{hammerblade::builtin_getVerticesHB(edges)});
  device->runJobs();
  device->enqueueJob("error_generated_vector_op_apply_func_5_kernel",{hammerblade::builtin_getVerticesHB(edges)});
  device->runJobs();
  for ( int trail = (0) ; trail < (10) ; trail++ )
  {
    device->enqueueJob("reset_kernel",{hammerblade::builtin_getVerticesHB(edges)});
    device->runJobs();
    for ( int i = (0) ; i < (20) ; i++ )
    {
      device->enqueueJob("computeContrib_kernel",{hammerblade::builtin_getVerticesHB(edges)});
      device->runJobs();
      device->enqueueJob("edgeset_apply_push_parallel_call", {edges.getOutIndicesAddr() , edges.getOutNeighborsAddr(), edges.num_nodes(), edges.num_edges(), edges.num_nodes()}); 
      device->runJobs();
      device->enqueueJob("updateVertex_kernel",{hammerblade::builtin_getVerticesHB(edges)});
      device->runJobs();
    }
  }
}
declare_program_main("pagerank", pagerank_main);
