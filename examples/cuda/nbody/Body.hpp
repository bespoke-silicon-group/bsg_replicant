#pragma once

#include <Point.hpp>
#include <Node.hpp>

struct Body : public Node {
        Point vel;
        Point acc;
};
