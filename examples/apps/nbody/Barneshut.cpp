/*
 * This file belongs to the Galois project, a C++ library for exploiting
 * parallelism. The code is being released under the terms of the 3-Clause BSD
 * License (a copy is located in LICENSE.txt at the top-level directory).
 *
 * Copyright (C) 2018, The University of Texas at Austin. All rights reserved.
 * UNIVERSITY EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES CONCERNING THIS
 * SOFTWARE AND DOCUMENTATION, INCLUDING ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR ANY PARTICULAR PURPOSE, NON-INFRINGEMENT AND WARRANTIES OF
 * PERFORMANCE, AND ANY WARRANTY THAT MIGHT OTHERWISE ARISE FROM COURSE OF
 * DEALING OR USAGE OF TRADE.  NO WARRANTY IS EITHER EXPRESS OR IMPLIED WITH
 * RESPECT TO THE USE OF THE SOFTWARE OR DOCUMENTATION. Under no circumstances
 * shall University be liable for incidental, special, indirect, direct or
 * consequential damages or loss of profits, interruption of business, or
 * related expenses which may arise from use of Software or Documentation,
 * including but not limited to those resulting from defects in Software and/or
 * Documentation, or loss or inaccuracy of data of any kind.
 */

#include <bsg_manycore_regression.h>
#include <bsg_manycore_cuda.h>

// One-off replacement for BSG_CUDA_CALL
#define HB_MC_CUDA_CALL(stmt)                                             \
        {                                                               \
                int __r = stmt;                                         \
                if (__r != HB_MC_SUCCESS) {                             \
                        bsg_pr_err("'%s' failed: %s\n", #stmt, hb_mc_strerror(__r)); \
                        exit(1);                                        \
                }                                                       \
        }


#include "galois/Galois.h"
#include "galois/Timer.h"
#include "galois/Bag.h"
#include "galois/Reduction.h"
#include "Lonestar/BoilerPlate.h"
#include "galois/runtime/Profile.h"

#include <boost/math/constants/constants.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <limits>
#include <iostream>
#include <fstream>
#include <random>
#include <deque>

#include <strings.h>

#include <Config.hpp>
#include <Point.hpp>
#include <Node.hpp>
#include <Body.hpp>
#include <Octree.hpp>
#include <BoundingBox.hpp>

std::ostream& operator<<(std::ostream& os, const Point& p) {
        os << "(" << p[0] << "," << p[1] << "," << p[2] << ")";
        return os;
}

const char* name = "Barnes-Hut N-Body Simulator";
const char* desc =
        "Simulates gravitational forces in a galactic cluster using the "
        "Barnes-Hut n-body algorithm";
const char* url = "barneshut";

static llvm::cl::opt<int>
nbodies("n", llvm::cl::desc("Number of bodies (default value 10000)"),
        llvm::cl::init(10000));
static llvm::cl::opt<int>
ntimesteps("steps", llvm::cl::desc("Number of steps (default value 1)"),
           llvm::cl::init(1));
static llvm::cl::opt<int> seed("seed",
                               llvm::cl::desc("Random seed (default value 7)"),
                               llvm::cl::init(7));

static llvm::cl::opt<std::string> kpath("k",
                               llvm::cl::desc("Kernel path (default value: kernel.riscv)"),
                               llvm::cl::init("kernel.riscv"));

Config config;

/**
 * InsertBag: Unordered collection of elements. This data structure
 * supports scalable concurrent pushes but reading the bag can only be
 * done serially.
 */
typedef galois::InsertBag<Body> Bodies;
typedef galois::InsertBag<Body*> BodyPtrs;
// FIXME: reclaim memory for multiple steps
typedef galois::InsertBag<Octree> Tree;

// DR: Build an oct tree (duh)
struct BuildOctree {

        Tree& T;

        // DR: Insert body b into octree,
        void insert(Body* b, Octree* node, float radius) const {

                // DR: Get the octant index by comparing the x,y,z of
                // the body with the oct tree node.
                int index   = node->pos.getChildIndex(b->pos);

                // DR: getValue() is for lock
                // DRTODO: Relaxed ordering? So not actually a lock?
                // DR: Get the child node at the octant index
                Node* child = node->child[index].getValue();

                // go through the tree lock-free while we can
                // DR: Recurse if the child node exists, and it is not a leaf.
                if (child && !child->Leaf) {
                        insert(b, static_cast<Octree*>(child), radius * .5f);
                        return;
                }

                // DR: Else, acquire the lock for that index
                // This locking may be too fined grain?
                node->child[index].lock();
                child = node->child[index].getValue();

                // DR: If the child is null, set the value at the
                // index to the current body and unlock
                if (child == NULL) {
                        b->pred = node;
                        b->octant = index;
                        node->child[index].unlock_and_set(b);
                        return;
                }

                // DR: I believe this is only partially correct and is
                // a bug. I think the recursion above should also
                // decrease the radius by .5
                radius *= 0.5f;

                // DR: If the child is a leaf, create a new node (this is where the real fun begins)
                if (child->Leaf) {
                        // Expand leaf

                        // DR: Create a new node. Using the index
                        // (which determines the octant), compute the
                        // radius as an offset from the current
                        // (x,y,z) positiion.
                        Octree* new_node = &T.emplace(updateCenter(node->pos, index, radius));
                               
                        // DR: If the position of the former child,
                        // and the current child are identical then
                        // add some jitter to guarantee uniqueness.
                        if (b->pos == child->pos) {
                                printf("Jittering\n");
                                // Jitter point to gaurantee uniqueness.
                                float jitter = config.tol / 2;
                                assert(jitter < radius);
                                b->pos += (new_node->pos - b->pos) * jitter;
                        }

                        // assert(node->pos != b->pos);
                        // node->child[index].unlock_and_set(new_node);

                        // DR: Recurse, on the new node, inserting b
                        // again (this time it will succeed)
                        insert(b, new_node, radius);
                        // DR: Recurse, on the new node, inserting the child
                        insert(static_cast<Body*>(child), new_node, radius);

                        // DR: Finish
                        new_node->pred = node;
                        new_node->octant = index;
                        node->child[index].unlock_and_set(new_node);
                } else {
                        // DR: Someone beat us to the lock, and now we no longer need it.
                        node->child[index].unlock();
                        insert(b, static_cast<Octree*>(child), radius);
                }
        }
};

int hb_mc_manycore_device_build_tree(hb_mc_device_t *device, eva_t _config, unsigned int *nNodes, HBOctree *hnodes, eva_t _hnodes, unsigned int nBodies, HBBody *hbodies, eva_t _hbodies, float radius){
        printf("Root Position: %2.4f %2.4f %2.4f, Radius: %2.4f \n", hnodes[0].pos[0], hnodes[0].pos[1], hnodes[0].pos[2], radius);
        hb_mc_dma_htod_t htod_bodies = {
                .d_addr = _hbodies,
                .h_addr = hbodies,
                .size   = sizeof(HBBody) * nBodies
        };

        hb_mc_dma_htod_t htod_nodes = {
                .d_addr = _hnodes,
                .h_addr = hnodes,
                .size   = sizeof(HBOctree) * (*nNodes)
        };

        unsigned int body_idx = TILE_GROUP_DIM_X * TILE_GROUP_DIM_Y;
        eva_t _body_idx;
        HB_MC_CUDA_CALL(hb_mc_device_malloc(device, sizeof(body_idx), &_body_idx));
        hb_mc_dma_htod_t htod_body_idx = {
                .d_addr = _body_idx,
                .h_addr = &body_idx,
                .size   = sizeof(body_idx)
        };

        // Node 0, the root, is already created.
        // node_idx is the location of the first free node.
        unsigned int node_idx = TILE_GROUP_DIM_X * TILE_GROUP_DIM_Y + 1;
        eva_t _node_idx;
        HB_MC_CUDA_CALL(hb_mc_device_malloc(device, sizeof(node_idx), &_node_idx));
        hb_mc_dma_htod_t htod_node_idx = {
                .d_addr = _node_idx,
                .h_addr = &node_idx,
                .size   = sizeof(node_idx)
        };

        HB_MC_CUDA_CALL(hb_mc_device_dma_to_device(device, &htod_body_idx, 1));
        HB_MC_CUDA_CALL(hb_mc_device_dma_to_device(device, &htod_node_idx, 1));
        HB_MC_CUDA_CALL(hb_mc_device_dma_to_device(device, &htod_bodies, 1));
        HB_MC_CUDA_CALL(hb_mc_device_dma_to_device(device, &htod_nodes, 1));

        hb_mc_dimension_t tg_dim = { .x = TILE_GROUP_DIM_X, .y = TILE_GROUP_DIM_Y};
        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1};
        // We can't transfer floats as arguments directly, so we pass it encoded as a binary value
        
        uint32_t fradius = *reinterpret_cast<uint32_t *>(&radius);

        //extern "C" void build(Config *pcfg, HBOctree *nodes, int nNodes, int *nidx, HBBody *bodies, int nBodies, int *bidx, unsigned int _radius){
        uint32_t cuda_argv[8] = {_config, _hnodes, (*nNodes), _node_idx, _hbodies, nBodies, _body_idx, fradius};
        HB_MC_CUDA_CALL(hb_mc_kernel_enqueue (device, grid_dim, tg_dim, "build", 8, cuda_argv));

        /* Launch and execute all tile groups on device and wait for all to finish.  */
        HB_MC_CUDA_CALL(hb_mc_device_tile_groups_execute(device));

        hb_mc_dma_dtoh_t dtoh_nnodes = {
                .d_addr = _node_idx,
                .h_addr = nNodes,
                .size   = sizeof(node_idx)
        };
        HB_MC_CUDA_CALL(hb_mc_device_dma_to_host(device, &dtoh_nnodes, 1));

        hb_mc_dma_dtoh_t dtoh_nodes = {
                .d_addr = _hnodes,
                .h_addr = hnodes,
                .size   = sizeof(HBOctree) * (*nNodes)
        };

        HB_MC_CUDA_CALL(hb_mc_device_dma_to_host(device, &dtoh_nodes, 1));
        HB_MC_CUDA_CALL(hb_mc_device_free(device, _body_idx));
        HB_MC_CUDA_CALL(hb_mc_device_free(device, _node_idx));

        /* Code doesn't work, but may be useful:
        for(int n_i = 0; n_i < *nNodes; n_i++){
                HBOctree *cur = &hnodes[n_i];
                printf("Cur Position: %2.4f %2.4f %2.4f \n", cur->pos[0], cur->pos[1], cur->pos[2]);

                for(int c_i = 0; c_i <8; c_i++){
                        eva_t child_eva = cur->child[c_i];
                        int idx = (_hbodies - child_eva) / sizeof(*hbodies);
                        int rem = (_hbodies - child_eva) % sizeof(*hbodies); // Should be 0
                        if(rem != 0){
                                bsg_pr_err("Oops! Non-zero remainder\n");
                                exit(1);
                        }
                        
                        if(idx){
                                HBBody *child = &hbodies[idx];
                           printf("    Leaf %d : %2.4f %2.4f %2.4f \n", child->pos[1], child->pos[1], child->pos[2]);
                        }
                }
        }
        */
        

        return HB_MC_SUCCESS;
}

// DR: Recursively compute center of mass
unsigned computeCenterOfMass(Octree* node) {
        float mass = 0.0;
        Point accum;
        // DR: Why isn't num 0?
        unsigned num = 1;

        // DR: Count of indices
        int index = 0;
        // DR: For each of the children within a node
        for (int i = 0; i < 8; ++i)
                // DR: If the node exists (not null), densify by
                // shifting the values at the indices toward 0
                if (node->child[i].getValue())
                        node->child[index++].setValue(node->child[i].getValue());

        // DR: Set the remaining to 0
        for (int i = index; i < 8; ++i)
                node->child[i].setValue(NULL);

        // DR: Set nChildren
        node->nChildren = index;
        node->cLeafs = 0;

        // DR: For each index
        for (int i = 0; i < index; i++) {
                // DR: Get the child at that index
                Node* child = node->child[i].getValue();
                // DR: If the child is not a leaf, recurse, and compute it's center of mass
                // computeCenterOfMass returns the number of leaves below a node (plus 1?)
                if (!child->Leaf) {
                        num += computeCenterOfMass(static_cast<Octree*>(child));
                } else {
                        // DR: Set the leaf mask, and update number of leaves
                        node->cLeafs |= (1 << i);
                        ++num;
                }
                // DR: Update running mass total
                mass += child->mass;
                // DR: Update weighted position
                accum += child->pos * child->mass;
        }

        // DR: Update self mass
        node->mass = mass;

        // DR: Update center of mass for node?
        if (mass > 0.0)
                node->pos = accum / mass;
        return num;
}

int hb_mc_manycore_device_summarize_centers(hb_mc_device_t *device, eva_t _config, unsigned int nNodes, HBOctree *hnodes, eva_t _hnodes, unsigned int nBodies, HBBody *hbodies, eva_t _hbodies){
        hb_mc_dma_htod_t htod_bodies = {
                .d_addr = _hbodies,
                .h_addr = hbodies,
                .size   = sizeof(HBBody) * nBodies
        };

        hb_mc_dma_htod_t htod_nodes = {
                .d_addr = _hnodes,
                .h_addr = hnodes,
                .size   = sizeof(HBOctree) * nNodes
        };

        unsigned int idx = 127;
        eva_t _idx;
        HB_MC_CUDA_CALL(hb_mc_device_malloc(device, sizeof(idx), &_idx));
        hb_mc_dma_htod_t htod_idx = {
                .d_addr = _idx,
                .h_addr = &idx,
                .size   = sizeof(idx)
        };

        hb_mc_dimension_t tg_dim = { .x = TILE_GROUP_DIM_X, .y = TILE_GROUP_DIM_Y};
        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1};
        // We can't transfer floats as arguments directly, so we pass it encoded as a binary value
        uint32_t cuda_argv[4] = {_hnodes, _hbodies, nBodies, _idx};

        HB_MC_CUDA_CALL(hb_mc_device_dma_to_device(device, &htod_idx, 1));
        HB_MC_CUDA_CALL(hb_mc_device_dma_to_device(device, &htod_bodies, 1));
        HB_MC_CUDA_CALL(hb_mc_device_dma_to_device(device, &htod_nodes, 1));
        HB_MC_CUDA_CALL(hb_mc_kernel_enqueue (device, grid_dim, tg_dim, "summarize", 4, cuda_argv));

        /* Launch and execute all tile groups on device and wait for all to finish.  */
        HB_MC_CUDA_CALL(hb_mc_device_tile_groups_execute(device));

        hb_mc_dma_dtoh_t dtoh = {
                .d_addr = _hnodes,
                .h_addr = hnodes,
                .size   = sizeof(HBOctree) * nNodes
        };

        HB_MC_CUDA_CALL(hb_mc_device_dma_to_host(device, &dtoh, 1));
        HB_MC_CUDA_CALL(hb_mc_device_free(device, _idx));

        return HB_MC_SUCCESS;
}

Point updateForce(Point delta, float psq, float mass) {
        // Computing force += delta * mass * (|delta|^2 + eps^2)^{-3/2}
        float idr   = 1.0f / sqrt((float)(psq + config.epssq));
        float scale = mass * idr * idr * idr;
        return delta * scale;
}

struct ComputeForces {
        // Optimize runtime for no conflict case

        Octree* top;
        float root_dsq;

        ComputeForces(Octree* _top, float diameter) : top(_top) {
                assert(diameter > 0.0 && "non positive diameter of bb");
                root_dsq = diameter * diameter * config.itolsq;
        }

        // DR: Compute forces on a body
        template <typename Context>
        void computeForce(Body* b, Context& cnx) {
                Point p = b->acc;
                b->acc  = Point(0.0, 0.0, 0.0);
                iterate(*b, cnx);
                b->vel += (b->acc - p) * config.dthf;
        }

        struct Frame {
                float dsq;
                Octree* node;
                Frame(Octree* _node, float _dsq) : dsq(_dsq), node(_node) {}
        };

        // DR: "Thread Loop" for computing the foces on a body
        template <typename Context>
        void iterate(Body& b, Context& cnx) {
                std::deque<Frame, galois::PerIterAllocTy::rebind<Frame>::other> stack(
                                                                                      cnx.getPerIterAlloc());
                // DR: Perform DFS traversal of tree. If a node is
                // "far enough" away, then do not consider its leaves
                // and only consider it's center of mass.
                //
                // This is the Barnes-Hut approximation. If "far
                // enough" is too large, this algorithm degrades to
                // O(N^2).
                stack.push_back(Frame(top, root_dsq));

                while (!stack.empty()) {
                        const Frame f = stack.back();
                        stack.pop_back();

                        // DR: Compute distance squared (to avoid sqrt)
                        Point p   = b.pos - f.node->pos;
                        float psq = p.dist2();

                        // Node is far enough away, summarize contribution
                        // DR: If node "Far enough", summarize using
                        // center of mass instead of individual bodies
                        if (psq >= f.dsq) {
                                b.acc += updateForce(p, psq, f.node->mass);
                                continue;
                        }

                        // DR: If the node is not far enough, recurse
                        // by adding all sub-nodes to the stack, and
                        // adding the contribution of the individual
                        // bodies.  DR: Why /4?
                        float dsq = f.dsq * 0.25;
                        // DR: Iterate through the children
                        for (int i = 0; i < f.node->nChildren; i++) {
                                Node* n = f.node->child[i].getValue();
                                assert(n);
                                // DR: If a sub-node is a body/leaf
                                if (f.node->cLeafs & (1 << i)) {
                                        assert(n->Leaf);
                                        if (static_cast<const Node*>(&b) != n) {
                                                Point p = b.pos - n->pos;
                                                b.acc += updateForce(p, p.dist2(), n->mass);
                                        }
                                } else {
#ifndef GALOIS_CXX11_DEQUE_HAS_NO_EMPLACE
                                        stack.emplace_back(static_cast<Octree*>(n), dsq);
#else
                                        stack.push_back(Frame(static_cast<Octree*>(n), dsq));
#endif
                                        __builtin_prefetch(n);
                                }
                        }
                }
        }
};

struct centerXCmp {
        template <typename T>
        bool operator()(const T& lhs, const T& rhs) const {
                return lhs.pos[0] < rhs.pos[0];
        }
};

struct centerYCmp {
        template <typename T>
        bool operator()(const T& lhs, const T& rhs) const {
                return lhs.pos[1] < rhs.pos[1];
        }
};

struct centerYCmpInv {
        template <typename T>
        bool operator()(const T& lhs, const T& rhs) const {
                return rhs.pos[1] < lhs.pos[1];
        }
};

template <typename Iter, typename Gen>
void divide(const Iter& b, const Iter& e, Gen& gen) {
        if (std::distance(b, e) > 32) {
                std::sort(b, e, centerXCmp());
                Iter m = galois::split_range(b, e);
                std::sort(b, m, centerYCmpInv());
                std::sort(m, e, centerYCmp());
                divide(b, galois::split_range(b, m), gen);
                divide(galois::split_range(b, m), m, gen);
                divide(m, galois::split_range(m, e), gen);
                divide(galois::split_range(m, e), e, gen);
        } else {
                std::shuffle(b, e, gen);
        }
}


struct CheckAllPairs {
        Bodies& bodies;

        CheckAllPairs(Bodies& b) : bodies(b) {}

        float operator()(const Body& body) const {
                const Body* me = &body;
                Point acc;
                for (Bodies::iterator ii = bodies.begin(), ei = bodies.end(); ii != ei;
                     ++ii) {
                        Body* b = &*ii;
                        if (me == b)
                                continue;
                        Point delta = me->pos - b->pos;
                        float psq  = delta.dist2();
                        acc += updateForce(delta, psq, b->mass);
                }

                float dist2 = acc.dist2();
                acc -= me->acc;
                float retval = acc.dist2() / dist2;
                return retval;
        }
};

float checkAllPairs(Bodies& bodies, int N) {
        Bodies::iterator end(bodies.begin());
        std::advance(end, N);

        return galois::ParallelSTL::map_reduce(bodies.begin(), end,
                                               CheckAllPairs(bodies),
                                               std::plus<float>(), 0.0) /
                N;
}

std::ostream& operator<<(std::ostream& os, const Body& b) {
        os << "(pos:" << b.pos << " vel:" << b.vel << " acc:" << b.acc
           << " mass:" << b.mass << ")";
        return os;
}

std::ostream& operator<<(std::ostream& os, const BoundingBox& b) {
        os << "(min:" << b.min << " max:" << b.max << ")";
        return os;
}

std::ostream& operator<<(std::ostream& os, const Config& c) {
        os << "Barnes-Hut configuration:"
           << " dtime: " << c.dtime << " eps: " << c.eps << " tol: " << c.tol;
        return os;
}

/*
void printRec(std::ofstream& file, Node* node, unsigned level) {
        static const char* ct[] = {
                "blue", "cyan", "aquamarine", "chartreuse",
                "darkorchid", "darkorange",
                "deeppink", "gold", "chocolate"
        };

        if (!node) return;
        // DR: node->idx used to be node->owner, but node doesn't have an owner field.
        file << "\"" << node << "\" [color=" << ct[node->idx / 4] << (node->idx % 4 + 1)
             << (level ? "" : " style=filled") << " label = \"" << (node->Leaf ? "L" : "N")
             << "\"];\n";
        if (!node->Leaf) {
                Octree* node2 = static_cast<Octree*>(node);
                for (int i = 0; i < 8 && node2->child[i].getValue(); ++i) {
                        if (level == 3 || level == 6)
                                file << "subgraph cluster_" << level << "_" << i << " {\n";
                        file << "\"" << node << "\" -> \"" << node2->child[i].getValue() << "\"[weight=0.01]\n";
                        printRec(file, node2->child[i].getValue(), level + 1); if (level == 3 || level == 6) file << "}\n";
                }
}
        }

void printTree(Octree* node) {
        std::ofstream file("out.txt");
        file << "digraph octree {\n";
        file << "ranksep = 2\n";
        file << "root = \"" << node << "\"\n";
        //  file << "overlap = scale\n";
        printRec(file, node, 0);
        file << "}\n";
}
*/

/**
 * Generates random input according to the Plummer model, which is more
 * realistic but perhaps not so much so according to astrophysicists
 */
void generateInput(Bodies& bodies, BodyPtrs& pBodies, int nbodies, int seed) {
        float v, sq, scale;
        Point p;
        float PI = boost::math::constants::pi<float>();

        std::mt19937 gen(seed);
#if __cplusplus >= 201103L || defined(HAVE_CXX11_UNIFORM_INT_DISTRIBUTION)
        std::uniform_real_distribution<float> dist(0, 1);
#else
        std::uniform_real<float> dist(0, 1);
#endif

        float rsc = (3 * PI) / 16;
        float vsc = sqrt(1.0 / rsc);

        std::vector<Body> tmp;

        for (int body = 0; body < nbodies; body++) {
                float r = 1.0 / sqrt(pow(dist(gen) * 0.999, -2.0 / 3.0) - 1);
                do {
                        for (int i = 0; i < 3; i++)
                                p[i] = dist(gen) * 2.0 - 1.0;
                        sq = p.dist2();
                } while (sq > 1.0);
                scale = rsc * r / sqrt(sq);

                Body b;
                b.mass = 1.0 / nbodies;
                b.pos  = p * scale;
                do {
                        p[0] = dist(gen);
                        p[1] = dist(gen) * 0.1;
                } while (p[1] > p[0] * p[0] * pow(1 - p[0] * p[0], 3.5));
                v = p[0] * sqrt(2.0 / sqrt(1 + r * r));
                do {
                        for (int i = 0; i < 3; i++)
                                p[i] = dist(gen) * 2.0 - 1.0;
                        sq = p.dist2();
                } while (sq > 1.0);
                scale  = vsc * v / sqrt(sq);
                b.vel  = p * scale;
                b.Leaf = true;
                tmp.push_back(b);
                // pBodies.push_back(&bodies.push_back(b));
        }

        // sort and copy out
        divide(tmp.begin(), tmp.end(), gen);

        galois::do_all(
                       galois::iterate(tmp),
                       [&pBodies, &bodies](const Body& b) {
                               pBodies.push_back(&(bodies.push_back(b)));
                       },
                       galois::loopname("InsertBody"));
}

/**
 * Convert an x86 pointer to a corresponding EVA pointer using the
 * type and index, and check for buffer overrun.
 * @param[in]  buf    Buffer pointer #eva
 * @param[in]  sz     Buffer size (in bytes)
 * @param[in]  index  Array index to convert
 * @param[out] hb_p   Starting #eva of the array index
 * @return HB_MC_INVALID the index is larger than can be represented
 * by the buffer. HB_MC_SUCCESS otherwise.
 */
template <typename T>
__attribute__((warn_unused_result))
int hb_mc_manycore_eva_translate(eva_t buf,
                                 size_t sz,
                                 unsigned int idx,
                                 eva_t *hb_p){
        *hb_p = buf + idx * sizeof(T);
        if(*hb_p > (buf + sz)){
                printf("Error in translation!\n");
                exit(1);
                return HB_MC_INVALID;
        }
        return HB_MC_SUCCESS;
}

int hb_mc_manycore_check_host_conversion_nodes(unsigned int nNodes, Octree **nodes, HBOctree *hnodes, eva_t _hnodes){
        // DR: Check that the tree conversion happened correctly
        NodeIdx node_i = 0;
        // Don't check the root node (index 0) because it points to itself, start at index 1.
        for (node_i = 1; node_i < nNodes; node_i ++){
                Octree *n = nodes[node_i];
                HBOctree *_n = &hnodes[node_i];
                int pred_i = (_n->pred - _hnodes)/sizeof(HBOctree);
                // Check that Host and HB predecessors correspond
                if(n->pred != nodes[pred_i]){
                        printf("Error: Node %d predecessor does not match HBNode!\n", node_i);
                        return HB_MC_FAIL;
                }

                // Check data match
                if(!n->isMatch(*_n)){
                        printf("Node %d Data Mismatch\n", node_i);
                        return HB_MC_FAIL;
                }

                int octant_i = n->pred->pos.getChildIndex(_n->pos);
                eva_t _p = hnodes[pred_i].child[octant_i];
                if(_p != (_hnodes + node_i * sizeof(HBOctree))){
                        printf("Error: HBOctree Node %u EVA does not match predecessor EVA @ Octant index!\n", node_i);
                        return HB_MC_FAIL;
                }
        }
        return HB_MC_SUCCESS;
}

int hb_mc_manycore_check_host_conversion_bodies(unsigned int nBodies, Body **bodies, HBBody *hbodies, eva_t _hbodies, Octree **nodes, HBOctree *hnodes, eva_t _hnodes){
        BodyIdx body_i = 0;
        for (body_i = 0 ; body_i < nBodies; body_i ++){
                Body *b = bodies[body_i];
                HBBody *_b = &hbodies[body_i];
                int pred_i = (_b->pred - _hnodes)/sizeof(HBOctree);
                // Check that Host and HB predecessors correspond
                if(b->pred != nodes[pred_i]){
                        printf("Error: Body %d predecessor does not match HBBody!\n", body_i);
                        return HB_MC_FAIL;
                }

                if(!b->isMatch(*_b)){
                        printf("Body %d Data Mismatch\n", body_i);
                        return HB_MC_FAIL;
                }

                // Check the predecessor's corresponding octant child eva matches the current body's eva.
                int octant_i = b->pred->pos.getChildIndex(_b->pos);
                eva_t _c = hnodes[pred_i].child[octant_i];
                if(_c != (_hbodies + body_i * sizeof(HBBody))){
                        printf("Error: HBBody %d EVA does not match predecessor EVA @ Octant index!\n", body_i);
                        return HB_MC_FAIL;
                }
        }
        return HB_MC_SUCCESS;
}

// nodes[0] is the tree root
int hb_mc_manycore_host_build_tree(unsigned int nBodies, Body **bodies, HBBody *hbodies, eva_t _hbodies,
                                   unsigned int nNodes, Octree **nodes, HBOctree *hnodes, eva_t _hnodes){

                // Map of Octree node/body pointer to index
                std::map<Node*, NodeIdx> nodeIdxMap;
                std::map<Body*, BodyIdx> bodyIdxMap;

        NodeIdx node_i = 0;
        BodyIdx body_i = 0;
        int err = HB_MC_SUCCESS;

        // Start with the root node/top, and assign it index 0.
        std::deque<Octree*> q;
        q.push_back(nodes[0]);
        // The root's predecessor points to itself
        nodes[0]->pred = nodes[0];
        nodes[0]->convert(_hnodes, hnodes[node_i]);
        nodeIdxMap[nodes[0]] = node_i++;

        while(!q.empty()){
                Octree *cur_p = q.back();
                q.pop_back();
                NodeIdx cur_i = nodeIdxMap[cur_p];

                // Get this node's EVA Pointer to set children predecessors
                eva_t _cur_p;
                err = hb_mc_manycore_eva_translate<HBOctree>(_hnodes, sizeof(HBOctree) * nNodes, cur_i, &_cur_p);

                for (int c_i = 0; c_i < Octree::octants; c_i++) {
                        Node *child = cur_p->child[c_i].getValue();
                        if (child){
                                cur_p->nChildren++; // Will be reset by CoM computation
                                hnodes[cur_i].nChildren++;
                                child->pred = cur_p;
                                if(!child->Leaf) {
                                        // Internal octree node
                                        Octree *c = static_cast<Octree*>(child);

                                        // Convert Octree node to HBOctree node
                                        c->convert(_cur_p, hnodes[node_i]);

                                        // Set up child EVA pointer
                                        err = hb_mc_manycore_eva_translate<HBOctree>(_hnodes, sizeof(HBOctree) * nNodes, node_i, &hnodes[cur_i].child[c_i]);

                                        nodes[node_i] = c;
                                        nodeIdxMap[c] = node_i++;

                                        q.push_back(c);
                                } else {
                                        hnodes[cur_i].cLeafs++;
                                        cur_p->cLeafs++;
                                        // Leaf/Body
                                        Body *b = static_cast<Body*>(child);
                                        bodies[body_i] = b;

                                        // Convert Octree Body to HBBody
                                        b->convert(_cur_p, hbodies[body_i]);

                                        // Set up child EVA pointer
                                        err = hb_mc_manycore_eva_translate<HBBody>(_hbodies, sizeof(HBBody) * nBodies, body_i, &hnodes[cur_i].child[c_i]);

                                        bodyIdxMap[b] = body_i++;
                                }
                        }
                }
        }
        err = hb_mc_manycore_check_host_conversion_nodes(nNodes, nodes, hnodes, _hnodes);
        if(err != HB_MC_SUCCESS)
                exit(1);
        err = hb_mc_manycore_check_host_conversion_bodies(nBodies, bodies, hbodies, _hbodies, nodes, hnodes, _hnodes);
        if(err != HB_MC_SUCCESS)
                exit(1);
        return HB_MC_SUCCESS;
}

int hb_mc_manycore_device_compute_forces(hb_mc_device_t *device, eva_t _config, float diamsq, unsigned int nNodes, HBOctree *hnodes, eva_t _hnodes, unsigned int nBodies, HBBody *hbodies, eva_t _hbodies){
        hb_mc_dma_htod_t htod_bodies = {
                .d_addr = _hbodies,
                .h_addr = hbodies,
                .size   = sizeof(HBBody) * nBodies
        };

        hb_mc_dma_htod_t htod_nodes = {
                .d_addr = _hnodes,
                .h_addr = hnodes,
                .size   = sizeof(HBOctree) * nNodes
        };
        for(NodeIdx i =0 ; i < nNodes; i++){
                printf("Node: %u, %lx, pred: %x\n", i, _hnodes + sizeof(HBOctree) * i, hnodes[i].pred);
                for(int c = 0; c < Octree::octants; c++){
                        printf("\t Child: %x\n", hnodes[i].child[c]);
                }
        }
        for(BodyIdx i =0 ; i < nBodies; i++){
                printf("Body: %u, %lx, pred: %x, Pos: %f %f %f\n", i, _hbodies + sizeof(HBBody) * i, hbodies[i].pred, hbodies[i].pos.val[0], hbodies[i].pos.val[1], hbodies[i].pos.val[2]);
        }

        hb_mc_dimension_t tg_dim = { .x = TILE_GROUP_DIM_X, .y = TILE_GROUP_DIM_Y};
        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1};
        // We can't transfer floats as arguments directly, so we pass it encoded as a binary value
        uint32_t fdiamsq = *reinterpret_cast<uint32_t *>(&diamsq);
        uint32_t cuda_argv[5] = {_config, _hnodes, _hbodies, nBodies, fdiamsq};

        HB_MC_CUDA_CALL(hb_mc_device_dma_to_device(device, &htod_bodies, 1));
        HB_MC_CUDA_CALL(hb_mc_device_dma_to_device(device, &htod_nodes, 1));
        HB_MC_CUDA_CALL(hb_mc_kernel_enqueue (device, grid_dim, tg_dim, "forces", 5, cuda_argv));

        /* Launch and execute all tile groups on device and wait for all to finish.  */
        HB_MC_CUDA_CALL(hb_mc_device_tile_groups_execute(device));

        hb_mc_dma_dtoh_t dtoh = {
                .d_addr = _hbodies,
                .h_addr = hbodies,
                .size   = sizeof(HBBody) * nBodies
        };

        HB_MC_CUDA_CALL(hb_mc_device_dma_to_host(device, &dtoh, 1));

        return HB_MC_SUCCESS;
}

int hb_mc_manycore_device_update_bodies(hb_mc_device_t *device, eva_t _config, unsigned int nBodies, HBBody *hbodies, eva_t _hbodies){
        hb_mc_dma_htod_t htod = {
                .d_addr = _hbodies,
                .h_addr = hbodies,
                .size   = sizeof(HBBody) * nBodies
        };

        hb_mc_dimension_t tg_dim = { .x = TILE_GROUP_DIM_X, .y = TILE_GROUP_DIM_Y};
        hb_mc_dimension_t grid_dim = { .x = 1, .y = 1};
        uint32_t cuda_argv[3] = {_config, nBodies, _hbodies};

        HB_MC_CUDA_CALL(hb_mc_device_dma_to_device(device, &htod, 1));
        HB_MC_CUDA_CALL(hb_mc_kernel_enqueue (device, grid_dim, tg_dim, "update", 3, cuda_argv));

        /* Launch and execute all tile groups on device and wait for all to finish.  */
        HB_MC_CUDA_CALL(hb_mc_device_tile_groups_execute(device));

        hb_mc_dma_dtoh_t dtoh = {
                .d_addr = _hbodies,
                .h_addr = hbodies,
                .size   = sizeof(HBBody) * nBodies
        };

        HB_MC_CUDA_CALL(hb_mc_device_dma_to_host(device, &dtoh, 1));

        return HB_MC_SUCCESS;
}

void run(Bodies& bodies, BodyPtrs& pBodies, size_t nbodies) {
        typedef galois::worklists::StableIterator<true> WLL;


        galois::preAlloc(galois::getActiveThreads() +
                         (3 * sizeof(Octree) + 2 * sizeof(Body)) * nbodies /
                         galois::runtime::pagePoolSize());
        galois::reportPageAlloc("MeminfoPre");
        hb_mc_device_t device;
        std::string test_name = "Barnes-Hut Simulation";
        eva_t _config;

        HB_MC_CUDA_CALL(hb_mc_device_init_custom_dimensions(&device, test_name.c_str(), 0, { .x = TILE_GROUP_DIM_X, .y = TILE_GROUP_DIM_Y}));
        HB_MC_CUDA_CALL(hb_mc_device_program_init(&device, "kernel.riscv", "default_allocator", 0));
        HB_MC_CUDA_CALL(hb_mc_device_malloc(&device, sizeof(config), &_config));
        hb_mc_dma_htod_t htod = {
                .d_addr = _config,
                .h_addr = &config,
                .size   = sizeof(config)
        };
        HB_MC_CUDA_CALL(hb_mc_device_dma_to_device(&device, &htod, 1));

        for (int step = 0; step < ntimesteps; step++) {

                auto mergeBoxes = [](const BoundingBox& lhs, const BoundingBox& rhs) {
                        return lhs.merge(rhs);
                };

                auto identity = []() { return BoundingBox(); };

                // Do tree building sequentially
                auto boxes = galois::make_reducible(mergeBoxes, identity);

                galois::do_all(
                               galois::iterate(pBodies),
                               [&boxes](const Body* b) { boxes.update(BoundingBox(b->pos)); },
                               galois::loopname("reduceBoxes"));

                BoundingBox box = boxes.reduce();

                // ============================================================
                // Build Octree (on the Host)
                Tree t;
                BuildOctree treeBuilder{t};
                Octree& top = t.emplace(box.center());

                galois::StatTimer T_build("BuildTime");
                T_build.start();
                galois::do_all(
                               galois::iterate(pBodies),
                               [&](Body* body) { treeBuilder.insert(body, &top, box.radius()); },
                               galois::loopname("BuildTree"));
                T_build.stop();

                // DR: Convert Bodies, Tree into arrays of Body and Octree
                unsigned int nBodies = 0, nNodes = 0, i = 0;
                for (auto ii : pBodies) { nBodies ++; }
                for (auto ii : t) { nNodes ++; }

                printf("x86 Created %u Nodes\n", nNodes);

                Body **BodyPtrs = new Body*[nBodies];
                Octree **OctNodePtrs = new Octree*[nNodes];
                OctNodePtrs[0] = &top;

                // Host buffers
                HBOctree *HostHBOctNodes = new HBOctree[nNodes];
                HBBody *HostHBBodies = new HBBody[nBodies];

                // HammerBlade buffers. We don't know how many nodes
                // HB will need to create, so we make a very rough
                // estimate.
                NodeIdx maxNodes = nBodies * std::log2(nBodies) + 128, nHBNodes = maxNodes;
                HBOctree *DeviceHBOctNodes = new HBOctree[maxNodes];
                HBOctree *HBOctNodes;
                eva_t _DeviceHBOctNodes = 0, _HostHBOctNodes, _HBOctNodes;
                HB_MC_CUDA_CALL(hb_mc_device_malloc(&device, maxNodes * sizeof(HBOctree), &_DeviceHBOctNodes));
                HB_MC_CUDA_CALL(hb_mc_device_malloc(&device, nNodes * sizeof(HBOctree), &_HostHBOctNodes));
                printf("Nodes EVA (size): %x (%lu)\n", _DeviceHBOctNodes, maxNodes *sizeof(HBOctree));

                HBBody *DeviceHBBodies = new HBBody[nBodies];
                HBBody *HBBodies;
                eva_t _HostHBBodies, _DeviceHBBodies, _HBBodies;
                HB_MC_CUDA_CALL(hb_mc_device_malloc(&device, nBodies * sizeof(HBBody), &_DeviceHBBodies));
                HB_MC_CUDA_CALL(hb_mc_device_malloc(&device, nBodies * sizeof(HBBody), &_HostHBBodies));
                printf("Bodies EVA (size): %x (%lu)\n", _HostHBBodies, nBodies * sizeof(HBBody));

                // If we use HostHBBodies to construct the array, they
                // are produced from an in-order traversal of the Host
                // Octree, and will have locality. This will increase
                // contention. I believe the raw array has less
                // contention in its ordering (and profiler stats
                // support this).
                BodyIdx body_i = 0;
                for (auto b : pBodies) {
                        b->convert(0, DeviceHBBodies[body_i]);
                        body_i ++;
                }

                // TODO: Create switch
                // Convert Octree node to HBOctree node
                top.convert(_DeviceHBOctNodes, DeviceHBOctNodes[0]);
                DeviceHBOctNodes[0].nChildren = 0;
                
                // Perform in-order octree tree traversal to enumerate all nodes/bodies
                // Use the node/body pointer to create a map between nodes/bodies and their index
                // Convert the x86 nodes/bodies to HB equivalents for processing

                // Build tree on the device:
                hb_mc_manycore_host_build_tree(nBodies, BodyPtrs, HostHBBodies, _HostHBBodies,
                                               nNodes, OctNodePtrs, HostHBOctNodes, _HostHBOctNodes);


                hb_mc_manycore_device_build_tree(&device, _config, &nHBNodes, DeviceHBOctNodes, _DeviceHBOctNodes, nBodies, DeviceHBBodies, _DeviceHBBodies, box.radius());
                printf("Created %u HB Nodes\n", nHBNodes);

                // TODO: Set HBOctNodes to Device or Host Version
                // TODO: Set HBBodies to Device or Host Version

                // TODO: I don't know if the code for building a tree
                // is 100% correct. Next step is to write a method
                // that verifies the tree -- checking that the
                // position of all the children at a node are within
                // it's radius, and are in the correct octant. It
                // would also be good to create a method that takes a
                // HB tree and turns it back into a x86 tree so we can
                // hand it back to the CPU code

                // TL;DR, the biggest next-step is verification.
                exit(1);

                // ============================================================

                // ============================================================
                // Summarize centers of mass in tree
                galois::timeThis(
                                 [&](void) {
                                         unsigned size = computeCenterOfMass(&top);
                                         // printTree(&top);
                                         std::cout << "Tree Size: " << size << "\n";
                                 },
                                 "summarize-Serial");

                // TODO: Implement skipping for CoM summarizaion
                hb_mc_manycore_device_summarize_centers(&device, _config, nNodes, HBOctNodes, _HBOctNodes, nBodies, HBBodies, _HBBodies);

                float pmse = 0.0f;
                for(NodeIdx node_i = 0; node_i < nNodes; node_i++){
                        printf("HB: %2.4f %2.4f %2.4f (%2.4f)\n", HBOctNodes[node_i].pos.val[0], HBOctNodes[node_i].pos.val[1], HBOctNodes[node_i].pos.val[2], HBOctNodes[node_i].mass);
                        printf("86: %2.4f %2.4f %2.4f (%2.4f)\n", OctNodePtrs[node_i]->pos.val[0], OctNodePtrs[node_i]->pos.val[1], OctNodePtrs[node_i]->pos.val[2], OctNodePtrs[node_i]->mass);
                        pmse += (HBOctNodes[node_i].pos - OctNodePtrs[node_i]->pos).dist2();
                }
                printf("Position MSE: %f\n", pmse);

                // DR: Update centers of mass in the HB nodes, if the previous kernel is not run
                for(NodeIdx node_i = 0; node_i < nNodes; node_i++){
                        HBOctNodes[node_i].mass = OctNodePtrs[node_i]->mass;
                        HBOctNodes[node_i].pos = OctNodePtrs[node_i]->pos;
                }
                // ============================================================


                // ============================================================
                // Compute pair-wise forces using the Barnes-Hut Approximation
                ComputeForces cf(&top, box.diameter());

                galois::StatTimer T_compute("ComputeTime");
                T_compute.start();
                galois::for_each(
                                 galois::iterate(pBodies),
                                 [&](Body* b, auto& cnx) { cf.computeForce(b, cnx); },
                                 galois::loopname("compute"), galois::wl<WLL>(),
                                 galois::disable_conflict_detection(), galois::no_pushes(),
                                 galois::per_iter_alloc());
                T_compute.stop();

                // TODO: Implement skipping for forces computation
                // hb_mc_manycore_device_compute_forces(&device, _config, box.diameter(), nNodes, HBOctNodes, _HBOctNodes, nBodies, HBBodies, _HBBodies);                

                float amse = 0.0f;
                for(BodyIdx body_i = 0; body_i < nBodies; body_i++){
                        printf("HB: %2.4f %2.4f %2.4f\n", HBBodies[body_i].acc.val[0], HBBodies[body_i].acc.val[1], HBBodies[body_i].acc.val[2]);
                        printf("86: %2.4f %2.4f %2.4f\n", BodyPtrs[body_i]->acc.val[0], BodyPtrs[body_i]->acc.val[1], BodyPtrs[body_i]->acc.val[2]);
                        amse += (HBBodies[body_i].acc - BodyPtrs[body_i]->acc).dist2();
                }
                printf("Acceleration MSE: %f\n", amse);

                // DR: Update centers of mass in the HB nodes
                for(BodyIdx body_i = 0; body_i < nBodies; body_i++){
                        HBBodies[body_i].acc = BodyPtrs[body_i]->acc;
                        HBBodies[body_i].vel = BodyPtrs[body_i]->vel;
                }
                // ============================================================


                // Verify the results using a classic pair-wise computation on a random sampling of nodes
                if (!skipVerify) {
                        galois::timeThis(
                                         [&](void) {
                                                 std::cout << "MSE (sampled) "
                                                           << checkAllPairs(bodies, std::min((int)nbodies, 100))
                                                           << "\n";
                                         },
                                         "checkAllPairs");
                }

                // ============================================================
                // Update velocity and position.
                galois::do_all(
                               galois::iterate(pBodies),
                               [](Body* b) {
                                       Point dvel(b->acc);
                                       dvel *= config.dthf;
                                       Point velh(b->vel);
                                       velh += dvel;
                                       b->pos += velh * config.dtime;
                                       b->vel = velh + dvel;
                               },
                               galois::loopname("advance"));

                // TODO: Implement skipping switch for update
                // hb_mc_manycore_device_update_bodies(&device, _config, nBodies, HBBodies, _HBBodies);

                pmse = 0.0f;
                float vmse = 0.0f;
                for(BodyIdx body_i = 0; body_i < nBodies; body_i++){
                        pmse += (HBBodies[body_i].pos - BodyPtrs[body_i]->pos).dist2();
                        vmse += (HBBodies[body_i].vel - BodyPtrs[body_i]->vel).dist2();
                }
                printf("Position MSE: %f Velocity MSE: %f\n", pmse/nBodies, vmse/nBodies);
                // ============================================================

                std::cout << "Timestep " << step << " Center of Mass = ";
                std::ios::fmtflags flags =
                        std::cout.setf(std::ios::showpos | std::ios::right |
                                       std::ios::scientific | std::ios::showpoint);
                std::cout << top.pos;
                std::cout.flags(flags);
                std::cout << "\n";

                HB_MC_CUDA_CALL(hb_mc_device_free(&device, _HBBodies));
                HB_MC_CUDA_CALL(hb_mc_device_free(&device, _HBOctNodes));
        }

        galois::reportPageAlloc("MeminfoPost");
        HB_MC_CUDA_CALL(hb_mc_device_free(&device, _config));
        HB_MC_CUDA_CALL(hb_mc_device_finish(&device));
}

int barneshut_main(int argc, char *argv[]) {
        galois::SharedMemSys G;
        LonestarStart(argc, argv, name, desc, url, nullptr);

        galois::StatTimer totalTime("TimerTotal");
        totalTime.start();

        std::cout << config << "\n";
        std::cout << nbodies << " bodies, " << ntimesteps << " time steps\n";

        Bodies bodies;
        BodyPtrs pBodies;
        generateInput(bodies, pBodies, nbodies, seed);

        galois::StatTimer execTime("Timer_0");
        execTime.start();
        run(bodies, pBodies, nbodies);
        execTime.stop();

        totalTime.stop();

        return 0;
}

declare_program_main("Barnes Hut", barneshut_main);
