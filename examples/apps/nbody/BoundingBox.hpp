#pragma once

#include <Point.hpp>

// DR: What is a bounding box used for?
struct BoundingBox {
        Point min;
        Point max;
        explicit BoundingBox(const Point& p) : min(p), max(p) {}
        BoundingBox()
                : min(std::numeric_limits<float>::max()),
                  max(std::numeric_limits<float>::min()) {}

        // DR: Merge two bounding boxes by taking the min and max
        // x,y,z dimension from this box and another.
        BoundingBox merge(const BoundingBox& other) const {
                BoundingBox copy(*this);

                copy.min.pairMin(other.min);
                copy.max.pairMax(other.max);
                return copy;
        }

        float diameter() const { return (max - min).minDim(); }
        float radius() const { return diameter() * 0.5; }
        // DR: Compute the geometric center of the bounding box
        Point center() const { return (min + max) * 0.5; }
};
