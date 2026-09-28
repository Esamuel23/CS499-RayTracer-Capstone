//=============================================================================
// File:     aabb.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Slab-method ray test for axis-aligned bounding boxes.
//=============================================================================
#include "core/aabb.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "core/constants.h"

namespace rt {

bool Aabb::hit(const Ray& ray, double t_min, double t_max) const {
    for (int axis = 0; axis < 3; ++axis) {
        const double origin = ray.origin()[axis];
        const double direction = ray.direction()[axis];

        // A ray parallel to this slab never enters or leaves it.
        // It is inside for all t, or outside for all t.
        // Handling this case avoids dividing by zero.
        if (std::fabs(direction) < kParallelEpsilon) {
            if (origin < min[axis] || origin > max[axis]) {
                return false;
            }
            continue;
        }

        const double inverse = 1.0 / direction;
        double t0 = (min[axis] - origin) * inverse;
        double t1 = (max[axis] - origin) * inverse;
        if (inverse < 0.0) {
            std::swap(t0, t1);
        }

        // Narrow the interval. If it becomes empty the ray misses.
        t_min = std::max(t0, t_min);
        t_max = std::min(t1, t_max);
        if (t_max <= t_min) {
            return false;
        }
    }
    return true;
}

Aabb Aabb::surrounding(const Aabb& a, const Aabb& b) {
    const Point3 low(std::fmin(a.min.x(), b.min.x()),
                     std::fmin(a.min.y(), b.min.y()),
                     std::fmin(a.min.z(), b.min.z()));
    const Point3 high(std::fmax(a.max.x(), b.max.x()),
                      std::fmax(a.max.y(), b.max.y()),
                      std::fmax(a.max.z(), b.max.z()));
    return Aabb{low, high};
}

}  // namespace rt
