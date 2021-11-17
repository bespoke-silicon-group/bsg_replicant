#include "hb_intrinsics.h"
#include <string.h> 
const std::string ucode_path = 
		 "/home/centos/bsg_bladerunner/bsg_manycore"
		 "/software/spmd/bsg_cuda_lite_runtime/graphit_kernel_code/main.riscv"
using hammerblade::Device;
using hammerblade::Vector;
using hammerblade::GraphHB;
using hammerblade::GlobalScalar;
GraphHB edges; 
GlobalScalar<hb_mc_eva_t> parent_dev;
int main(int argc, char * argv[]){
  hammerblade::builtin_loadMicroCodeFromFile(ucode_path);
  parent_dev = GlobalScalar<hb_mc_eva_t>("parent");
  hammerblade::init_global_array(hammerblade::builtin_getVerticesHB(edges), parent_dev);
  hammerblade::assign_val(0, hammerblade::builtin_getVerticesHB(edges),  -(1) , parent_dev);
  edges = hammerblade::builtin_loadEdgesFromFileToHB ( argv[(1) ]) ;
  Device::Ptr device = Device::GetInstance();
  device->enqueueJob("parent_generated_vector_op_apply_func_0_kernel",{hammerblade::builtin_getVerticesHB(edges)});
  device->runJobs();
  Vector<int32_t> frontier = new Vector<int32_t>(hammerblade::builtin_getVerticesHB(edges), 0);
  hammerblade::builtin_addVertexHB(frontier, (8) );
  hammerblade::insert_val((8), (8), parent_dev);
  while ( (hammerblade::builtin_getVertexSetSizeHB(frontier, edges.num_nodes()) ) != ((0) ))
  {
    Vector<int32_t> next_frontier = new Vector<int32_t>(edges.num_nodes(), 0);
    device->enqueueJob("edgeset_apply_push_parallel_from_vertexset_with_frontier_call", {edges.getOutVertexListAddr() , edges.getOutNeighborsAddr(), frontier.getAddr(), next_frontier.getAddr(), edges.num_nodes(), edges.num_edges(), edges.num_nodes()}); 
    device->runJobs();

    hammerblade::builtin_swapVectors(frontier, next_frontier);
    deleteObject(next_frontier);
  }
  deleteObject(frontier) ;
}
