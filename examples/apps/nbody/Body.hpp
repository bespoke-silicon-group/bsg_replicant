#pragma once

#include <Point.hpp>
#include <Node.hpp>

typedef NodeIdx BodyIdx;

struct _Body {
        Point vel;
        Point acc;
};

struct HBBody : public HBNode, public _Body{
};

#ifndef RISCV
struct Body : public Node, public _Body {
        void convert(eva_t pred, HBBody &b){
                Node::convert(pred, static_cast<HBNode&>(b));
                _Body &_b = static_cast<_Body&>(b);
                _b = static_cast<_Body>(*this);
        }

        bool isMatch(HBBody &b, bool HBLeaf){
                return Node::isMatch(static_cast<HBNode&>(b), HBLeaf) && b.vel == vel && b.acc == acc;
        }
};
#endif
