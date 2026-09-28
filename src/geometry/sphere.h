//=============================================================================
// File:     sphere.h
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Sphere with an exact (analytic) ray intersection.
//=============================================================================
#pragma once

#include <memory>

#include "core/hittable.h"

namespace rt {

class Sphere final : public Hittable {
public:
    // Throws std::invalid_argument if radius <= 0 or material is null.
    Sphere(const Point3& center, double radius, std::shared_ptr<const Material> material);

    bool hit(const Ray& ray, double t_min, double t_max, HitRecord& record) const override;
    Aabb bounding_box() const override;

private:
    Point3 center_;
    double radius_;
    std::shared_ptr<const Material> material_;
};

}  // namespace rt
