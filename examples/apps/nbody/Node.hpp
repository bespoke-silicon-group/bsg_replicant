#pragma once

#include <Point.hpp>

typedef unsigned int NodeIdx;

struct _Node {
        Point pos; // DR: X, Y, Z location
        int octant;
        float mass;
};

struct HBNode : public _Node{
#ifndef RISCV
        eva_t pred; // Used on Manycore
#else
        HBNode *pred;
#endif
};

#ifndef RISCV
struct Node : public _Node{
#ifndef RISCV
        bool Leaf;
#endif
        Node *pred; // Used on x86
        void convert(eva_t pred, HBNode &n){
                _Node &_n = static_cast<_Node&>(n);
                _n = static_cast<_Node>(*this);
                n.pred = pred;
        }
        bool isMatch(HBNode &n, bool HBleaf){
                return n.pos == pos && n.mass == mass && HBleaf == Leaf && n.octant == octant;
        }
        
};
#endif
