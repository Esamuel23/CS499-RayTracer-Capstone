//=============================================================================
// File:     cone.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Upright cone with its tip at the top and a closed base.
//
// Design notes:
//   Solved analytically like the cylinder, but the radius shrinks linearly
//   toward the tip. That adds a Y term to the quadratic and a case where
//   the quadratic collapses to a linear equation.
//=============================================================================
#pragma once

#include <memory>

#include "core/hittable.h"

namespace rt {

class Cone final : public Hittable {
public:
    // center is the midpoint between base and tip. Throws
    // std::invalid_argument if radius or height <= 0, or the material is null.
    Cone(const Point3& center, double base_radius, double height,
         std::shared_ptr<const Material> material);

    bool hit(const Ray& ray, double t_min, double t_max, HitRecord& record) const override;
    Aabb bounding_box() const override;

private:
    Point3 center_;
    double base_radius_;
    double half_height_;
    double slope_squared_;  // (base_radius / height)^2, the cone's steepness.
    std::shared_ptr<const Material> material_;
};

}  // namespace rt
