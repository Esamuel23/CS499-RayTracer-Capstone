//=============================================================================
// File:     aabb.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Axis-aligned bounding box and its ray test (slab method).
//
// Design notes:
//   Every Hittable reports a bounding box. Enhancement One only uses it for
//   testing. Enhancement Two (algorithms and data structures) will build a
//   bounding volume hierarchy on top of it, so the interface is in place now.
//=============================================================================
#pragma once

#include "core/ray.h"
#include "core/vec3.h"

namespace rt {

struct Aabb {
    Point3 min;
    Point3 max;

    // Returns true when the ray passes through the box inside (t_min, t_max).
    bool hit(const Ray& ray, double t_min, double t_max) const;

    // Smallest box that contains both inputs.
    static Aabb surrounding(const Aabb& a, const Aabb& b);
};

}  // namespace rt
