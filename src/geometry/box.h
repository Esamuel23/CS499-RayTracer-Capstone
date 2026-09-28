//=============================================================================
// File:     box.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Axis-aligned box with an exact ray intersection (slab method).
//
// Design notes:
//   The original drew a unit cube mesh scaled by scaleXYZ and moved to
//   positionXYZ. This class takes those same two values (center and size),
//   so the desk scene carries over without converting any numbers.
//   The original scene never rotates a box, so rotation is out of scope.
//=============================================================================
#pragma once

#include <memory>

#include "core/hittable.h"

namespace rt {

class Box final : public Hittable {
public:
    // Throws std::invalid_argument if any size component <= 0 or the
    // material is null.
    Box(const Point3& center, const Vec3& size, std::shared_ptr<const Material> material);

    bool hit(const Ray& ray, double t_min, double t_max, HitRecord& record) const override;
    Aabb bounding_box() const override;

private:
    Point3 min_;
    Point3 max_;
    std::shared_ptr<const Material> material_;
};

}  // namespace rt
