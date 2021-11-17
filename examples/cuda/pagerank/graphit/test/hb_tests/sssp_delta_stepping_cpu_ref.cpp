#include <iostream> 
#include <vector>
#include <algorithm>
#include "intrinsics.h"
#ifdef GEN_PYBIND_WRAPPERS
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
namespace py = pybind11;
#endif
WGraph edges;
int  * __restrict dist;
julienne::PriorityQueue < int  >* pq; 
template <typename APPLY_FUNC > VertexSubset<NodeID>* edgeset_apply_push_serial_weighted_deduplicatied_from_vertexset_with_frontier(WGraph & g , VertexSubset<NodeID>* from_vertexset, APPLY_FUNC apply_func) 
{ 
    int64_t numVertices = g.num_nodes(), numEdges = g.num_edges();
    from_vertexset->toSparse();
    long m = from_vertexset->size();
    // used to generate nonzero indices to get degrees
    uintT *degrees = newA(uintT, m);
    // We probably need this when we get something that doesn't have a dense set, not sure
    // We can also write our own, the eixsting one doesn't quite work for bitvectors
    //from_vertexset->toSparse();
    {
        parallel_for (long i = 0; i < m; i++) {
            NodeID v = from_vertexset->dense_vertex_set_[i];
            degrees[i] = g.out_degree(v);
        }
    }
    uintT outDegrees = sequence::plusReduce(degrees, m);
    if (g.get_flags_() == nullptr){
      g.set_flags_(new int[numVertices]());
      parallel_for(int i = 0; i < numVertices; i++) g.get_flags_()[i]=0;
    }
    VertexSubset<NodeID> *next_frontier = new VertexSubset<NodeID>(g.num_nodes(), 0);
    if (numVertices != from_vertexset->getVerticesRange()) {
        cout << "edgeMap: Sizes Don't match" << endl;
        abort();
    }
    if (outDegrees == 0) return next_frontier;
    uintT *offsets = degrees;
    long outEdgeCount = sequence::plusScan(offsets, degrees, m);
    uintE *outEdges = newA(uintE, outEdgeCount);
  for (long i=0; i < m; i++) {
    NodeID s = from_vertexset->dense_vertex_set_[i];
    int j = 0;
    uintT offset = offsets[i];
    for(WNode d : g.out_neigh(s)){
      if( apply_func ( s , d.v, d.w ) && CAS(&(g.get_flags_()[d.v]), 0, 1)  ) { 
        outEdges[offset + j] = d.v; 
      } else { outEdges[offset + j] = UINT_E_MAX; }
      j++;
    } //end of for loop on neighbors
  }
  uintE *nextIndices = newA(uintE, outEdgeCount);
  long nextM = sequence::filter(outEdges, nextIndices, outEdgeCount, nonMaxF());
  free(outEdges);
  free(degrees);
  next_frontier->num_vertices_ = nextM;
  next_frontier->dense_vertex_set_ = nextIndices;
  parallel_for(int i = 0; i < nextM; i++){
     g.get_flags_()[nextIndices[i]] = 0;
  }
  return next_frontier;
} //end of edgeset apply function 
struct dist_generated_vector_op_apply_func_0
{
  void operator() (NodeID v) 
  {
    dist[v] = (2147483647) ;
  };
};
struct updateEdge
{
  bool operator() (NodeID src, NodeID dst, int weight) 
  {
    bool output3 ;
    bool dist_trackving_var_2 = (bool) 0;
    int new_dist = (dist[src] + weight);
    dist_trackving_var_2 = pq->updatePriorityMinAtomic(dst, dist[dst], new_dist);
    output3 = dist_trackving_var_2;
    return output3;
  };
};
struct printDist
{
  void operator() (NodeID v) 
  {
    std::cout << dist[v]<< std::endl;
  };
};
int main(int argc, char * argv[])
{
  edges = builtin_loadWeightedEdgesFromFile ( "../test/graphs/4.wel") ;
  dist = new int [ builtin_getVertices(edges) ];
  parallel_for (int vertexsetapply_iter = 0; vertexsetapply_iter < builtin_getVertices(edges) ; vertexsetapply_iter++) {
    dist_generated_vector_op_apply_func_0()(vertexsetapply_iter);
  };
  NodeID start_vertex = (0) ;
  dist[start_vertex] = (0) ;
  pq = new julienne::PriorityQueue <int  > ( edges.num_nodes(), dist, (julienne::bucket_order)1, (julienne::priority_order)0, 128);
  while ( (pq->finished() ) == ((bool) 0))
  {
    VertexSubset<int> *  frontier = getBucketWithGraphItVertexSubset(pq) ;
    VertexSubset<int> *  modified_vertexsubset1 = edgeset_apply_push_serial_weighted_deduplicatied_from_vertexset_with_frontier(edges, frontier, updateEdge()); 
    updateBucketWithGraphItVertexSubset(modified_vertexsubset1, pq, 0, 1);
    deleteObject(frontier) ;
  }
  parallel_for (int vertexsetapply_iter = 0; vertexsetapply_iter < builtin_getVertices(edges) ; vertexsetapply_iter++) {
    printDist()(vertexsetapply_iter);
  };
};
#ifdef GEN_PYBIND_WRAPPERS
PYBIND11_MODULE(, m) {
}
#endif

