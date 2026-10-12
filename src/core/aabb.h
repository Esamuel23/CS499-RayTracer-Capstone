//=============================================================================
// File:     aabb.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Axis-aligned bounding box and its ray test (slab method).
//
// Design notes:
//   Every Hittable reports a bounding box. Enhancement One only used it for
//   testing. Enhancement Two (algorithms and data structures) builds a
//   bounding volume hierarchy on top of it (src/accel/bvh.h). The BVH needs
//   two extras: the entry distance of a hit, so it can visit the nearer
//   child first, and the surface area, for the surface area heuristic.
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

    // Same test. On a hit, also writes the parameter where the ray enters
    // the box, clamped to t_min (a ray that starts inside enters at t_min).
    // t_enter is left untouched on a miss.
    bool hit(const Ray& ray, double t_min, double t_max, double& t_enter) const;

    // 2 * (dx*dy + dy*dz + dz*dx). A flat box (zero on one axis) still has
    // a positive area. Only a box that is a line or a point has area 0.
    double surface_area() const;

    // Smallest box that contains both inputs.
    static Aabb surrounding(const Aabb& a, const Aabb& b);
};

}  // namespace rt
