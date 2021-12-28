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
                        insert(b, static_cast<Octree*>(child), radius);
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

                // DR: Why reduce radius?
                radius *= 0.5;

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
                        // DR: Recurse into the child. This is redundant.
                        node->child[index].unlock();
                        insert(b, static_cast<Octree*>(child), radius);
                }
        }
};

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

Point updateForce(Point delta, float psq, float mass) {
        // Computing force += delta * mass * (|delta|^2 + eps^2)^{-3/2}
        float idr   = 1 / sqrt((float)(psq + config.epssq));
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
                        Point p    = b.pos - f.node->pos;
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

// nodes[0] is the tree root
void hb_mc_manycore_build_tree(int nBodies, Body **bodies, std::map<Body*, BodyIdx> &bodyIdxMap, HBBody *hbodies, eva_t _hbodies,
                               int nNodes, Octree **nodes, std::map<Node*, NodeIdx> &nodeIdxMap, HBOctree *hnodes, eva_t _hnodes){

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

                // DR: Check that the tree conversion happened correctly
                // Don't check the root node (index 0) because it points to itself, start at index 1.
                for (node_i = 1; node_i < nNodes; node_i ++){
                        Octree *n = nodes[node_i];
                        HBOctree *_n = &hnodes[node_i];
                        int pred_i = (_n->pred - _hnodes)/sizeof(HBOctree);
                        // Check that Host and HB predecessors correspond                        
                        if(n->pred != nodes[pred_i]){
                                printf("Error: Octree Node %d predecessor does not match HBNode!\n", node_i);
                                exit(1);
                        }

                        // Check data match
                        if(!n->isMatch(*_n)){
                                printf("Node %d Data Mismatch\n", node_i);
                                exit(1);
                        }
                        
                        int octant_i = n->pred->pos.getChildIndex(_n->pos);
                        eva_t _p = hnodes[pred_i].child[octant_i];
                        if(_p != (_hnodes + node_i * sizeof(HBOctree))){
                                printf("Error: HBOctree Node %d EVA does not match predecessor EVA @ Octant index!\n");
                                exit(1);
                        }
                }

                for (body_i = 0 ; body_i < nBodies; body_i ++){
                        Body *b = bodies[body_i];
                        HBBody *_b = &hbodies[body_i];
                        int pred_i = (_b->pred - _hnodes)/sizeof(HBOctree);
                        // Check that Host and HB predecessors correspond
                        if(b->pred != nodes[pred_i]){
                                printf("Error: Octree Body %d predecessor does not match HBBody!\n", body_i);
                                exit(1);
                        }

                        if(!b->isMatch(*_b)){
                                printf("Body %d Data Mismatch\n", body_i);
                                exit(1);
                        }

                        // Check the predecessor's corresponding octant child eva matches the current body's eva.
                        int octant_i = b->pred->pos.getChildIndex(_b->pos);
                        eva_t _c = hnodes[pred_i].child[octant_i];
                        if(_c != (_hbodies + body_i * sizeof(HBBody))){
                                printf("Error: HBBody %d EVA does not match predecessor EVA @ Octant index!\n");
                                exit(1);
                        }
                }
}

void run(Bodies& bodies, BodyPtrs& pBodies, size_t nbodies) {
        typedef galois::worklists::StableIterator<true> WLL;

        galois::preAlloc(galois::getActiveThreads() +
                         (3 * sizeof(Octree) + 2 * sizeof(Body)) * nbodies /
                         galois::runtime::pagePoolSize());
        galois::reportPageAlloc("MeminfoPre");

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
                int nBodies = 0, nNodes = 0, i = 0;
                for (auto ii : pBodies) { nBodies ++; }
                for (auto ii : t) { nNodes ++; }

                Body **BodyPtrs = new Body*[nBodies];
                Octree **OctNodePtrs = new Octree*[nNodes];
                OctNodePtrs[0] = &top;

                // HammerBlade buffers. For now, I am just faking the EVA space to ensure correctness.
                HBOctree HBOctNodes[nNodes];
                eva_t _HBOctNodes = 0;
                // BSG_CUDA_CALL(hb_mc_device_malloc(&device, sizeof(HBOctNodes), &_HBOctNodes));
                HBBody HBBodies[nBodies];
                eva_t _HBBodies = nNodes * sizeof(HBOctNodes);
                // BSG_CUDA_CALL(hb_mc_device_malloc(&device, sizeof(HBBodes), &_HBBodies));

                // Perform in-order octree tree traversal to enumerate all nodes/bodies
                // Use the node/body pointer to create a map between nodes/bodies and their index
                // Convert the x86 nodes/bodies to HB equivalents for processing

                // Map of Octree node/body pointer to index
                std::map<Node*, NodeIdx> nodeIdxMap;
                std::map<Body*, BodyIdx> bodyIdxMap;
                hb_mc_manycore_build_tree(nBodies, BodyPtrs, bodyIdxMap, HBBodies, _HBBodies,
                                          nNodes, OctNodePtrs, nodeIdxMap, HBOctNodes, _HBOctNodes);
                
                // DR: Send to Manycore to do bottom-up traversal of Octree to update centers of mass
                // DR: Transfer back from Manycore

                // update centers of mass in tree
                galois::timeThis(
                                 [&](void) {
                                         unsigned size = computeCenterOfMass(&top);
                                         // printTree(&top);
                                         std::cout << "Tree Size: " << size << "\n";
                                 },
                                 "summarize-Serial");

                ComputeForces cf(&top, box.diameter());
                // DR: Send to Manycore to do per-body computation of forces.
                // DR: After all tiles have finished, barrier, and update position/velocity.
                // DR: Transfer back from Manycore

                galois::StatTimer T_compute("ComputeTime");
                T_compute.start();
                galois::for_each(
                                 galois::iterate(pBodies),
                                 [&](Body* b, auto& cnx) { cf.computeForce(b, cnx); },
                                 galois::loopname("compute"), galois::wl<WLL>(),
                                 galois::disable_conflict_detection(), galois::no_pushes(),
                                 galois::per_iter_alloc());
                T_compute.stop();

                if (!skipVerify) {
                        galois::timeThis(
                                         [&](void) {
                                                 std::cout << "MSE (sampled) "
                                                           << checkAllPairs(bodies, std::min((int)nbodies, 100))
                                                           << "\n";
                                         },
                                         "checkAllPairs");
                }
                // Done in compute forces
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

                std::cout << "Timestep " << step << " Center of Mass = ";
                std::ios::fmtflags flags =
                        std::cout.setf(std::ios::showpos | std::ios::right |
                                       std::ios::scientific | std::ios::showpoint);
                std::cout << top.pos;
                std::cout.flags(flags);
                std::cout << "\n";
        }

        galois::reportPageAlloc("MeminfoPost");
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
