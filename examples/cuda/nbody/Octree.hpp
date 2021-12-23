#pragma once

#include <Node.hpp>
#include <array>

/**
 * A node in an octree is either an internal node or a leaf.
 */
struct Octree : public Node {
        // DR: Note, lock!
        std::array<galois::substrate::PtrLock<Node>, 8> child;
        char cLeafs;
        char nChildren;

        Octree(const Point& p) {
                Node::pos  = p;
                Node::Leaf = false;
                cLeafs     = 0;
                nChildren  = 0;
        }
};
