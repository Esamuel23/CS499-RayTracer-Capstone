//=============================================================================
// File:     ray.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  A ray P(t) = origin + t * direction.
//
// Design notes:
//   The ray replaces the draw call as the unit of work. The rasterizer asked
//   "which pixels does this triangle cover?" The ray tracer asks "what does
//   this pixel see?" Every shape and material in the project is written
//   against this one small type.
//=============================================================================
#pragma once

#include "core/vec3.h"

namespace rt {

class Ray {
public:
    Ray() = default;
    Ray(const Point3& origin, const Vec3& direction)
        : origin_(origin), direction_(direction) {}

    const Point3& origin() const { return origin_; }
    const Vec3& direction() const { return direction_; }

    Point3 at(double t) const { return origin_ + t * direction_; }

private:
    Point3 origin_;
    Vec3 direction_;
};

}  // namespace rt
