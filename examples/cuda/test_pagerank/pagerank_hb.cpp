#include "hb_intrinsics.h"
#include <string.h> 
const std::string ucode_path = 
		 "/home/centos/bsg_bladerunner/bsg_manycore"
		 "/software/spmd/bsg_cuda_lite_runtime/graphit_kernel_code/main.riscv"
using hammerblade::Device;
using hammerblade::Vector;
using hammerblade::GraphHB;
using hammerblade::WGraphHB;
using hammerblade::GlobalScalar;
GraphHB edges; 
GlobalScalar<hb_mc_eva_t> old_rank_dev;
GlobalScalar<hb_mc_eva_t> new_rank_dev;
GlobalScalar<hb_mc_eva_t> out_degree_dev;
GlobalScalar<hb_mc_eva_t> error_dev;
GlobalScalar<hb_mc_eva_t> generated_tmp_vector_2_dev;
GlobalScalar<float > damp_dev;
GlobalScalar<float > beta_score_dev;
int main(int argc, char * argv[]){
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
  error_dev = GlobalScalar<hb_mc_eva_t>("error");
  hammerblade::init_global_array(hammerblade::builtin_getVerticesHB(edges), error_dev);
  hammerblade::assign_val(0, hammerblade::builtin_getVerticesHB(edges), ((float) 0) , error_dev);
  generated_tmp_vector_2_dev = GlobalScalar<hb_mc_eva_t>("generated_tmp_vector_2");
  hammerblade::init_global_array(hammerblade::builtin_getVerticesHB(edges), generated_tmp_vector_2_dev);
  hammerblade::assign_val(0, hammerblade::builtin_getVerticesHB(edges), builtin_getOutDegrees(edges) , generated_tmp_vector_2_dev);
  damp_dev = GlobalScalar<float >("damp");
  beta_score_dev = GlobalScalar<float >("beta_score");
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
  device->enqueueJob("error_generated_vector_op_apply_func_4_kernel",{hammerblade::builtin_getVerticesHB(edges)});
  device->runJobs();
  for ( int i = (1) ; i < (10) ; i++ )
  {
    device->enqueueJob("edgeset_apply_push_parallel_call", {edges.getOutIndicesAddr() , edges.getOutNeighborsAddr(), edges.num_nodes(), edges.num_edges(), edges.num_nodes()}); 
    device->runJobs();
    device->enqueueJob("updateVertex_kernel",{hammerblade::builtin_getVerticesHB(edges)});
    device->runJobs();
  }
  float sum = (0) ;
  for ( int i = (0) ; i < hammerblade::builtin_getVerticesHB(edges); i++ )
  {
    sum += error[i];
  }
}
