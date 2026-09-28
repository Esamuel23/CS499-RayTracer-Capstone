//=============================================================================
// File:     plane.cpp
// Author:   Elijah Samuel
// Course:   CS 499 Computer Science Capstone, SNHU
// Date:     September 2026
// Purpose:  Ray intersection with a finite horizontal rectangle.
//=============================================================================
#include "geometry/plane.h"

#include <cmath>
#include <stdexcept>
#include <utility>

#include "core/constants.h"

namespace rt {

Plane::Plane(const Point3& center, double half_extent_x, double half_extent_z,
             std::shared_ptr<const Material> material)
    : center_(center),
      half_x_(half_extent_x),
      half_z_(half_extent_z),
      material_(std::move(material)) {
    if (!(half_x_ > 0.0 && half_z_ > 0.0)) {
        throw std::invalid_argument("Plane half extents must be positive");
    }
    if (!material_) {
        throw std::invalid_argument("Plane requires a material");
    }
}

bool Plane::hit(const Ray& ray, double t_min, double t_max, HitRecord& record) const {
    // A ray parallel to the plane never crosses it. Checking first avoids
    // a divide by zero below.
    const double direction_y = ray.direction().y();
    if (std::fabs(direction_y) < kParallelEpsilon) {
        return false;
    }

    const double t = (center_.y() - ray.origin().y()) / direction_y;
    if (t <= t_min || t >= t_max) {
        return false;
    }

    // The infinite plane is hit. Check the point is inside the rectangle.
    const Point3 point = ray.at(t);
    if (std::fabs(point.x() - center_.x()) > half_x_ ||
        std::fabs(point.z() - center_.z()) > half_z_) {
        return false;
    }

    record.t = t;
    record.point = point;
    record.set_face_normal(ray, Vec3(0.0, 1.0, 0.0));
    record.material = material_.get();
    return true;
}

Aabb Plane::bounding_box() const {
    // Pad the thickness so the box is never zero-width on Y.
    constexpr double kThicknessPad = 1e-4;
    const Vec3 extent(half_x_, kThicknessPad, half_z_);
    return Aabb{center_ - extent, center_ + extent};
}

}  // namespace rt
