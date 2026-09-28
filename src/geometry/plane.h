//=============================================================================
// File:     plane.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Finite horizontal plane (a flat rectangle facing +Y).
//
// Design notes:
//   The original plane mesh spans -1 to 1 on X and Z and is then scaled.
//   This class takes the scaled half extents directly, so the ground plane
//   keeps its original 40 x 40 size.
//=============================================================================
#pragma once

#include <memory>

#include "core/hittable.h"

namespace rt {

class Plane final : public Hittable {
public:
    // Throws std::invalid_argument if a half extent <= 0 or the material
    // is null.
    Plane(const Point3& center, double half_extent_x, double half_extent_z,
          std::shared_ptr<const Material> material);

    bool hit(const Ray& ray, double t_min, double t_max, HitRecord& record) const override;
    Aabb bounding_box() const override;

private:
    Point3 center_;
    double half_x_;
    double half_z_;
    std::shared_ptr<const Material> material_;
};

}  // namespace rt
