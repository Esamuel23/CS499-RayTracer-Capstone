//=============================================================================
// File:     cylinder.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Closed cylinder standing upright on the Y axis.
//
// Design notes:
//   Solved analytically: a quadratic for the curved side plus two disk
//   tests for the caps. The course mesh version approximates the side with
//   flat slices. The analytic version is exact at any zoom level.
//=============================================================================
#pragma once

#include <memory>

#include "core/hittable.h"

namespace rt {

class Cylinder final : public Hittable {
public:
    // center is the midpoint of the axis. Throws std::invalid_argument if
    // radius or height <= 0, or the material is null.
    Cylinder(const Point3& center, double radius, double height,
             std::shared_ptr<const Material> material);

    bool hit(const Ray& ray, double t_min, double t_max, HitRecord& record) const override;
    Aabb bounding_box() const override;

private:
    Point3 center_;
    double radius_;
    double half_height_;
    std::shared_ptr<const Material> material_;
};

}  // namespace rt
