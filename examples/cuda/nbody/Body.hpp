#pragma once

#include <Point.hpp>

struct Body : public Node {
        Point vel;
        Point acc;
};
