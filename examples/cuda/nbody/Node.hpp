#pragma once

#include <Point.hpp>

struct Node {
        Point pos; // DR: X, Y, Z location
        float mass;
        bool Leaf;
        char idx;
        union _pred {
                Node *p; // Used on x86
                int id; // Used on Manycore
        } pred;
};


