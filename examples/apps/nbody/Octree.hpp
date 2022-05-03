#pragma once

#include <Node.hpp>
#include <array>
#ifdef RISCV
#define BSG_TILE_GROUP_X_DIM bsg_tiles_X
#define BSG_TILE_GROUP_Y_DIM bsg_tiles_Y
#include <bsg_mcs_mutex.hpp>
#else
typedef std::atomic<eva_t> bsg_mcs_mutex_t;
#endif

struct _Octree {
        static const int octants = 8;
        int nChildren;
        char cLeafs;
};

struct HBOctree : public HBNode, public _Octree {
        // DR: Note, no lock!
#ifndef RISCV
        std::array<eva_t, 8> child = {0};
        bsg_mcs_mutex_t mtx;
#else
        std::array<HBNode*, 8> child = {0};
        bsg_mcs_mutex_t mtx;
#endif
        HBOctree(const Point& p) {
                HBNode::pos  = p;
                cLeafs     = 0;
                nChildren  = 0;
        }
        HBOctree() {
        }
};

#ifndef RISCV
struct Octree : public Node, public _Octree {
        // DR: Note, lock!
        std::array<galois::substrate::PtrLock<Node>, 8> child;

        Octree(const Point& p) {
                Node::pos  = p;
                Node::Leaf = false;
                cLeafs     = 0;
                nChildren  = 0;
                for(int i = 0; i < 8; ++i){
                        child[i].lock();
                        child[i].unlock_and_set(nullptr);
                }
        }
        void convert(eva_t pred, HBOctree &o){
                Node::convert(pred, static_cast<HBNode&>(o));
                _Octree &_o = static_cast<_Octree&>(o);
                _o = static_cast<_Octree>(*this);
        }
        bool isMatch(HBOctree &o, bool HBLeaf){
                return Node::isMatch(static_cast<HBNode&>(o), HBLeaf) && o.cLeafs == cLeafs && o.nChildren == nChildren;
        }        
};
#endif



#ifdef RISCV
template<unsigned int DEPTH>
class HBOctreeTraverser{
public:
        bsg_mcs_mutex_node_t lcl, *lcl_as_glbl;
        HBOctreeTraverser(){
                lcl_as_glbl = (bsg_mcs_mutex_node_t*)bsg_tile_group_remote_ptr(int, bsg_x, bsg_y, &lcl);
        }

        // HBOctreeTraverser(const HBOctreeTraverser &t) = delete;

        void lock(unsigned int depth, HBOctree *cur){
                bsg_mcs_mutex_acquire(&cur->mtx, &lcl, lcl_as_glbl);

        }

        void unlock(unsigned int depth, HBOctree *cur){
                bsg_mcs_mutex_release(&cur->mtx, &lcl, lcl_as_glbl);
        }                
};

#endif
