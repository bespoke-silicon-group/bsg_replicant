#pragma once

#include <Node.hpp>
#include <array>

/**
 * A node in an octree is either an internal node or a leaf.
 */

struct _Octree {
        static const int octants = 8;
        int nChildren;
        char cLeafs;
};

struct HBOctree : public HBNode, public _Octree {
        // DR: Note, no lock!
#ifndef RISCV
        std::array<eva_t, 8> child = {0};
#else
        std::array<HBNode*, 8> child = {0};
#endif
        HBOctree(const Point& p) {
                HBNode::pos  = p;
                HBNode::Leaf = false;
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
        }
        void convert(eva_t pred, HBOctree &o){
                Node::convert(pred, static_cast<HBNode&>(o));
                _Octree &_o = static_cast<_Octree&>(o);
                _o = static_cast<_Octree>(*this);
        }
        bool isMatch(HBOctree &o){
                return Node::isMatch(static_cast<HBNode&>(o)) && o.cLeafs == cLeafs && o.nChildren == nChildren;
        }        
};
#endif

